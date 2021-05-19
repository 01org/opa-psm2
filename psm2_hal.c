/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2017 Intel Corporation.

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

  Copyright(c) 2017 Intel Corporation.

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

#include "psm_user.h"
#include "psm2_hal.h"

#include "ptl_ips/ips_scb.h"

static SLIST_HEAD(, _psmi_hal_instance) head_hi;

/* define the current hal instance pointer */
psmi_hal_instance_t *psmi_hal_current_hal_instance = NULL;

/* psmi_hal_register_instance */
void psmi_hal_register_instance(psmi_hal_instance_t *psm_hi)
{
#define REJECT_IMPROPER_HI(MEMBER) if (!psm_hi->MEMBER) return

	/* If an attempt to register a hal instance contains a NULL func ptr, reject it. */
	/* To allow fast lookups, please keep this code segment alphabetized by hfp_*
	   func ptr member name: */
#if PSMI_HAL_INST_CNT > 1
	REJECT_IMPROPER_HI(hfp_ack_hfi_event);
	REJECT_IMPROPER_HI(hfp_check_rhf_sequence_number);
	REJECT_IMPROPER_HI(hfp_cl_q_empty);
	REJECT_IMPROPER_HI(hfp_close_context);
	REJECT_IMPROPER_HI(hfp_context_open);
	REJECT_IMPROPER_HI(hfp_dma_slot_available);
	REJECT_IMPROPER_HI(hfp_finalize_);
	REJECT_IMPROPER_HI(hfp_forward_packet_to_subcontext);
	REJECT_IMPROPER_HI(hfp_free_tid);
	REJECT_IMPROPER_HI(hfp_get_bthqp);
	REJECT_IMPROPER_HI(hfp_get_cc_settings_bin);
	REJECT_IMPROPER_HI(hfp_get_cc_table_bin);
	REJECT_IMPROPER_HI(hfp_get_cl_q_head_index);
	REJECT_IMPROPER_HI(hfp_get_cl_q_tail_index);
	REJECT_IMPROPER_HI(hfp_get_context);
	REJECT_IMPROPER_HI(hfp_get_egr_buff);
	REJECT_IMPROPER_HI(hfp_get_fd);
	REJECT_IMPROPER_HI(hfp_get_gid_hi);
	REJECT_IMPROPER_HI(hfp_get_gid_lo);
	REJECT_IMPROPER_HI(hfp_get_hfi_event_bits);
	REJECT_IMPROPER_HI(hfp_get_hfi_type);
	REJECT_IMPROPER_HI(hfp_get_hw_status);
	REJECT_IMPROPER_HI(hfp_get_hw_status_freezemsg);
	REJECT_IMPROPER_HI(hfp_get_jkey);
	REJECT_IMPROPER_HI(hfp_get_lid);
	REJECT_IMPROPER_HI(hfp_get_node_id);
	REJECT_IMPROPER_HI(hfp_get_num_contexts);
	REJECT_IMPROPER_HI(hfp_get_num_free_contexts);
	REJECT_IMPROPER_HI(hfp_get_pio_size);
	REJECT_IMPROPER_HI(hfp_get_pio_stall_cnt);
	REJECT_IMPROPER_HI(hfp_get_port_active);
	REJECT_IMPROPER_HI(hfp_get_port_gid);
	REJECT_IMPROPER_HI(hfp_get_port_index2pkey);
	REJECT_IMPROPER_HI(hfp_get_port_lid);
	REJECT_IMPROPER_HI(hfp_get_port_lmc);
	REJECT_IMPROPER_HI(hfp_get_port_num);
	REJECT_IMPROPER_HI(hfp_get_port_rate);
	REJECT_IMPROPER_HI(hfp_get_sc2vl_map);
	REJECT_IMPROPER_HI(hfp_get_port_sl2sc);
	REJECT_IMPROPER_HI(hfp_get_receive_event);
	REJECT_IMPROPER_HI(hfp_get_rhf_expected_sequence_number);
	REJECT_IMPROPER_HI(hfp_get_rx_egr_tid_cnt);
	REJECT_IMPROPER_HI(hfp_get_rx_hdr_q_cnt);
	REJECT_IMPROPER_HI(hfp_get_rx_hdr_q_ent_size);
	REJECT_IMPROPER_HI(hfp_get_sdma_req_size);
	REJECT_IMPROPER_HI(hfp_get_sdma_ring_size);
	REJECT_IMPROPER_HI(hfp_get_sdma_ring_slot_status);
	REJECT_IMPROPER_HI(hfp_get_subctxt);
	REJECT_IMPROPER_HI(hfp_get_subctxt_cnt);
	REJECT_IMPROPER_HI(hfp_get_tid_exp_cnt);
	REJECT_IMPROPER_HI(hfp_get_tidcache_invalidation);
	REJECT_IMPROPER_HI(hfp_get_unit_active);
	REJECT_IMPROPER_HI(hfp_get_unit_id);
	REJECT_IMPROPER_HI(hfp_get_user_major_bldtime_version);
	REJECT_IMPROPER_HI(hfp_get_user_major_runtime_version);
	REJECT_IMPROPER_HI(hfp_get_user_minor_bldtime_version);
	REJECT_IMPROPER_HI(hfp_get_user_minor_runtime_version);
	REJECT_IMPROPER_HI(hfp_hfi_reset_context);
	REJECT_IMPROPER_HI(hfp_poll_type);
	REJECT_IMPROPER_HI(hfp_retire_hdr_q_entry);
	REJECT_IMPROPER_HI(hfp_set_cl_q_head_index);
	REJECT_IMPROPER_HI(hfp_set_cl_q_tail_index);
	REJECT_IMPROPER_HI(hfp_set_effective_mtu);
	REJECT_IMPROPER_HI(hfp_set_pbc);
	REJECT_IMPROPER_HI(hfp_set_pio_size);
	REJECT_IMPROPER_HI(hfp_set_pkey);
	REJECT_IMPROPER_HI(hfp_set_rhf_expected_sequence_number);
	REJECT_IMPROPER_HI(hfp_set_tf_valid);
	REJECT_IMPROPER_HI(hfp_spio_fini);
	REJECT_IMPROPER_HI(hfp_spio_init);
	REJECT_IMPROPER_HI(hfp_spio_process_events);
	REJECT_IMPROPER_HI(hfp_spio_transfer_frame);
	REJECT_IMPROPER_HI(hfp_subcontext_ureg_get);
	REJECT_IMPROPER_HI(hfp_tidflow_check_update_pkt_seq);
	REJECT_IMPROPER_HI(hfp_tidflow_get);
	REJECT_IMPROPER_HI(hfp_tidflow_get_enabled);
	REJECT_IMPROPER_HI(hfp_tidflow_get_flowvalid);
	REJECT_IMPROPER_HI(hfp_tidflow_get_genmismatch);
	REJECT_IMPROPER_HI(hfp_tidflow_get_genval);
	REJECT_IMPROPER_HI(hfp_tidflow_get_hw);
	REJECT_IMPROPER_HI(hfp_tidflow_get_keep_after_seqerr);
	REJECT_IMPROPER_HI(hfp_tidflow_get_keep_on_generr);
	REJECT_IMPROPER_HI(hfp_tidflow_get_keep_payload_on_generr);
	REJECT_IMPROPER_HI(hfp_tidflow_get_seqmismatch);
	REJECT_IMPROPER_HI(hfp_tidflow_get_seqnum);
	REJECT_IMPROPER_HI(hfp_tidflow_reset);
	REJECT_IMPROPER_HI(hfp_tidflow_set_entry);
	REJECT_IMPROPER_HI(hfp_update_tid);
	REJECT_IMPROPER_HI(hfp_writev);
#endif
	REJECT_IMPROPER_HI(hfp_get_default_pkey);
	REJECT_IMPROPER_HI(hfp_get_num_ports);
	REJECT_IMPROPER_HI(hfp_get_num_units);
	REJECT_IMPROPER_HI(hfp_initialize);

#ifndef PSM2_MOCK_TESTING
	if (!sysfs_init(psm_hi->hfi_sys_class_path))
#endif
		SLIST_INSERT_HEAD(&head_hi, psm_hi, next_hi);
}

