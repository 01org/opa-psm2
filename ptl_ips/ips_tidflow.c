/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2016 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Contact Information:
  Intel Corporation, www.intel.com

  BSD LICENSE

  Copyright(c) 2016 Intel Corporation.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* Copyright (c) 2003-2016 Intel Corporation. All rights reserved. */

#include "psm_user.h"
#include "ipserror.h"
#include "ips_proto.h"
#include "ips_proto_internal.h"

psm2_error_t ips_tf_init(struct ips_protoexp *protoexp,
			const psmi_context_t *context,
			struct ips_tf *tfc,
			ips_tf_avail_cb_fn_t cb)
{
	const struct hfi1_ctxt_info *ctxt_info = &context->ctrl->ctxt_info;
	int tf_idx;

#if TF_ADD
	struct psmi_stats_entry entries[] = {
		PSMI_STATS_DECL("tidflow update count",
				MPSPAWN_STATS_REDUCTION_ALL,
				NULL, &tfc->tf_num_total),
	};
#endif

	tfc->context = context;
	tfc->tf_num_total = 0;
	tfc->tf_num_inuse = 0;
	tfc->tf_avail_cb = cb;
	tfc->tf_avail_context = (void *)protoexp;
	if ((context->runtime_flags & HFI1_CAP_EXTENDED_PSN)) {
		tfc->tf_gen_mask = 0xFFFFF;
	} else {
		tfc->tf_gen_mask = 0x1FFF;
	}

	/* Allocate and Initialize tidrecvc array. */
	tfc->tidrecvc = (struct ips_tid_recv_desc *)
		psmi_calloc(context->ep, UNDEFINED, 1,
			sizeof(struct ips_tid_recv_desc)*HFI_TF_NFLOWS);
	if (tfc->tidrecvc == NULL)
		return PSM2_NO_MEMORY;

	for (tf_idx = 0; tf_idx < HFI_TF_NFLOWS; tf_idx++) {
		tfc->tidrecvc[tf_idx].context = context;
		tfc->tidrecvc[tf_idx].protoexp = protoexp;
		tfc->tidrecvc[tf_idx].rdescid._desc_idx = tf_idx;
		tfc->tidrecvc[tf_idx].rdescid._desc_genc = tf_idx;
		tfc->tidrecvc[tf_idx].tidflow.flowid = EP_FLOW_TIDFLOW;
		tfc->tidrecvc[tf_idx].tidflow.frag_size = protoexp->proto->epinfo.ep_mtu;
	}

	/* Shared control structure, it will be in shared memory
	 * for context sharing, otherwise calloc() it */
	tfc->tf_ctrl = (struct ips_tf_ctrl *)context->tf_ctrl;
	if (!tfc->tf_ctrl) {
		tfc->tf_ctrl = (struct ips_tf_ctrl *)
		    psmi_calloc(context->ep, UNDEFINED, 1,
				sizeof(struct ips_tf_ctrl));
		if (tfc->tf_ctrl == NULL) {
			return PSM2_NO_MEMORY;
		}
	}

	/*
	 * Only the master process can initialize.
	 */
	if (ctxt_info->subctxt == 0) {
		pthread_spin_init(&tfc->tf_ctrl->tf_ctrl_lock,
					PTHREAD_PROCESS_SHARED);
		tfc->tf_ctrl->tf_num_max = HFI_TF_NFLOWS;
		tfc->tf_ctrl->tf_num_avail = HFI_TF_NFLOWS;

		for (tf_idx = 0; tf_idx < HFI_TF_NFLOWS; tf_idx++) {
			/* Update flow state */
			tfc->tf_ctrl->tf[tf_idx].state = TF_STATE_DEALLOCATED;
			tfc->tf_ctrl->tf[tf_idx].tf_idx = tf_idx;
			tfc->tf_ctrl->tf[tf_idx].next_gen = 0;
			tfc->tf_ctrl->tf[tf_idx].next_free = tf_idx + 1;

			hfi_tidflow_reset(context->ctrl, tf_idx,
					  tfc->tf_gen_mask, 0x7FF);
		}
		tfc->tf_ctrl->tf_head = 0;
	}

#if TF_ADD
	/* TF_ADD: Add a new stats type for tid flows in psm_stats.h */
	return psmi_stats_register_type(PSMI_STATS_NO_HEADING,
					PSMI_STATSTYPE_TIDS,
					entries,
					PSMI_STATS_HOWMANY(entries), tidc);
#else
	return PSM2_OK;
#endif
}

