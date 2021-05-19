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

#ifndef __PSM2_HAL_H__

#define __PSM2_HAL_H__

#include "psm_user.h"

/* Forward declaration of PSM structs: */
struct ips_subcontext_ureg;
struct ips_recvhdrq_event;
struct ips_writehdrq;
struct ips_flow;
struct ips_scb;
struct ips_tid_session_list_tag;
struct ips_epinfo;
struct ips_message_header;

/* Declare types: */
typedef enum
{
	PSM_HAL_INSTANCE_ANY_GEN =  0,
	PSM_HAL_INSTANCE_GEN1    =  1,
	PSM_HAL_INSTANCE_GEN2    =  2,
	PSM_HAL_INSTANCE_GEN3    =  3,

	PSM_HAL_INSTANCE_NULL	 = 98,
#ifdef PSM2_MOCK_TESTING
	PSM_HAL_INSTANCE_MOCK    = 99,
#endif
} psmi_hal_instance_type;

typedef enum
{
	/* Operation was successful.  No error occurred. */
	PSM_HAL_ERROR_OK			= 0,
	/* The operation can not be done unless HAL is initialized first. */
	PSM_HAL_ERROR_NOT_INITIALIZED		= 1,
	/* No HAL INSTANCE has been registered.  Initialization is impossible. */
	PSM_HAL_ERROR_NO_HI_REGISTERED		= 2,
	/* Initialization failure. */
	PSM_HAL_ERROR_INIT_FAILED		= 3,
	/* Can't open device file. */
	PSM_HAL_ERROR_CANNOT_OPEN_DEVICE	= 4,
	/* Can't open context. */
	PSM_HAL_ERROR_CANNOT_OPEN_CONTEXT	= 5,
	/* Context is not open. */
	PSM_HAL_ERROR_CONTEXT_IS_NOT_OPEN	= 6,
	/* General error. */
	PSM_HAL_ERROR_GENERAL_ERROR		= 7,
	/* Not implemented. */
	PSM_HAL_ERROR_NOT_IMPLEMENTED		= 8,
	/* Internal error. */
	PSM_HAL_ERROR_INTERNAL_ERROR		= 9,

	/* HAL instances should not return errors less than the value
	   PSM_HAL_ERROR_RESERVED_BY_HAL_API.  These errors are reserved by
	   the HAL API layer. */
	PSM_HAL_ERROR_RESERVED_BY_HAL_API	= 1000,
} psmi_hal_errors;

typedef enum
{
	PSM_HAL_HW_STATUS_INITTED	  = (1UL << 0),
	PSM_HAL_HW_STATUS_CHIP_PRESENT	  = (1UL << 1),
	PSM_HAL_HW_STATUS_IB_READY	  = (1UL << 2),
	PSM_HAL_HW_STATUS_IB_CONF	  = (1UL << 3),
	PSM_HAL_HW_STATUS_HWERROR	  = (1UL << 4)
} psmi_hal_hw_status;

typedef enum
{
	PSM_HAL_HFI_EVENT_FROZEN	  = (1UL << 0),
	PSM_HAL_HFI_EVENT_LINKDOWN	  = (1UL << 1),
	PSM_HAL_HFI_EVENT_LID_CHANGE	  = (1UL << 2),
	PSM_HAL_HFI_EVENT_LMC_CHANGE	  = (1UL << 3),
	PSM_HAL_HFI_EVENT_SL2VL_CHANGE	  = (1UL << 4),
	PSM_HAL_HFI_EVENT_TID_MMU_NOTIFY  = (1UL << 5)
} psmi_hal_hfi_events;

/* The following enum constants correspond to the bits in the
   cap_mask member of the psmi_hal_params_t. */
typedef enum
{
	PSM_HAL_CAP_SDMA			= (1UL <<  0),
	PSM_HAL_CAP_SDMA_AHG			= (1UL <<  1),
	PSM_HAL_CAP_EXTENDED_PSN		= (1UL <<  2),
	PSM_HAL_CAP_HDRSUPP			= (1UL <<  3),
	PSM_HAL_CAP_USE_SDMA_HEAD		= (1UL <<  4),
	PSM_HAL_CAP_MULTI_PKT_EGR		= (1UL <<  5),
	PSM_HAL_CAP_NODROP_RHQ_FULL		= (1UL <<  6),
	PSM_HAL_CAP_NODROP_EGR_FULL		= (1UL <<  7),
	PSM_HAL_CAP_TID_UNMAP			= (1UL <<  8),
	PSM_HAL_CAP_PRINT_UNIMPL		= (1UL <<  9),
	PSM_HAL_CAP_ALLOW_PERM_JKEY		= (1UL << 10),
	PSM_HAL_CAP_NO_INTEGRITY		= (1UL << 11),
	PSM_HAL_CAP_PKEY_CHECK			= (1UL << 12),
	PSM_HAL_CAP_STATIC_RATE_CTRL		= (1UL << 13),
	PSM_HAL_CAP_SDMA_HEAD_CHECK		= (1UL << 14),
	PSM_HAL_CAP_EARLY_CREDIT_RETURN		= (1UL << 15),
	PSM_HAL_CAP_GPUDIRECT_OT		= (1UL << 16),
	PSM_HAL_CAP_DMA_HSUPP_FOR_32B_MSGS	= (1UL << 17),
	PSM_HAL_CAP_RSM_FECN_SUPP		= (1UL << 18),
	PSM_HAL_CAP_MERGED_TID_CTRLS		= (1UL << 19),
	PSM_HAL_CAP_NON_DW_MULTIPLE_MSG_SIZE	= (1UL << 20),
} psmi_hal_capability_bits;

/* The following enum constants correspond to the bits in the
   sw_status member of the psmi_hal_params_t. */
typedef enum
{
	/* Request to start rx thread. */
	PSM_HAL_PSMI_RUNTIME_RTS_RX_THREAD	= (1UL <<  0),
	/* Rx thread is started. */
	PSM_HAL_PSMI_RUNTIME_RX_THREAD_STARTED	= (1UL <<  1),
	PSM_HAL_PSMI_RUNTIME_INTR_ENABLED       = (1UL <<  2),
	/* Header suppression is enabled: */
	PSM_HAL_HDRSUPP_ENABLED                 = (1UL <<  3),
	PSM_HAL_PARAMS_VALID_NUM_UNITS          = (1UL <<  4),
	PSM_HAL_PARAMS_VALID_NUM_PORTS          = (1UL <<  5),
	PSM_HAL_PARAMS_VALID_DEFAULT_PKEY       = (1UL <<  6),

} psmi_hal_sw_status;