static struct _psmi_hal_instance *psmi_hal_get_pi_inst(void);

int psmi_hal_pre_init_cache_func(enum psmi_hal_pre_init_cache_func_krnls k, ...)
{
	va_list ap;
	va_start(ap, k);

	int rv = 0;
	struct _psmi_hal_instance *p = psmi_hal_get_pi_inst();

	if (!p)
		rv = -1;
	else
	{
		switch(k)
		{
		case psmi_hal_pre_init_cache_func_get_num_units:
			rv = p->params.num_units;
			break;
		case psmi_hal_pre_init_cache_func_get_num_ports:
			rv = p->params.num_ports;
			break;
		case psmi_hal_pre_init_cache_func_get_unit_active:
			{
				int unit = va_arg(ap,int);

				if ((unit >= 0) && (unit < p->params.num_units))
				{
					if (!p->params.unit_active_valid[unit]) {
						p->params.unit_active_valid[unit] = 1;
						p->params.unit_active[unit] = p->hfp_get_unit_active(unit);
					}
					rv = p->params.unit_active[unit];
				}
				else
					rv = -1;
			}
			break;
		case psmi_hal_pre_init_cache_func_get_port_active:
			{
				int unit = va_arg(ap,int);

				if ((unit >= 0) && (unit < p->params.num_units))
				{
					int port = va_arg(ap,int);
					if ((port >= 1) && (port <= p->params.num_ports))
					{
						if (!p->params.port_active_valid[unit*port]) {
							p->params.port_active_valid[unit*port] = 1;
							p->params.port_active[unit*port] = p->hfp_get_port_active(unit,port);
						}
						rv = p->params.port_active[unit*port];
					}
					else
						rv = -1;
				}
				else
					rv = -1;
			}
			break;
		case psmi_hal_pre_init_cache_func_get_num_contexts:
			{
				int unit = va_arg(ap,int);
				if ((unit >= 0) && (unit < p->params.num_units))
				{
					if (!p->params.num_contexts_valid[unit]) {
						p->params.num_contexts_valid[unit] = 1;
						p->params.num_contexts[unit] = p->hfp_get_num_contexts(unit);
					}
					rv = p->params.num_contexts[unit];
				}
				else
					rv = -1;
			}
			break;
		case psmi_hal_pre_init_cache_func_get_num_free_contexts:
			{
				int unit = va_arg(ap,int);

				if ((unit >= 0) && (unit < p->params.num_units))
				{
					if (!p->params.num_free_contexts_valid[unit]) {
						p->params.num_free_contexts_valid[unit] = 1;
						p->params.num_free_contexts[unit] = p->hfp_get_num_free_contexts(unit);
					}
					rv = p->params.num_free_contexts[unit];
				}
				else
					rv = -1;
			}
			break;
		case psmi_hal_pre_init_cache_func_get_default_pkey:
			rv = p->params.default_pkey;
			break;
		default:
			rv = -1;
			break;
		}
	}

	va_end(ap);
	return rv;
}