psm2_error_t ips_tf_fini(struct ips_tf *tfc)
{
	if (!tfc->context->tf_ctrl)
		psmi_free(tfc->tf_ctrl);
	psmi_free(tfc->tidrecvc);
	return PSM2_OK;
}

/* Allocate a tidflow */
psm2_error_t ips_tf_allocate(struct ips_tf *tfc,
		struct ips_tid_recv_desc **tidrecvc)
{
	struct ips_tf_ctrl *ctrl = tfc->tf_ctrl;
	struct ips_tf_entry *entry;

	if (tfc->context->tf_ctrl)
		pthread_spin_lock(&ctrl->tf_ctrl_lock);

	if (!ctrl->tf_num_avail) {
		psmi_assert(ctrl->tf_head == HFI_TF_NFLOWS);
		*tidrecvc = NULL;

		if (tfc->context->tf_ctrl)
			pthread_spin_unlock(&ctrl->tf_ctrl_lock);

		return PSM2_EP_NO_RESOURCES;
	}

	entry = &ctrl->tf[ctrl->tf_head];
	ctrl->tf_head = entry->next_free;
	ctrl->tf_num_avail--;

	if (tfc->context->tf_ctrl)
		pthread_spin_unlock(&ctrl->tf_ctrl_lock);

	tfc->tf_num_total++;
	tfc->tf_num_inuse++;

	psmi_assert(entry->state == TF_STATE_DEALLOCATED);
	entry->state = TF_STATE_ALLOCATED;

	*tidrecvc = &(tfc->tidrecvc[entry->tf_idx]);
	/* initial tidflow generation */
	(*tidrecvc)->tidflow_active_gen = entry->next_gen;

	psmi_assert((*tidrecvc)->rdescid._desc_idx == entry->tf_idx);
	psmi_assert_always(entry->next_gen < tfc->tf_gen_mask);

	entry->next_gen++;
	if (entry->next_gen == tfc->tf_gen_mask)
		entry->next_gen = 0;

	return PSM2_OK;
}

/* Deallocate a tidflow */
psm2_error_t ips_tf_deallocate(struct ips_tf *tfc, uint32_t tf_idx)
{
	struct ips_tf_ctrl *ctrl = tfc->tf_ctrl;
	struct ips_tf_entry *entry;

	psmi_assert(tf_idx < HFI_TF_NFLOWS);
	psmi_assert(tf_idx >= 0);

	entry = &ctrl->tf[tf_idx];
	psmi_assert(entry->state == TF_STATE_ALLOCATED);
	entry->state = TF_STATE_DEALLOCATED;

	/*
	 * The wire protocol only uses 16bits tidrecvc generation
	 * count in exptid packet, this should be bigger enough,
	 * u16w3 is the lower 16bits of _desc_genc
	 */
	tfc->tidrecvc[tf_idx].rdescid.u16w3++;

	/* Mark invalid generation for flow (stale packets will be dropped) */
	hfi_tidflow_reset(tfc->context->ctrl, tf_idx, tfc->tf_gen_mask, 0x7FF);

	if (tfc->context->tf_ctrl)
		pthread_spin_lock(&ctrl->tf_ctrl_lock);

	entry->next_free = ctrl->tf_head;
	ctrl->tf_head = tf_idx;
	ctrl->tf_num_avail++;

	if (tfc->context->tf_ctrl)
		pthread_spin_unlock(&ctrl->tf_ctrl_lock);

	tfc->tf_num_inuse--;
	/* If an available callback is registered invoke it */
	if (((tfc->tf_num_inuse + 1) == ctrl->tf_num_max) && tfc->tf_avail_cb)
		tfc->tf_avail_cb(tfc, tfc->tf_avail_context);

	return PSM2_OK;
}

/* Allocate a generation for a flow */
psm2_error_t ips_tfgen_allocate(struct ips_tf *tfc,
			       uint32_t tf_idx, uint32_t *tfgen)
{
	struct ips_tf_entry *entry;
	int ret = PSM2_OK;

	psmi_assert(tf_idx < HFI_TF_NFLOWS);
	psmi_assert(tf_idx >= 0);

	entry = &tfc->tf_ctrl->tf[tf_idx];
	psmi_assert(entry->state == TF_STATE_ALLOCATED);

	*tfgen = entry->next_gen;

	entry->next_gen++;
	if (entry->next_gen == tfc->tf_gen_mask)
		entry->next_gen = 0;

	psmi_assert_always(*tfgen < tfc->tf_gen_mask);

	return ret;
}