/* The _psmi_hal_params structure stores values that remain constant for the entire life of
   the process and this structure resides in the hal instance structure (below).
   The values are settled after the context is opened. */
typedef struct _psmi_hal_params
{
	uint32_t   cap_mask;
	uint32_t   sw_status;
	/* start cached members */
	uint16_t   num_units;
	uint16_t   num_ports;
	uint16_t   default_pkey;
	int8_t     *unit_active,*unit_active_valid;
	int8_t     *port_active,*port_active_valid;
	uint16_t   *num_contexts,*num_contexts_valid;
	uint16_t   *num_free_contexts,*num_free_contexts_valid;
} psmi_hal_params_t;

/* HAL assumes that the rx hdr q and the egr buff q are circular lists
 with two important indexes:

 head - software takes from this side of the circular list
 tail - hardware deposits new content here

The indexes advance in the list 0, 1, 2, 3, ... until they reach the value:
(number_of_entries_in_the_q-1), then the next value they take is 0.  And,
so, that is why these are called circular lists.

When the head idx == tail idx, that represents an empty circular list.

A completely full circular list is when:

    head_idx == (tail_idx + 1) % number_of_entries_in_the_q

Both indexes will always be in the range: 0 <= index < number_of_entries_in_the_q

After software receives the packet in the slot corresponding to the head idx,
and processes it completely, software will signal to the hardware that the slot
is available for re-use by retiring it - see api below for details.

Note that these are simplified assumptions for the benefit of the hardware independent
layer of PSM.  The actual implementation details are hidden in the hal instances.

Note that subcontexts have a collection of head / tail indexes for their use.

So, HAL supports the use of the following circular lists dealing with the
following entities:

1. Rx Hdr q - corresponding to hardware (software modifies head index, hardware modifies tail index).
2. Rx egr q - corresponding to hardware (software modifies head index, hardware modifies tail index).
3. Rx Hdr q - corresponding to a subcontext (software modifies both head and tail indexes).
4. Rx egr q - corresponding to a subcontext (software modifies both head and tail indexes).

Declare a type to indicate a circular list index:
*/
typedef uint32_t psmi_hal_cl_idx;

typedef enum
{
	PSM_HAL_CL_Q_RX_HDR_Q      =  0, /* HW context for the rx hdr q. */
	PSM_HAL_CL_Q_RX_EGR_Q      =  1, /* HW context for the rx eager q. */
	/* Start of subcontexts (This is subcontext 0) */
	PSM_HAL_CL_Q_RX_HDR_Q_SC_0 =  2, /* Subcontext 0's rx hdr q. */
	PSM_HAL_CL_Q_RX_EGR_Q_SC_0 =  3, /* Subcontext 0's rx eager q. */

	/* Following SC 0's CL_Q's are the circular list q for subcontexts 1-7,
	   two per subcontext.  Even values are the rx hdr q for the subcontext
	   Odd value are for the eager q. */

/* Given a subcontext number (0-7), return the CL_Q for the RX HDR_Q: */
#define PSM_HAL_GET_SC_CL_Q_RX_HDR_Q(SC) ((SC)*2 + PSM_HAL_CL_Q_RX_HDR_Q_SC_0)
/* Given a subcontext number (0-7), return the CL_Q for the RX EGR_Q: */
#define PSM_HAL_GET_SC_CL_Q_RX_EGR_Q(SC) ((SC)*2 + PSM_HAL_CL_Q_RX_EGR_Q_SC_0)
} psmi_hal_cl_q;

#define PSM_HAL_MAX_SHARED_CTXTS 8

#define PSM_HAL_ALG_ACROSS     0
#define PSM_HAL_ALG_WITHIN     1
#define PSM_HAL_ALG_ACROSS_ALL 2

typedef enum
{
	PSM_HAL_EXP   = 0,
	PSM_HAL_EGR   = 1,
} psmi_hal_set_sdma_req_type;

#define PSM_HAL_SDMA_REQ_VERSION_MASK 0xF
#define PSM_HAL_SDMA_REQ_VERSION_SHIFT 0x0
#define PSM_HAL_SDMA_REQ_OPCODE_MASK 0xF
#define PSM_HAL_SDMA_REQ_OPCODE_SHIFT 0x4
#define PSM_HAL_SDMA_REQ_IOVCNT_MASK 0xFF
#define PSM_HAL_SDMA_REQ_IOVCNT_SHIFT 0x8

#ifdef PSM_CUDA
#define PSM_HAL_BUF_GPU_MEM  1
#endif

struct psm_hal_sdma_req_info {
	/*
	 * bits 0-3 - version (currently used only for GPU direct)
	 *               1 - user space is NOT using flags field
	 *               2 - user space is using flags field
	 * bits 4-7 - opcode (enum sdma_req_opcode)
	 * bits 8-15 - io vector count
	 */
	__u16 ctrl;
	/*
	 * Number of fragments contained in this request.
	 * User-space has already computed how many
	 * fragment-sized packet the user buffer will be
	 * split into.
	 */
	__u16 npkts;
	/*
	 * Size of each fragment the user buffer will be
	 * split into.
	 */
	__u16 fragsize;
	/*
	 * Index of the slot in the SDMA completion ring
	 * this request should be using. User-space is
	 * in charge of managing its own ring.
	 */
	__u16 comp_idx;
#ifdef PSM_CUDA
	/*
	 * Buffer flags for this request. See HFI1_BUF_*
	 */
	__u16 flags;
	/* The extra bytes for the PSM_CUDA version of the sdma req info
	 * struct is the size of the flags member. */
#define PSM_HAL_CUDA_SDMA_REQ_INFO_EXTRA sizeof(__u16)
#endif
} __attribute__((packed));


typedef enum {
	PSM_HAL_SDMA_RING_AVAILABLE = 0,
	PSM_HAL_SDMA_RING_QUEUED    = 1,
	PSM_HAL_SDMA_RING_COMPLETE  = 2,
	PSM_HAL_SDMA_RING_ERROR     = 3,
} psmi_hal_sdma_ring_slot_status;

typedef uint64_t psmi_hal_raw_rhf_t;

typedef struct psmi_hal_rhf_
{
	/* The first entity in rhf is the decomposed rhf.
	   Each HAL instance, in hfp_get_receive_event(), will decompose the raw rhf
	   obtained from the hardware and deposit the data into this common
	   decomposed rhf, so the upper layers of psm can find the data in one
	   uniform place. */

	uint64_t decomposed_rhf;

	/* The second entry is the raw rhf that comes from the h/w.
	   The upper layers of psm should not use the raw rhf, instead use the
	   decomposed rhf above.  The raw rhf is intended for use by the HAL
	   instance only. */
	uint64_t raw_rhf;
} psmi_hal_rhf_t;