static struct _psmi_hal_instance *psmi_hal_get_pi_inst(void)
{

	if (psmi_hal_current_hal_instance)
		return psmi_hal_current_hal_instance;

	if (SLIST_EMPTY(&head_hi))
		return NULL;

	/* At this point, assuming there are multiple HAL INSTANCES that are
	   registered, and two or more of the HAL INSTANCES are capable
	   of initialization on a host, the environment variable PSM2_HAL_PREF
	   allows the user to identify the one HAL INSTANCE that is desired to
	   be used. The default policy is, when the PSM2_HAL_PREF is not set, the
	   first hal instance that successfully initializes is used. */

	union psmi_envvar_val env_hi_pref; /* HAL instance preference */
	psmi_getenv("PSM2_HAL_PREF",
		    "Indicate preference for HAL instance (Default is use first HAL"
		    " instance to successfully initialize))",
		    PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_INT,
		    (union psmi_envvar_val)PSM_HAL_INSTANCE_ANY_GEN, &env_hi_pref);

	/* The hfp_get_num_units() call below, will not wait for the HFI driver
	   to come up and create device nodes in /dev/.) */
	struct _psmi_hal_instance *p;
	SLIST_FOREACH(p, &head_hi, next_hi)
	{
		if ((env_hi_pref.e_int == PSM_HAL_INSTANCE_ANY_GEN) ||
		    (p->type == env_hi_pref.e_int))
		{
			const int valid_flags = PSM_HAL_PARAMS_VALID_DEFAULT_PKEY |
				PSM_HAL_PARAMS_VALID_NUM_UNITS |
				PSM_HAL_PARAMS_VALID_NUM_PORTS;

			if ((p->params.sw_status & valid_flags) == valid_flags)
				return p;

			int nunits = p->hfp_get_num_units();
			int nports = p->hfp_get_num_ports();
			int dflt_pkey = p->hfp_get_default_pkey();
			if (nunits > 0 && nports > 0 && dflt_pkey > 0
#ifndef PSM2_MOCK_TESTING
			    && (0 == sysfs_init(p->hfi_sys_class_path))
#endif
				)
			{
				p->params.num_units = nunits;
				p->params.num_ports = nports;
				p->params.default_pkey = dflt_pkey;
				p->params.sw_status |= valid_flags;
				p->params.unit_active = (int8_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits, sizeof(int8_t));
				p->params.unit_active_valid = (int8_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits, sizeof(int8_t));
				p->params.port_active = (int8_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits*nports, sizeof(int8_t));
				p->params.port_active_valid = (int8_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits*nports, sizeof(int8_t));
				p->params.num_contexts = (uint16_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits,
										  sizeof(uint16_t));
				p->params.num_contexts_valid = (uint16_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits,
											sizeof(uint16_t));
				p->params.num_free_contexts = (uint16_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits,
										       sizeof(uint16_t));
				p->params.num_free_contexts_valid = (uint16_t *) psmi_calloc(PSMI_EP_NONE, UNDEFINED, nunits,
											     sizeof(uint16_t));
				return p;
			}
		}
	}
	return NULL;
}

