/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2015 Intel Corporation.

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

  Copyright(c) 2015 Intel Corporation.

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

/* Copyright (c) 2003-2015 Intel Corporation. All rights reserved. */

#ifndef _PSMI_IN_USER_H
#error psm2_ep.h not meant to be included directly, include psm_user.h instead
#endif

#ifndef _PSMI_EP_H
#define _PSMI_EP_H

/*
 * EPIDs encode the following information:
 *
 * LID:16 bits - LID for endpoint
 * CONTEXT:8 bits - Context used for bits (upto 256 contexts)
 * SUBCONTEXT:3 bits - Subcontext used for endpoint
 * HFIUNIT: 2 bits - HFI unit number
 * HFITYPE: 3 bits - OPA1, OPA2, ...
 * RANK: 26 bits - process rank
 * reserved: 6 bit - for future usage
 */

#define PSMI_HFI_TYPE_UNKNOWN 0
#define PSMI_HFI_TYPE_OPA1    1
#define PSMI_HFI_TYPE_OPA2    2

#define PSMI_SL_DEFAULT 0
#define PSMI_SC_DEFAULT 0
#define PSMI_VL_DEFAULT 0
#define PSMI_SL_MIN	0
#define PSMI_SL_MAX	31
#define PSMI_SC_ADMIN	15
#define PSMI_VL_ADMIN	15
#define PSMI_SC_NBITS   5  /* Number of bits in SC */
#define PSMI_N_SCS       (1 << PSMI_SC_NBITS)  /* The number of SC's */

#define PSMI_EPID_PACK_V1(lid, context, subcontext, hfiunit, epid_version, rank) \
	(((((uint64_t)lid)&0xffff)<<16)			|								\
	 ((((uint64_t)context)&0xff)<<8)		|			  					\
	 ((((uint64_t)subcontext)&0x7)<<5)		|								\
	 ((((uint64_t)hfiunit)&0x3)<<3)			|								\
	 ((((uint64_t)epid_version)&0x7)<<0)	|								\
	 ((((uint64_t)rank)&0x3ffffff)<<32))

#define PSMI_EPID_PACK_V2(lid, context, subcontext, shmbool, epid_version, subnet_id) \
	(((((uint64_t)lid)&0xffffff)<<16)			|								\
	 ((((uint64_t)context)&0xff)<<8)		|			  					\
	 ((((uint64_t)subcontext)&0x7)<<5)		|								\
	 ((((uint64_t)shmbool)&0x1)<<3)			|								\
	 ((((uint64_t)epid_version)&0x7)<<0)	|								\
	 ((((uint64_t)subnet_id)&0xffff)<<48))

#define PSMI_EPID_PACK_V2_SHM(process_id, shmbool, epid_version) \
	(((((uint64_t)process_id)&0xffffffff)<<32)			|								\
	 ((((uint64_t)shmbool)&0x1)<<3)		|			  					\
	 ((((uint64_t)epid_version)&0x7)<<0))

#define PSMI_EPID_GET_LID_V1(epid)			(((epid)>>16)&0xffff)
#define PSMI_EPID_GET_LID_V2(epid)			(((epid)>>16)&0xffffff)
#define PSMI_EPID_GET_CONTEXT(epid)			(((epid)>>8)&0xff)
#define PSMI_EPID_GET_SUBCONTEXT(epid)		(((epid)>>5)&0x7)
#define PSMI_EPID_GET_HFIUNIT(epid)			(((epid)>>3)&0x3)
#define PSMI_EPID_GET_EPID_VERSION(epid)	(((epid)>>0)&0x7)
#define PSMI_EPID_GET_RANK(epid)			(((epid)>>32)&0x3ffffff)
#define PSMI_EPID_GET_SHMBOOL(epid)			(((epid)>>3)&0x1)
#define PSMI_EPID_GET_SUBNET_ID(epid)		(((epid)>>48)&0xffff)
#define PSMI_EPID_GET_PROCESS_ID(epid)		(((epid)>>32)&0xffffffff)

#define PSM_MCTXT_APPEND(head, node)	\
	node->mctxt_prev = head->mctxt_prev; \
	node->mctxt_next = head; \
	head->mctxt_prev->mctxt_next = node; \
	head->mctxt_prev = node; \
	node->mctxt_master = head