#define PSMI_HAL_RHF_ERR_ICRC_NBITS       1
#define PSMI_HAL_RHF_ERR_ICRC_SHFTC      63
#define PSMI_HAL_RHF_ERR_RSRV_NBITS       1
#define PSMI_HAL_RHF_ERR_RSRV_SHFTC      62
#define PSMI_HAL_RHF_ERR_ECC_NBITS        1
#define PSMI_HAL_RHF_ERR_ECC_SHFTC       61
#define PSMI_HAL_RHF_ERR_LEN_NBITS        1
#define PSMI_HAL_RHF_ERR_LEN_SHFTC       60
#define PSMI_HAL_RHF_ERR_TID_NBITS        1
#define PSMI_HAL_RHF_ERR_TID_SHFTC       59
#define PSMI_HAL_RHF_ERR_TFGEN_NBITS      1
#define PSMI_HAL_RHF_ERR_TFGEN_SHFTC     58
#define PSMI_HAL_RHF_ERR_TFSEQ_NBITS      1
#define PSMI_HAL_RHF_ERR_TFSEQ_SHFTC     57
#define PSMI_HAL_RHF_ERR_RTE_NBITS        3
#define PSMI_HAL_RHF_ERR_RTE_SHFTC       56
#define PSMI_HAL_RHF_ERR_DC_NBITS         1
#define PSMI_HAL_RHF_ERR_DC_SHFTC        55
#define PSMI_HAL_RHF_ERR_DCUN_NBITS       1
#define PSMI_HAL_RHF_ERR_DCUN_SHFTC      54
#define PSMI_HAL_RHF_ERR_KHDRLEN_NBITS    1
#define PSMI_HAL_RHF_ERR_KHDRLEN_SHFTC   53
#define PSMI_HAL_RHF_ALL_ERR_FLAGS_NBITS (PSMI_HAL_RHF_ERR_ICRC_NBITS + PSMI_HAL_RHF_ERR_RSRV_NBITS		\
					  	+ PSMI_HAL_RHF_ERR_ECC_NBITS					\
						+ PSMI_HAL_RHF_ERR_LEN_NBITS + PSMI_HAL_RHF_ERR_TID_NBITS	\
						+ PSMI_HAL_RHF_ERR_TFGEN_NBITS + PSMI_HAL_RHF_ERR_TFSEQ_NBITS	\
						+ PSMI_HAL_RHF_ERR_RTE_NBITS + PSMI_HAL_RHF_ERR_DC_NBITS	\
						+ PSMI_HAL_RHF_ERR_DCUN_NBITS + PSMI_HAL_RHF_ERR_KHDRLEN_NBITS)
#define PSMI_HAL_RHF_ALL_ERR_FLAGS_SHFTC 53
#define PSMI_HAL_RHF_EGR_BUFF_OFF_NBITS  12
#define PSMI_HAL_RHF_EGR_BUFF_OFF_SHFTC  32
#define PSMI_HAL_RHF_SEQ_NBITS		  4
#define PSMI_HAL_RHF_SEQ_SHFTC		 28
#define PSMI_HAL_RHF_EGR_BUFF_IDX_NBITS  11
#define PSMI_HAL_RHF_EGR_BUFF_IDX_SHFTC  16
#define PSMI_HAL_RHF_USE_EGR_BUFF_NBITS   1
#define PSMI_HAL_RHF_USE_EGR_BUFF_SHFTC  15
#define PSMI_HAL_RHF_RX_TYPE_NBITS        3
#define PSMI_HAL_RHF_RX_TYPE_SHFTC       12
#define PSMI_HAL_RHF_PKT_LEN_NBITS       12
#define PSMI_HAL_RHF_PKT_LEN_SHFTC        0

typedef enum {
	PSM_HAL_RHF_RX_TYPE_EXPECTED = 0,
	PSM_HAL_RHF_RX_TYPE_EAGER    = 1,
	PSM_HAL_RHF_RX_TYPE_NON_KD   = 2,
	PSM_HAL_RHF_RX_TYPE_ERROR    = 3
} psmi_hal_rhf_rx_type;

struct psm_hal_pbc {
	__u32 pbc0;
	__u16 PbcStaticRateControlCnt;
	__u16 fill1;
};

typedef enum {
	PSMI_HAL_POLL_TYPE_URGENT = 1
} psmi_hal_poll_type;

/* Forward declaration of incomplete struct type _psmi_hal_instance and
 * psmi_hal_instance_t typedef: */

struct _psmi_hal_instance;
typedef struct _psmi_hal_instance psmi_hal_instance_t;

struct _psmi_hal_instance
{
	SLIST_ENTRY(_psmi_hal_instance) next_hi;
	psmi_hal_instance_type		type;
	const char			*description;
	const char			*hfi_name;
	const char			*hfi_sys_class_path;
	/* The params member should be read-only for HIC, and
	   written only by the HAL instance. */
	psmi_hal_params_t		params;
	/* Initialize the HAL INSTANCE. */
	int (*hfp_initialize)(psmi_hal_instance_t *);
	/* Finalize the HAL INSTANCE. */
	int (*hfp_finalize_)(void);

	/* Returns the number of hfi units installed on ths host:
	   NOTE: hfp_get_num_units is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_num_units)(void);

	/* Returns the number of ports on each hfi unit installed.
	   on ths host.
	   NOTE: hfp_get_num_ports is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_num_ports)(void);

	/* Returns the default pkey:
	   NOTE: hfp_get_default_pkey is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_default_pkey)(void);

	/* Given a unit number, returns 1 if any port on the unit is active.
	   returns 0 if no port on the unit is active.
	   returns -1 when an error occurred.
	   NOTE: hfp_get_unit_active is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_unit_active)(int unit);

	int (*hfp_get_port_active)(int unit,int port);
	/* NOTE: hfp_get_num_contexts is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_num_contexts)(int unit);
	/* NOTE: hfp_get_num_free_contexts is a function that must
	   be callable before the hal instance is initialized. */
	int (*hfp_get_num_free_contexts)(int unit);

	/* Context open includes opening the device file, and get hw params. */
	int (*hfp_context_open)(int unit,
				int port,
				uint64_t open_timeout,
				psm2_ep_t ep,
				psm2_uuid_t const job_key,
				psmi_context_t *psm_ctxt,
				uint32_t cap_mask,
				unsigned retryCnt);

	/* Close the context, including the device file. */
	int (*hfp_close_context)(psmi_hal_hw_context *);