/* psmi_hal_initialize */
int psmi_hal_initialize(void)
{
	struct _psmi_hal_instance *p = psmi_hal_get_pi_inst();

	if (!p)
		return -PSM_HAL_ERROR_INIT_FAILED;

	int rv = p->hfp_initialize(p);

	if (!rv)
	{
		psmi_hal_current_hal_instance = p;

		if (psmi_hal_has_cap(PSM_HAL_CAP_HDRSUPP)) {
			union psmi_envvar_val env_hdrsupp;

			psmi_getenv("PSM2_HDRSUPP",
				    "Receive header suppression. Default is 1 (enabled),"
				    	" 0 to disable.\n",
				    PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_UINT_FLAGS,
				    (union psmi_envvar_val)1, &env_hdrsupp);
			if (env_hdrsupp.e_uint)
				psmi_hal_add_sw_status(PSM_HAL_HDRSUPP_ENABLED);
			else
				/* user wants to disable header suppression */
				psmi_hal_set_tf_valid(0, p);
		}

		return rv;
	}
	return -PSM_HAL_ERROR_INIT_FAILED;
}

/* psmi_hal_initialize_null */
void psmi_hal_initialize_null(void)
{
	static struct _psmi_hal_instance nullhal = {
		.type = PSM_HAL_INSTANCE_NULL,
		.description = "NULL HAL (hardware disabled)",
		.hfi_name = "null",
		.hfi_sys_class_path = "/dev/null",
		.params = {0}
	};

	psmi_hal_current_hal_instance = &nullhal;
}

int psmi_hal_finalize(void)
{
	struct _psmi_hal_instance *p = psmi_hal_current_hal_instance;

	int rv = psmi_hal_finalize_();

	psmi_free(p->params.unit_active);
	psmi_free(p->params.unit_active_valid);
	psmi_free(p->params.port_active);
	psmi_free(p->params.port_active_valid);
	psmi_free(p->params.num_contexts);
	psmi_free(p->params.num_contexts_valid);
	psmi_free(p->params.num_free_contexts);
	psmi_free(p->params.num_free_contexts_valid);
	p->params.unit_active = NULL;
	p->params.unit_active_valid = NULL;
	p->params.port_active = NULL;
	p->params.port_active_valid = NULL;
	p->params.num_contexts = NULL;
	p->params.num_contexts_valid = NULL;
	p->params.num_free_contexts = NULL;
	p->params.num_free_contexts_valid = NULL;
	psmi_hal_current_hal_instance = NULL;
	sysfs_fini();
	return rv;
}


#ifdef PSM2_MOCK_TESTING

#include "psm_hal_gen1/opa_user_gen1.h"

void ips_ptl_non_dw_mul_sdma_init(void)
{
	uint16_t major_version = hfi_get_user_major_version();
	uint16_t minor_version = hfi_get_user_minor_version();
	int allow_non_dw_mul = 0;

	if ((major_version > HFI1_USER_SWMAJOR_NON_DW_MUL_MSG_SIZE_ALLOWED) ||
		((major_version == HFI1_USER_SWMAJOR_NON_DW_MUL_MSG_SIZE_ALLOWED) &&
		 (minor_version >= HFI1_USER_SWMINOR_NON_DW_MUL_MSG_SIZE_ALLOWED)))
	{
		allow_non_dw_mul = 1;
	}
	psmi_hal_current_hal_instance->params.cap_mask = 0;
	if (allow_non_dw_mul)
		psmi_hal_current_hal_instance->params.cap_mask |= PSM_HAL_CAP_NON_DW_MULTIPLE_MSG_SIZE;
}

void set_sdma_ring_size_in_MOCK_HAL_instance(int sdma_ring_size)
{
	extern int __psm_hal_mock_sdma_ring_size;

	__psm_hal_mock_sdma_ring_size = sdma_ring_size;
}

void set_comp_entry(struct hfi1_sdma_comp_entry *pce)
{
	extern struct hfi1_sdma_comp_entry * __psm_hal_mock_hfi1_sdma_comp_entry;

	__psm_hal_mock_hfi1_sdma_comp_entry = pce;
}

#endif