#define PSM_MCTXT_REMOVE(node)	\
	node->mctxt_prev->mctxt_next = node->mctxt_next; \
	node->mctxt_next->mctxt_prev = node->mctxt_prev; \
	node->mctxt_next = node->mctxt_prev = node; \
	node->mctxt_master = NULL

struct psm2_ep {
	psm2_epid_t epid;	    /**> This endpoint's Endpoint ID */
	psm2_epaddr_t epaddr;	    /**> This ep's ep address */
	psm2_mq_t mq;		    /**> only 1 MQ */
	int unit_id;
	uint16_t portnum;
	uint16_t out_sl;
	uint16_t mtu;		/* out_sl-->vl-->mtu in sysfs */
	uint16_t network_pkey;	      /**> OPA Pkey */
	int did_syslog;
	psm2_uuid_t uuid;
	uint16_t jkey;
	uint64_t service_id;	/* OPA service ID */
	psm2_path_res_t path_res_type;	/* Path resolution for endpoint */
	psm2_ep_errhandler_t errh;
	int devid_enabled[PTL_MAX_INIT];
	int memmode;		    /**> min, normal, large memory mode */

	uint32_t hfi_num_sendbufs;/**> Number of allocated send buffers */
	uint32_t hfi_num_descriptors;/** Number of allocated scb descriptors*/
	uint32_t hfi_imm_size;	  /** Immediate data size */
	uint32_t connections;	    /**> Number of connections */

	psmi_context_t context;
	char *context_mylabel;
	uint32_t yield_spin_cnt;

	/* EP link-lists */
	struct psm2_ep *user_ep_next;

	/* EP link-lists for multi-context. */
	struct psm2_ep *mctxt_prev;
	struct psm2_ep *mctxt_next;
	struct psm2_ep *mctxt_master;

	/* Active Message handler table */
	struct psm2_ep_am_handle_entry *am_htable;

	uint64_t gid_hi;
	uint64_t gid_lo;

	ptl_ctl_t ptl_amsh;
	ptl_ctl_t ptl_ips;
	ptl_ctl_t ptl_self;

	/* All ptl data is allocated inline below */
	uint8_t ptl_base_data[0] __attribute__ ((aligned(64)));
	bool skip_affinity;
};

struct mqq {
	psm2_mq_req_t first;
	psm2_mq_req_t last;
};

typedef
union psmi_seqnum {
	struct {
		uint32_t psn_seq:11;
		uint32_t psn_gen:20;
	};
	struct {
		uint32_t psn_num:31;
	};
	uint32_t psn_val;
} psmi_seqnum_t;

/*
 * PSM end point address. One per connection and per rail.
 */
struct psm2_epaddr {
	psm2_epid_t epid;	/* peer's epid */
	ptl_ctl_t *ptlctl;	/* The control structure for the ptl */
	struct ips_proto *proto;	/* only for ips protocol */
	void *usr_ep_ctxt;	/* User context associated with endpoint */
};

#ifndef PSMI_BLOCKUNTIL_POLLS_BEFORE_YIELD
#  define PSMI_BLOCKUNTIL_POLLS_BEFORE_YIELD  250
#endif

/*
 * Users of BLOCKUNTIL should check the value of err upon return
 */
#define PSMI_BLOCKUNTIL(ep, err, cond)	do {				\
	int spin_cnt = 0;						\
	PSMI_PROFILE_BLOCK();						\
	while (!(cond)) {						\
		err = psmi_poll_internal(ep, 1);			\
		if (err == PSM2_OK_NO_PROGRESS) {			\
			PSMI_PROFILE_REBLOCK(1);			\
			if (++spin_cnt == (ep)->yield_spin_cnt) {	\
				spin_cnt = 0;				\
				PSMI_YIELD((ep)->mq->progress_lock);	\
			}						\
		}							\
		else if (err == PSM2_OK) {				\
			PSMI_PROFILE_REBLOCK(0);			\
			spin_cnt = 0;					\
		}							\
		else							\
		break;							\
	}								\
	PSMI_PROFILE_UNBLOCK();						\
} while (0)


psm2_error_t psmi_parse_devices(int devices[PTL_MAX_INIT],
				      const char *devstr);
int psmi_device_is_enabled(const int devices[PTL_MAX_INIT], int devid);

#endif /* _PSMI_EP_H */