	/* Given a unit, port and index, return an error, or the corresponding pkey for
	   the index as programmed by the SM */
	/* Returns an int, so -1 indicates an error. */
	int (*hfp_get_port_index2pkey)(int unit, int port, int index);
	int (*hfp_get_cc_settings_bin)(int unit, int port, char *ccabuf, size_t len_ccabuf);
	int (*hfp_get_cc_table_bin)(int unit, int port, uint16_t **cctp);
	int (*hfp_get_port_lmc)(int unit, int port);
	int (*hfp_get_port_rate)(int unit, int port);
	int (*hfp_get_port_sl2sc)(int unit, int port,int sl);
	int (*hfp_get_sc2vl_map)(struct ips_proto *proto);
	int (*hfp_set_pkey)(psmi_hal_hw_context, uint16_t);
	int (*hfp_poll_type)(uint16_t poll_type, psmi_hal_hw_context);
	int (*hfp_get_port_lid)(int unit, int port);
	int (*hfp_get_port_gid)(int unit, int port, uint64_t *hi, uint64_t *lo);
	int (*hfp_free_tid)(psmi_hal_hw_context, uint64_t tidlist, uint32_t tidcnt);
	int (*hfp_get_tidcache_invalidation)(psmi_hal_hw_context, uint64_t tidlist, uint32_t *tidcnt);
	int (*hfp_update_tid)(psmi_hal_hw_context, uint64_t vaddr, uint32_t *length,
			      uint64_t tidlist, uint32_t *tidcnt,
			      uint16_t flags);
	/* Initiate a DMA.  Intrinsically specifies a DMA slot to use. */
	int (*hfp_writev)(const struct iovec *iov, int iovcnt, struct ips_epinfo *, psmi_hal_hw_context);
	/* Updates PSM from h/w on DMA completions: */
	int (*hfp_get_sdma_ring_slot_status)(int slotIdx, psmi_hal_sdma_ring_slot_status *, uint32_t *errorCode, psmi_hal_hw_context);
	/* Returns > 0 if the specified slots is available.  0 if not available
	   and a negative value if an error occurred. */
	int (*hfp_dma_slot_available)(int slotidx, psmi_hal_hw_context);

	/* Start of receive packet functions. */

	/* Getter for cl q head indexes: */
	psmi_hal_cl_idx (*hfp_get_cl_q_head_index)(psmi_hal_cl_q,
						   psmi_hal_hw_context);

	/* Getter for cl q tail indexes: */
	psmi_hal_cl_idx (*hfp_get_cl_q_tail_index)(psmi_hal_cl_q,
						   psmi_hal_hw_context);

	/* Setter for cl q head indexes: */
	void (*hfp_set_cl_q_head_index)(psmi_hal_cl_idx,
				        psmi_hal_cl_q,
					psmi_hal_hw_context);

	/* Setter for cl q tail indexes: */
	void (*hfp_set_cl_q_tail_index)(psmi_hal_cl_idx,
					psmi_hal_cl_q,
					psmi_hal_hw_context);

	/* Indicate whether the cl q is empty.
	   When this returns > 0 the cl q is empty.
	   When this returns == 0, the cl q is NOT empty (there are packets in the
	   circular list that are available to receive).
	   When this returns < 0, an error occurred.
	   the parameter should correspond to the head index of the
	   cl q circular list. */
	int (*hfp_cl_q_empty)(psmi_hal_cl_idx head_idx,
			      psmi_hal_cl_q,
			      psmi_hal_hw_context);

	/* Receive the raw rhf, decompose it, and then receive the ips_message_hdr. */
	int (*hfp_get_receive_event)(psmi_hal_cl_idx head_idx, psmi_hal_hw_context,
				     struct ips_recvhdrq_event *);

	/* Deliver an eager buffer given the index.
	   If the index does not refer to a current egr buffer, hfp_get_egr_buff() returns
	   NULL. */
	void *(*hfp_get_egr_buff)(psmi_hal_cl_idx, psmi_hal_cl_q, psmi_hal_hw_context);

	/* Retire the given head idx of the header q, and change *head_idx to point to the next
	      entry, lastly set *empty to indicate whether the headerq is empty at the new
	      head_idx. */
	int (*hfp_retire_hdr_q_entry)(psmi_hal_cl_idx *head_idx, psmi_hal_cl_q, psmi_hal_hw_context,
				     uint32_t elemsz, uint32_t elemlast,
				     int *emptyp);

	/* Returns expected sequence number for RHF. */
	int (*hfp_get_rhf_expected_sequence_number)(unsigned int *, psmi_hal_cl_q, psmi_hal_hw_context);

	/* Sets expected sequence number for RHF. */
	int (*hfp_set_rhf_expected_sequence_number)(unsigned int, psmi_hal_cl_q, psmi_hal_hw_context);

	/* Checks sequence number from RHF. Returns PSM_HAL_ERROR_OK if the sequence number is good
	   returns something else if the sequence number is bad. */
	int (*hfp_check_rhf_sequence_number)(unsigned int);

	/* Set PBC struct that lies within the extended memory region of SCB */
	int (*hfp_set_pbc)(struct ips_proto *proto, struct ips_flow *flow,
			   uint32_t isCtrlMsg, struct psm_hal_pbc *dest, uint32_t hdrlen,
			   uint32_t paylen);


	/* Start of tid flow functions. */
	int (*hfp_set_tf_valid)(uint32_t, psmi_hal_hw_context);

	int (*hfp_tidflow_set_entry)(uint32_t flowid, uint32_t genval,
				     uint32_t seqnum, psmi_hal_hw_context);

	int (*hfp_tidflow_reset)(psmi_hal_hw_context, uint32_t flowid, uint32_t genval,
				 uint32_t seqnum);

	int (*hfp_tidflow_get)(uint32_t flowid, uint64_t *ptf, psmi_hal_hw_context);

	/* hfp_tidflow_get_hw is identical to hfp_tidflow_get(), but guarantees to get
	   its information fron h/w, and not from cached values, but may be significantly
	   slower than hfp_tidflow_get(), so should be used for debug only. */
	int (*hfp_tidflow_get_hw)(uint32_t flowid, uint64_t *ptf, psmi_hal_hw_context);

	int (*hfp_tidflow_get_seqnum)(uint64_t val, uint32_t *pseqn);

	int (*hfp_tidflow_get_genval)(uint64_t val, uint32_t *pgv);

	int (*hfp_tidflow_check_update_pkt_seq)(void *vpprotoexp
						/* actually a:
						   struct ips_protoexp *protoexp */,
						psmi_seqnum_t sequence_num,
						void *vptidrecvc
						/* actually a:
						   struct ips_tid_recv_desc *tidrecvc */,
						struct ips_message_header *p_hdr,
						void (*ips_protoexp_do_tf_generr)
						(void *vpprotoexp
						 /* actually a:
						    struct ips_protoexp *protoexp */,
						 void *vptidrecvc
						 /* actually a:
						    struct ips_tid_recv_desc *tidrecvc */,
						 struct ips_message_header *p_hdr),
						void (*ips_protoexp_do_tf_seqerr)
						(void *vpprotoexp
						 /* actually a:
						    struct ips_protoexp *protoexp */,
						 void *vptidrecvc
						 /* actually a:
						    struct ips_tid_recv_desc *tidrecvc */,
						 struct ips_message_header *p_hdr)
		);

	int (*hfp_tidflow_get_flowvalid)(uint64_t val, uint32_t *pfv);

	int (*hfp_tidflow_get_enabled)(uint64_t val, uint32_t *penabled);

	int (*hfp_tidflow_get_keep_after_seqerr)(uint64_t val, uint32_t *pkase);

	int (*hfp_tidflow_get_keep_on_generr)(uint64_t val, uint32_t *pkoge);

	int (*hfp_tidflow_get_keep_payload_on_generr)(uint64_t val, uint32_t *pkpoge);

	/* For hfp_tidflow_get_seqmismatch and hfp_tidflow_get_genmismatch, if
	   val was obtained from hfp_tidflow_get_hw(), then these will be valid
	   but, if val was obtained from hfp_tidflow_get(), then these will
	   always return 0. */
	int (*hfp_tidflow_get_seqmismatch)(uint64_t val, uint32_t *psmm);

	int (*hfp_tidflow_get_genmismatch)(uint64_t val, uint32_t *pgmm);

	/* End of tid flow functions. */

	/* End of receive functions. */

	int (*hfp_forward_packet_to_subcontext)(struct ips_writehdrq *writeq,
						struct ips_recvhdrq_event *rcv_ev,
						uint32_t subcontext,
						psmi_hal_hw_context);
	int (*hfp_subcontext_ureg_get)(ptl_t *ptl,
				       struct ips_subcontext_ureg **uregp,
				       psmi_hal_hw_context);

	int (*hfp_get_hfi_event_bits) (uint64_t *event_bits, psmi_hal_hw_context);

	int (*hfp_ack_hfi_event) (uint64_t ack_bits, psmi_hal_hw_context);

	int (*hfp_hfi_reset_context) (psmi_hal_hw_context);

	uint64_t (*hfp_get_hw_status) (psmi_hal_hw_context);

	int (*hfp_get_hw_status_freezemsg) (volatile char** msg, psmi_hal_hw_context);

	uint16_t (*hfp_get_user_major_bldtime_version) (void);

	uint16_t (*hfp_get_user_minor_bldtime_version) (void);

	uint16_t (*hfp_get_user_major_runtime_version) (psmi_hal_hw_context);

	uint16_t (*hfp_get_user_minor_runtime_version) (psmi_hal_hw_context);

	int (*hfp_set_pio_size)(uint32_t, psmi_hal_hw_context);

	int (*hfp_set_effective_mtu)(uint32_t, psmi_hal_hw_context);

	int (*hfp_spio_init)(const psmi_context_t *context,
				struct ptl *ptl, void **ctrl);
	int (*hfp_spio_fini)(void **ctrl, psmi_hal_hw_context);

	int (*hfp_spio_transfer_frame)(struct ips_proto *proto,
				       struct ips_flow *flow, struct psm_hal_pbc *pbc,
				       uint32_t *payload, uint32_t length,
				       uint32_t isCtrlMsg, uint32_t cksum_valid,
				       uint32_t cksum, psmi_hal_hw_context
#ifdef PSM_CUDA
				, uint32_t is_cuda_payload
#endif
		);
	int (*hfp_spio_process_events)(const struct ptl *ptl);
	int (*hfp_get_node_id)(int unit, int *nodep);

	int      (*hfp_get_bthqp)(psmi_hal_hw_context);
	int      (*hfp_get_context)(psmi_hal_hw_context);
	uint64_t (*hfp_get_gid_lo)(psmi_hal_hw_context);
	uint64_t (*hfp_get_gid_hi)(psmi_hal_hw_context);
	int      (*hfp_get_hfi_type)(psmi_hal_hw_context);
	int      (*hfp_get_jkey)(psmi_hal_hw_context);
	int      (*hfp_get_lid)(psmi_hal_hw_context);
	int      (*hfp_get_pio_size)(psmi_hal_hw_context);
	int      (*hfp_get_port_num)(psmi_hal_hw_context);
	int      (*hfp_get_rx_egr_tid_cnt)(psmi_hal_hw_context);
	int      (*hfp_get_rx_hdr_q_cnt)(psmi_hal_hw_context);
	int      (*hfp_get_rx_hdr_q_ent_size)(psmi_hal_hw_context);
	int      (*hfp_get_sdma_req_size)(psmi_hal_hw_context);
	int      (*hfp_get_sdma_ring_size)(psmi_hal_hw_context);
	int      (*hfp_get_subctxt)(psmi_hal_hw_context);
	int      (*hfp_get_subctxt_cnt)(psmi_hal_hw_context);
	int      (*hfp_get_tid_exp_cnt)(psmi_hal_hw_context);
	int      (*hfp_get_unit_id)(psmi_hal_hw_context);
	int      (*hfp_get_fd)(psmi_hal_hw_context);
	int      (*hfp_get_pio_stall_cnt)(psmi_hal_hw_context, uint64_t **);
};

/* This is the current psmi_hal_instance, or, NULL if not initialized.
   The HIC should not modify the contents of the HAL instance directly. */
extern psmi_hal_instance_t *psmi_hal_current_hal_instance;

/* Declare functions called by the HAL INSTANCES. */
void psmi_hal_register_instance(psmi_hal_instance_t *);

/* Declare functions that are called by the HIC: */
/* All of these functions return a negative int value to
   indicate failure, or >= 0 for success. */

/* Chooses one of the the psmi_hal_instances that have been
    registered and then initializes it.
    Returns: -PSM_HAL_ERROR_NOT_REGISTERED_HI if no HAL
    INSTANCES are registered, or PSM_HAL_ERROR_INIT_FAILED when
    another failure has occured during initialization. */
int psmi_hal_initialize(void);
void psmi_hal_initialize_null(void);

int psmi_hal_finalize(void);

#include "psm2_hal_inlines_d.h"

enum psmi_hal_pre_init_cache_func_krnls
{
	psmi_hal_pre_init_cache_func_get_num_units,
	psmi_hal_pre_init_cache_func_get_num_ports,
	psmi_hal_pre_init_cache_func_get_unit_active,
	psmi_hal_pre_init_cache_func_get_port_active,
	psmi_hal_pre_init_cache_func_get_num_contexts,
	psmi_hal_pre_init_cache_func_get_num_free_contexts,
	psmi_hal_pre_init_cache_func_get_default_pkey,
};

int psmi_hal_pre_init_cache_func(enum psmi_hal_pre_init_cache_func_krnls k, ...);

#define PSMI_HAL_DISPATCH_PI(KERNEL,...) ( psmi_hal_pre_init_cache_func(psmi_hal_pre_init_cache_func_ ## KERNEL , ##__VA_ARGS__ ) )

#if PSMI_HAL_INST_CNT == 1

#define PSMI_HAL_DISPATCH(KERNEL,...)    ( PSMI_HAL_CAT_INL_SYM(KERNEL) ( __VA_ARGS__ ) )

#else

#define PSMI_HAL_DISPATCH(KERNEL,...)    ( psmi_hal_current_hal_instance->hfp_ ## KERNEL ( __VA_ARGS__ ))

#endif

#define psmi_hal_get_num_units_(...)                           PSMI_HAL_DISPATCH_PI(get_num_units,##__VA_ARGS__)
#define psmi_hal_get_num_ports_(...)                           PSMI_HAL_DISPATCH_PI(get_num_ports,##__VA_ARGS__)
#define psmi_hal_get_unit_active(...)                          PSMI_HAL_DISPATCH_PI(get_unit_active,__VA_ARGS__)
#define psmi_hal_get_port_active(...)                          PSMI_HAL_DISPATCH_PI(get_port_active,__VA_ARGS__)
#define psmi_hal_get_num_contexts(...)                         PSMI_HAL_DISPATCH_PI(get_num_contexts,__VA_ARGS__)
#define psmi_hal_get_num_free_contexts(...)                    PSMI_HAL_DISPATCH_PI(get_num_free_contexts,__VA_ARGS__)
#define psmi_hal_get_default_pkey(...)			       PSMI_HAL_DISPATCH_PI(get_default_pkey,##__VA_ARGS__)
#define psmi_hal_context_open(...)				PSMI_HAL_DISPATCH(context_open,__VA_ARGS__)
#define psmi_hal_close_context(...)				PSMI_HAL_DISPATCH(close_context,__VA_ARGS__)
#define psmi_hal_get_port_index2pkey(...)			PSMI_HAL_DISPATCH(get_port_index2pkey,__VA_ARGS__)
#define psmi_hal_get_cc_settings_bin(...)			PSMI_HAL_DISPATCH(get_cc_settings_bin,__VA_ARGS__)
#define psmi_hal_get_cc_table_bin(...)				PSMI_HAL_DISPATCH(get_cc_table_bin,__VA_ARGS__)
#define psmi_hal_get_port_lmc(...)				PSMI_HAL_DISPATCH(get_port_lmc,__VA_ARGS__)
#define psmi_hal_get_port_rate(...)				PSMI_HAL_DISPATCH(get_port_rate,__VA_ARGS__)
#define psmi_hal_get_port_sl2sc(...)				PSMI_HAL_DISPATCH(get_port_sl2sc,__VA_ARGS__)
#define psmi_hal_get_sc2vl_map(...)				PSMI_HAL_DISPATCH(get_sc2vl_map, __VA_ARGS__)
#define psmi_hal_set_pkey(...)					PSMI_HAL_DISPATCH(set_pkey,__VA_ARGS__)
#define psmi_hal_poll_type(...)					PSMI_HAL_DISPATCH(poll_type,__VA_ARGS__)
#define psmi_hal_get_port_lid(...)				PSMI_HAL_DISPATCH(get_port_lid,__VA_ARGS__)
#define psmi_hal_get_port_gid(...)				PSMI_HAL_DISPATCH(get_port_gid,__VA_ARGS__)
#define psmi_hal_free_tid(...)					PSMI_HAL_DISPATCH(free_tid,__VA_ARGS__)
#define psmi_hal_get_tidcache_invalidation(...)			PSMI_HAL_DISPATCH(get_tidcache_invalidation,__VA_ARGS__)
#define psmi_hal_update_tid(...)				PSMI_HAL_DISPATCH(update_tid,__VA_ARGS__)
#define psmi_hal_writev(...)					PSMI_HAL_DISPATCH(writev,__VA_ARGS__)
#define psmi_hal_dma_slot_available(...)			PSMI_HAL_DISPATCH(dma_slot_available,__VA_ARGS__)
#define psmi_hal_get_sdma_ring_slot_status(...)			PSMI_HAL_DISPATCH(get_sdma_ring_slot_status,__VA_ARGS__)
#define psmi_hal_get_cl_q_head_index(...)			PSMI_HAL_DISPATCH(get_cl_q_head_index,__VA_ARGS__)
#define psmi_hal_get_cl_q_tail_index(...)			PSMI_HAL_DISPATCH(get_cl_q_tail_index,__VA_ARGS__)
#define psmi_hal_set_cl_q_head_index(...)			PSMI_HAL_DISPATCH(set_cl_q_head_index,__VA_ARGS__)
#define psmi_hal_set_cl_q_tail_index(...)			PSMI_HAL_DISPATCH(set_cl_q_tail_index,__VA_ARGS__)
#define psmi_hal_cl_q_empty(...)				PSMI_HAL_DISPATCH(cl_q_empty,__VA_ARGS__)
#define psmi_hal_get_receive_event(...)				PSMI_HAL_DISPATCH(get_receive_event,__VA_ARGS__)
#define psmi_hal_get_egr_buff(...)				PSMI_HAL_DISPATCH(get_egr_buff,__VA_ARGS__)
#define psmi_hal_retire_hdr_q_entry(...)			PSMI_HAL_DISPATCH(retire_hdr_q_entry,__VA_ARGS__)
#define psmi_hal_get_rhf_expected_sequence_number(...)		PSMI_HAL_DISPATCH(get_rhf_expected_sequence_number,__VA_ARGS__)
#define psmi_hal_set_rhf_expected_sequence_number(...)		PSMI_HAL_DISPATCH(set_rhf_expected_sequence_number,__VA_ARGS__)
#define psmi_hal_check_rhf_sequence_number(...)			PSMI_HAL_DISPATCH(check_rhf_sequence_number,__VA_ARGS__)
#define psmi_hal_set_pbc(...)					PSMI_HAL_DISPATCH(set_pbc,__VA_ARGS__)
#define psmi_hal_tidflow_set_entry(...)				PSMI_HAL_DISPATCH(tidflow_set_entry,__VA_ARGS__)
#define psmi_hal_tidflow_reset(...)				PSMI_HAL_DISPATCH(tidflow_reset,__VA_ARGS__)
#define psmi_hal_tidflow_get(...)				PSMI_HAL_DISPATCH(tidflow_get,__VA_ARGS__)
#define psmi_hal_tidflow_get_hw(...)				PSMI_HAL_DISPATCH(tidflow_get_hw,__VA_ARGS__)
#define psmi_hal_tidflow_get_seqnum(...)			PSMI_HAL_DISPATCH(tidflow_get_seqnum,__VA_ARGS__)
#define psmi_hal_tidflow_get_genval(...)			PSMI_HAL_DISPATCH(tidflow_get_genval,__VA_ARGS__)
#define psmi_hal_tidflow_check_update_pkt_seq(...)		PSMI_HAL_DISPATCH(tidflow_check_update_pkt_seq,__VA_ARGS__)
#define psmi_hal_tidflow_get_flowvalid(...)			PSMI_HAL_DISPATCH(tidflow_get_flowvalid,__VA_ARGS__)
#define psmi_hal_tidflow_get_enabled(...)			PSMI_HAL_DISPATCH(tidflow_get_enabled,__VA_ARGS__)
#define psmi_hal_tidflow_get_keep_after_seqerr(...)		PSMI_HAL_DISPATCH(tidflow_get_keep_after_seqerr,__VA_ARGS__)
#define psmi_hal_tidflow_get_keep_on_generr(...)		PSMI_HAL_DISPATCH(tidflow_get_keep_on_generr,__VA_ARGS__)
#define psmi_hal_tidflow_get_keep_payload_on_generr(...)	PSMI_HAL_DISPATCH(tidflow_get_keep_payload_on_generr,__VA_ARGS__)
#define psmi_hal_tidflow_get_seqmismatch(...)			PSMI_HAL_DISPATCH(tidflow_get_seqmismatch,__VA_ARGS__)
#define psmi_hal_tidflow_get_genmismatch(...)			PSMI_HAL_DISPATCH(tidflow_get_genmismatch,__VA_ARGS__)
#define psmi_hal_forward_packet_to_subcontext(...)		PSMI_HAL_DISPATCH(forward_packet_to_subcontext,__VA_ARGS__)
#define psmi_hal_subcontext_ureg_get(...)			PSMI_HAL_DISPATCH(subcontext_ureg_get,__VA_ARGS__)
#define psmi_hal_finalize_(...)                                 PSMI_HAL_DISPATCH(finalize_,__VA_ARGS__)
#define psmi_hal_get_hfi_event_bits(...)			PSMI_HAL_DISPATCH(get_hfi_event_bits,__VA_ARGS__)
#define psmi_hal_ack_hfi_event(...)				PSMI_HAL_DISPATCH(ack_hfi_event,__VA_ARGS__)
#define psmi_hal_hfi_reset_context(...)				PSMI_HAL_DISPATCH(hfi_reset_context,__VA_ARGS__)
#define psmi_hal_get_hw_status(...)				PSMI_HAL_DISPATCH(get_hw_status,__VA_ARGS__)
#define psmi_hal_get_hw_status_freezemsg(...)			PSMI_HAL_DISPATCH(get_hw_status_freezemsg,__VA_ARGS__)
#define psmi_hal_get_user_major_bldtime_version(...)		PSMI_HAL_DISPATCH(get_user_major_bldtime_version,__VA_ARGS__)
#define psmi_hal_get_user_minor_bldtime_version(...)		PSMI_HAL_DISPATCH(get_user_minor_bldtime_version,__VA_ARGS__)
#define psmi_hal_get_user_major_runtime_version(...)		PSMI_HAL_DISPATCH(get_user_major_runtime_version,__VA_ARGS__)
#define psmi_hal_get_user_minor_runtime_version(...)		PSMI_HAL_DISPATCH(get_user_minor_runtime_version,__VA_ARGS__)
#define psmi_hal_set_pio_size(...)				PSMI_HAL_DISPATCH(set_pio_size,__VA_ARGS__)
#define psmi_hal_set_effective_mtu(...)				PSMI_HAL_DISPATCH(set_effective_mtu,__VA_ARGS__)
#define psmi_hal_set_tf_valid(...)				PSMI_HAL_DISPATCH(set_tf_valid,__VA_ARGS__)
#define psmi_hal_spio_init(...)					PSMI_HAL_DISPATCH(spio_init,__VA_ARGS__)
#define psmi_hal_spio_fini(...)					PSMI_HAL_DISPATCH(spio_fini,__VA_ARGS__)
#define psmi_hal_spio_transfer_frame(...)			PSMI_HAL_DISPATCH(spio_transfer_frame,__VA_ARGS__)
#define psmi_hal_spio_process_events(...)			PSMI_HAL_DISPATCH(spio_process_events,__VA_ARGS__)
#define psmi_hal_get_node_id(...)				PSMI_HAL_DISPATCH(get_node_id,__VA_ARGS__)
#define psmi_hal_get_bthqp(...)					PSMI_HAL_DISPATCH(get_bthqp,__VA_ARGS__)
#define psmi_hal_get_context(...)				PSMI_HAL_DISPATCH(get_context,__VA_ARGS__)
#define psmi_hal_get_gid_lo(...)				PSMI_HAL_DISPATCH(get_gid_lo,__VA_ARGS__)
#define psmi_hal_get_gid_hi(...)				PSMI_HAL_DISPATCH(get_gid_hi,__VA_ARGS__)
#define psmi_hal_get_hfi_type(...)				PSMI_HAL_DISPATCH(get_hfi_type,__VA_ARGS__)
#define psmi_hal_get_jkey(...)					PSMI_HAL_DISPATCH(get_jkey,__VA_ARGS__)
#define psmi_hal_get_lid(...)					PSMI_HAL_DISPATCH(get_lid,__VA_ARGS__)
#define psmi_hal_get_pio_size(...)				PSMI_HAL_DISPATCH(get_pio_size,__VA_ARGS__)
#define psmi_hal_get_port_num(...)				PSMI_HAL_DISPATCH(get_port_num,__VA_ARGS__)
#define psmi_hal_get_rx_egr_tid_cnt(...)			PSMI_HAL_DISPATCH(get_rx_egr_tid_cnt,__VA_ARGS__)
#define psmi_hal_get_rx_hdr_q_cnt(...)				PSMI_HAL_DISPATCH(get_rx_hdr_q_cnt,__VA_ARGS__)
#define psmi_hal_get_rx_hdr_q_ent_size(...)			PSMI_HAL_DISPATCH(get_rx_hdr_q_ent_size,__VA_ARGS__)
#define psmi_hal_get_sdma_req_size(...)				PSMI_HAL_DISPATCH(get_sdma_req_size,__VA_ARGS__)
#define psmi_hal_get_sdma_ring_size(...)			PSMI_HAL_DISPATCH(get_sdma_ring_size,__VA_ARGS__)
#define psmi_hal_get_subctxt(...)				PSMI_HAL_DISPATCH(get_subctxt,__VA_ARGS__)
#define psmi_hal_get_subctxt_cnt(...)				PSMI_HAL_DISPATCH(get_subctxt_cnt,__VA_ARGS__)
#define psmi_hal_get_tid_exp_cnt(...)				PSMI_HAL_DISPATCH(get_tid_exp_cnt,__VA_ARGS__)
#define psmi_hal_get_unit_id(...)				PSMI_HAL_DISPATCH(get_unit_id,__VA_ARGS__)
#define psmi_hal_get_fd(...)					PSMI_HAL_DISPATCH(get_fd,__VA_ARGS__)
#define psmi_hal_get_pio_stall_cnt(...)                         PSMI_HAL_DISPATCH(get_pio_stall_cnt,__VA_ARGS__)

#define PSMI_HAL_NBITS_TO_MASK(NBITS)				((uint64_t)((1 << NBITS)-1))
#define PSMI_HAL_RHF_UNPACK(A,NAME)				((uint32_t)((A.decomposed_rhf >>	\
									PSMI_HAL_RHF_ ## NAME ## _SHFTC	\
									) &  PSMI_HAL_NBITS_TO_MASK(	\
									 PSMI_HAL_RHF_ ## NAME ## _NBITS)))
/* define constants for the decomposed rhf error masks.
   Note how each of these are shifted by the ALL_ERR_FLAGS shift count. */

#define PSMI_HAL_RHF_ERR_MASK_64(NAME)				((uint64_t)(((PSMI_HAL_NBITS_TO_MASK( \
									PSMI_HAL_RHF_ERR_ ## NAME ## _NBITS) << \
									PSMI_HAL_RHF_ERR_ ## NAME ## _SHFTC ))))
#define PSMI_HAL_RHF_ERR_MASK_32(NAME)				((uint32_t)(PSMI_HAL_RHF_ERR_MASK_64(NAME) >> \
									   PSMI_HAL_RHF_ALL_ERR_FLAGS_SHFTC))
#define PSMI_HAL_RHF_ERR_ICRC					PSMI_HAL_RHF_ERR_MASK_32(ICRC)
#define PSMI_HAL_RHF_ERR_ECC					PSMI_HAL_RHF_ERR_MASK_32(ECC)
#define PSMI_HAL_RHF_ERR_LEN					PSMI_HAL_RHF_ERR_MASK_32(LEN)
#define PSMI_HAL_RHF_ERR_TID					PSMI_HAL_RHF_ERR_MASK_32(TID)
#define PSMI_HAL_RHF_ERR_TFGEN					PSMI_HAL_RHF_ERR_MASK_32(TFGEN)
#define PSMI_HAL_RHF_ERR_TFSEQ					PSMI_HAL_RHF_ERR_MASK_32(TFSEQ)
#define PSMI_HAL_RHF_ERR_RTE					PSMI_HAL_RHF_ERR_MASK_32(RTE)
#define PSMI_HAL_RHF_ERR_DC					PSMI_HAL_RHF_ERR_MASK_32(DC)
#define PSMI_HAL_RHF_ERR_DCUN					PSMI_HAL_RHF_ERR_MASK_32(DCUN)
#define PSMI_HAL_RHF_ERR_KHDRLEN				PSMI_HAL_RHF_ERR_MASK_32(KHDRLEN)

#define psmi_hal_rhf_get_use_egr_buff(A)			PSMI_HAL_RHF_UNPACK(A,USE_EGR_BUFF)
#define psmi_hal_rhf_get_egr_buff_index(A)			PSMI_HAL_RHF_UNPACK(A,EGR_BUFF_IDX)
#define psmi_hal_rhf_get_egr_buff_offset(A)			PSMI_HAL_RHF_UNPACK(A,EGR_BUFF_OFF)
#define psmi_hal_rhf_get_packet_length(A)			(PSMI_HAL_RHF_UNPACK(A,PKT_LEN)<<2)
#define psmi_hal_rhf_get_all_err_flags(A)			PSMI_HAL_RHF_UNPACK(A,ALL_ERR_FLAGS)
#define psmi_hal_rhf_get_seq(A)					PSMI_HAL_RHF_UNPACK(A,SEQ)
#define psmi_hal_rhf_get_rx_type(A)				PSMI_HAL_RHF_UNPACK(A,RX_TYPE)
#define PSMI_HAL_RHF_PACK(NAME,VALUE)				((uint64_t)((((uint64_t)(VALUE)) & \
									PSMI_HAL_NBITS_TO_MASK( \
									PSMI_HAL_RHF_ ## NAME ## _NBITS \
									)) << ( \
									PSMI_HAL_RHF_ ## NAME ## _SHFTC )))

#define psmi_hal_get_hal_instance_type()			psmi_hal_current_hal_instance->type
#define psmi_hal_get_hal_instance_description()			psmi_hal_current_hal_instance->description
#define psmi_hal_get_hfi_name()					psmi_hal_current_hal_instance->hfi_name
#define psmi_hal_get_num_units()				psmi_hal_current_hal_instance->params.num_units
#define psmi_hal_get_num_ports()				psmi_hal_current_hal_instance->params.num_ports
#define psmi_hal_get_cap_mask()					psmi_hal_current_hal_instance->params.cap_mask
#define psmi_hal_set_cap_mask(NEW_MASK)				(psmi_hal_current_hal_instance->params.cap_mask = (NEW_MASK))
#define psmi_hal_add_cap(CAP)					(psmi_hal_current_hal_instance->params.cap_mask |= (CAP))
#define psmi_hal_sub_cap(CAP)					(psmi_hal_current_hal_instance->params.cap_mask &= (~(CAP)))
#define psmi_hal_has_cap(CAP)                                   ((psmi_hal_get_cap_mask() & (CAP)) == (CAP))

#define psmi_hal_get_sw_status()				psmi_hal_current_hal_instance->params.sw_status
#define psmi_hal_set_sw_status(NEW_STATUS)			(psmi_hal_current_hal_instance->params.sw_status = (NEW_STATUS))
#define psmi_hal_add_sw_status(STATUS)				(psmi_hal_current_hal_instance->params.sw_status |= (STATUS))
#define psmi_hal_sub_sw_status(STATUS)				(psmi_hal_current_hal_instance->params.sw_status &= (~(STATUS)))
#define psmi_hal_has_sw_status(STATUS)				((psmi_hal_get_sw_status() & (STATUS)) == (STATUS))


#include "psm2_hal_inlines_i.h"

#endif /* #ifndef __PSM2_HAL_H__ */
