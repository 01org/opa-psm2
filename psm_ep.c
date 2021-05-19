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

#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>		/* cpu_set */
#include <ctype.h>		/* isalpha */
#include <stdbool.h>

#include "psm_user.h"
#include "psm2_hal.h"
#include "psm_mq_internal.h"
#include "psm_am_internal.h"

#ifdef PSM_CUDA
#include "psm_gdrcpy.h"
#endif
/*
 * Endpoint management
 */
psm2_ep_t psmi_opened_endpoint = NULL;
int psmi_opened_endpoint_count = 0;
static uint16_t *hfi_lids;
static uint32_t nlids;

static psm2_error_t psmi_ep_open_device(const psm2_ep_t ep,
				       const struct psm2_ep_open_opts *opts,
				       const psm2_uuid_t unique_job_key,
				       struct psmi_context *context,
				       psm2_epid_t *epid);

/*
 * Device management
 *
 * PSM uses "devices" as components to manage communication to self, to peers
 * reachable via shared memory and finally to peers reachable only through
 * hfi.
 */

int psmi_ep_device_is_enabled(const psm2_ep_t ep, int devid);

psm2_error_t __psm2_ep_num_devunits(uint32_t *num_units_o)
{
	static int num_units = -1;

	PSM2_LOG_MSG("entering");

	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	if (num_units == -1) {
		num_units = psmi_hal_get_num_units();
		if (num_units == -1)
			num_units = 0;
	}

	*num_units_o = (uint32_t) num_units;
	PSM2_LOG_MSG("leaving");
	return PSM2_OK;
}
PSMI_API_DECL(psm2_ep_num_devunits)

static int cmpfunc(const void *p1, const void *p2)
{
	uint64_t a = ((uint64_t *) p1)[0];
	uint64_t b = ((uint64_t *) p2)[0];
	if (a < b)
		return -1;
	if (a == b)
		return 0;
	return 1;
}

static psm2_error_t
psmi_ep_multirail(int *num_rails, uint32_t *unit, uint16_t *port)
{
	uint32_t num_units;
	uint64_t gid_hi, gid_lo;
	int i, j, ret, count = 0;
	char *env;
	psm2_error_t err = PSM2_OK;
	uint64_t gidh[HFI_MAX_RAILS][3];
	union psmi_envvar_val env_multirail;
	int multirail_within_socket_used = 0;
	int node_id = -1, found = 0;

	psmi_getenv("PSM2_MULTIRAIL",
			"Use all available HFIs in the system for communication.\n"
			 "0: Disabled (default),\n"
			 "1: Enable multirail across all available HFIs,\n"
			 "2: Enable multirail within socket.\n"
			 "\t For multirail within a socket, we try to find at\n"
			 "\t least one HFI on the same socket as current task.\n"
			 "\t If none found, we continue to use other HFIs within\n"
			 "\t the system.",
			PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_INT,
			(union psmi_envvar_val)0,
			&env_multirail);
	if (!env_multirail.e_int) {
		*num_rails = 0;
		return err;
	}

	if (env_multirail.e_int == 2)
		multirail_within_socket_used = 1;

/*
 * map is in format: unit:port,unit:port,...
 */
	if ((env = getenv("PSM2_MULTIRAIL_MAP"))) {
		if (sscanf(env, "%d:%d", &i, &j) == 2) {
			char *comma = strchr(env, ',');
			unit[count] = i;
			port[count] = j;
			count++;
			while (comma) {
				if (sscanf(comma, ",%d:%d", &i, &j) != 2) {
					break;
				}
				unit[count] = i;
				port[count] = j;
				count++;
				if (count == HFI_MAX_RAILS)
					break;
				comma = strchr(comma + 1, ',');
			}
		}
		*num_rails = count;

/*
 * Check if any of the port is not usable.
 */
		for (i = 0; i < count; i++) {
			ret = psmi_hal_get_port_active(unit[i], port[i]);
			if (ret <= 0) {
				err =
				    psmi_handle_error(NULL,
						      PSM2_EP_DEVICE_FAILURE,
						      "Unit/port: %d:%d is not active.",
						      unit[i], port[i]);
				return err;
			}
			ret = psmi_hal_get_port_lid(unit[i], port[i]);
			if (ret <= 0) {
				err =
				    psmi_handle_error(NULL,
						      PSM2_EP_DEVICE_FAILURE,
						      "Couldn't get lid for unit %d:%d",
						      unit[i], port[i]);
				return err;
			}
			ret =
			    psmi_hal_get_port_gid(unit[i], port[i], &gid_hi,
					     &gid_lo);
			if (ret == -1) {
				err =
				    psmi_handle_error(NULL,
						      PSM2_EP_DEVICE_FAILURE,
						      "Couldn't get gid for unit %d:%d",
						      unit[i], port[i]);
				return err;
			}
		}

		return err;
	}

	if ((err = psm2_ep_num_devunits(&num_units))) {
		return err;
	}
	if (num_units > HFI_MAX_RAILS) {
		_HFI_INFO
		    ("Found %d units, max %d units are supported, use %d\n",
		     num_units, HFI_MAX_RAILS, HFI_MAX_RAILS);
		num_units = HFI_MAX_RAILS;
	}

	/*
	 * PSM2_MULTIRAIL=2 functionality-
	 *   - Try to find at least find one HFI in the same root
	 *     complex. If none found, continue to run and
	 *     use remaining HFIs in the system.
	 *   - If we do find at least one HFI in same root complex, we
	 *     go ahead and add to list.
	 */
	if (multirail_within_socket_used) {
		node_id = psmi_get_current_proc_location();
		for (i = 0; i < num_units; i++) {
			if (psmi_hal_get_unit_active(i) <= 0)
				continue;
			int node_id_i;

			if (!psmi_hal_get_node_id(i, &node_id_i)) {
				if (node_id_i == node_id) {
					found = 1;
					break;
				}
			}
		}
	}
/*
 * Get all the ports with a valid lid and gid, one per unit.
 */
	for (i = 0; i < num_units; i++) {
		int node_id_i;

		if (!psmi_hal_get_node_id(i, &node_id_i))
		{
			if (multirail_within_socket_used &&
			    found && (node_id_i != node_id))
				continue;
		}

		for (j = HFI_MIN_PORT; j <= HFI_MAX_PORT; j++) {
			ret = psmi_hal_get_port_lid(i, j);
			if (ret <= 0)
				continue;
			ret = psmi_hal_get_port_gid(i, j, &gid_hi, &gid_lo);
			if (ret == -1)
				continue;

			gidh[count][0] = gid_hi;
			gidh[count][1] = i;
			gidh[count][2] = j;
			count++;
			break;
		}
	}

/*
 * Sort all the ports with gidh from small to big.
 * This is for multiple fabrics, and we use fabric with the
 * smallest gid to make the master connection.
 */
	qsort(gidh, count, sizeof(uint64_t) * 3, cmpfunc);

	for (i = 0; i < count; i++) {
		unit[i] = (uint32_t) gidh[i][1];
		port[i] = (uint16_t) (uint32_t) gidh[i][2];
	}
	*num_rails = count;
	return err;
}

static psm2_error_t
psmi_ep_devlids(uint16_t **lids, uint32_t *num_lids_o,
		uint64_t my_gid_hi, uint64_t my_gid_lo)
{
	uint32_t num_units;
	int i;
	psm2_error_t err = PSM2_OK;

	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	if (hfi_lids == NULL) {
		if ((err = psm2_ep_num_devunits(&num_units)))
			goto fail;
		hfi_lids = (uint16_t *)
		    psmi_calloc(PSMI_EP_NONE, UNDEFINED,
				num_units * psmi_hal_get_num_ports(), sizeof(uint16_t));
		if (hfi_lids == NULL) {
			err = psmi_handle_error(NULL, PSM2_NO_MEMORY,
						"Couldn't allocate memory for dev_lids structure");
			goto fail;
		}

		for (i = 0; i < num_units; i++) {
			int j;
			for (j = HFI_MIN_PORT; j <= HFI_MAX_PORT; j++) {
				int lid = psmi_hal_get_port_lid(i, j);
				int ret;
				uint64_t gid_hi = 0, gid_lo = 0;

				if (lid <= 0)
					continue;
				ret = psmi_hal_get_port_gid(i, j, &gid_hi, &gid_lo);
				if (ret == -1)
					continue;
				else if (my_gid_hi != gid_hi) {
					_HFI_VDBG("LID %d, unit %d, port %d, "
						  "mismatched GID %llx:%llx and "
						  "%llx:%llx\n",
						  lid, i, j,
						  (unsigned long long)gid_hi,
						  (unsigned long long)gid_lo,
						  (unsigned long long)my_gid_hi,
						  (unsigned long long)
						  my_gid_lo);
					continue;
				}
				_HFI_VDBG("LID %d, unit %d, port %d, "
					  "matching GID %llx:%llx and "
					  "%llx:%llx\n", lid, i, j,
					  (unsigned long long)gid_hi,
					  (unsigned long long)gid_lo,
					  (unsigned long long)my_gid_hi,
					  (unsigned long long)my_gid_lo);

				hfi_lids[nlids++] = (uint16_t) lid;
			}
		}
		if (nlids == 0) {
			err = psmi_handle_error(NULL, PSM2_EP_DEVICE_FAILURE,
						"Couldn't get lid&gid from any unit/port");
			goto fail;
		}
	}
	*lids = hfi_lids;
	*num_lids_o = nlids;

fail:
	return err;
}

static psm2_error_t
psmi_ep_verify_pkey(psm2_ep_t ep, uint16_t pkey, uint16_t *opkey)
{
	int i, ret;
	psm2_error_t err;

	for (i = 0; i < 16; i++) {
		ret = psmi_hal_get_port_index2pkey(ep->unit_id, ep->portnum, i);
		if (ret < 0) {
			err = psmi_handle_error(NULL, PSM2_EP_DEVICE_FAILURE,
						"Can't get a valid pkey value from pkey table\n");
			return err;
		} else if ((ret & 0x7fff) == 0x7fff) {
			continue;	/* management pkey, not for app traffic. */
		}

		if ((pkey & 0x7fff) == (uint16_t)(ret & 0x7fff)) {
			break;
		}
	}

	/* if pkey does not match */
	if (i == 16) {
		err = psmi_handle_error(NULL, PSM2_EP_DEVICE_FAILURE,
					"Wrong pkey 0x%x, please use PSM2_PKEY to specify a valid pkey\n",
					pkey);
		return err;
	}

	if (((uint16_t)ret & 0x8000) == 0) {
		err = psmi_handle_error(NULL, PSM2_EP_DEVICE_FAILURE,
					"Limited Member pkey 0x%x, please use PSM2_PKEY to specify a valid pkey\n",
					(uint16_t)ret);
		return err;
	}

	/* return the final pkey */
	*opkey = (uint16_t)ret;

	return PSM2_OK;
}

uint64_t __psm2_epid_nid(psm2_epid_t epid)
{
	uint64_t rv;

	PSM2_LOG_MSG("entering");
	rv = (uint64_t) PSMI_EPID_GET_LID(epid);
	PSM2_LOG_MSG("leaving");
	return rv;
}
PSMI_API_DECL(psm2_epid_nid)

/* Currently not exposed to users, we don't acknowledge the existence of
 * subcontexts */
uint64_t psmi_epid_subcontext(psm2_epid_t epid)
{
	return (uint64_t) PSMI_EPID_GET_SUBCONTEXT(epid);
}

/* Currently not exposed to users, we don't acknowledge the existence of
 * service levels encoding within epids. This may require
 * changing to expose SLs
 */
uint64_t psmi_epid_version(psm2_epid_t epid)
{
	return (uint64_t) PSMI_EPID_GET_EPID_VERSION(epid);
}

uint64_t __psm2_epid_context(psm2_epid_t epid)
{
	uint64_t rv;

	PSM2_LOG_MSG("entering");
	rv = (uint64_t) PSMI_EPID_GET_CONTEXT(epid);
	PSM2_LOG_MSG("leaving");
	return rv;
}
PSMI_API_DECL(psm2_epid_context)

uint64_t __psm2_epid_port(psm2_epid_t epid)
{
	uint64_t rv;
	PSM2_LOG_MSG("entering");
	rv = __psm2_epid_context(epid);
	PSM2_LOG_MSG("leaving");
	return rv;
}
PSMI_API_DECL(psm2_epid_port)

psm2_error_t __psm2_ep_query(int *num_of_epinfo, psm2_epinfo_t *array_of_epinfo)
{
	psm2_error_t err = PSM2_OK;
	int i;
	psm2_ep_t ep;

	PSM2_LOG_MSG("entering");
	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	if (*num_of_epinfo <= 0) {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid psm2_ep_query parameters");
		PSM2_LOG_MSG("leaving");
		return err;
	}

	if (psmi_opened_endpoint == NULL) {
		err = psmi_handle_error(NULL, PSM2_EP_WAS_CLOSED,
					"PSM Endpoint is closed or does not exist");
		PSM2_LOG_MSG("leaving");
		return err;
	}

	ep = psmi_opened_endpoint;
	for (i = 0; i < *num_of_epinfo; i++) {
		if (ep == NULL)
			break;
		array_of_epinfo[i].ep = ep;
		array_of_epinfo[i].epid = ep->epid;
		array_of_epinfo[i].jkey = ep->jkey;
		memcpy(array_of_epinfo[i].uuid,
		       (void *)ep->uuid, sizeof(psm2_uuid_t));
		psmi_uuid_unparse(ep->uuid, array_of_epinfo[i].uuid_str);
		ep = ep->user_ep_next;
	}
	*num_of_epinfo = i;
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_ep_query)

psm2_error_t __psm2_ep_epid_lookup(psm2_epid_t epid, psm2_epconn_t *epconn)
{
	psm2_error_t err = PSM2_OK;
	psm2_epaddr_t epaddr;
	psm2_ep_t ep;

	PSM2_LOG_MSG("entering");
	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	/* Need to have an opened endpoint before we can resolve epids */
	if (psmi_opened_endpoint == NULL) {
		err = psmi_handle_error(NULL, PSM2_EP_WAS_CLOSED,
					"PSM Endpoint is closed or does not exist");
		PSM2_LOG_MSG("leaving");
		return err;
	}

	ep = psmi_opened_endpoint;
	while (ep) {
		epaddr = psmi_epid_lookup(ep, epid);
		if (!epaddr) {
			ep = ep->user_ep_next;
			continue;
		}

		/* Found connection for epid. Return info about endpoint to caller. */
		psmi_assert_always(epaddr->ptlctl->ep == ep);
		epconn->addr = epaddr;
		epconn->ep = ep;
		epconn->mq = ep->mq;
		PSM2_LOG_MSG("leaving");
		return err;
	}

	err = psmi_handle_error(NULL, PSM2_EPID_UNKNOWN,
				"Endpoint connection status unknown");
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_ep_epid_lookup);

psm2_error_t __psm2_ep_epid_lookup2(psm2_ep_t ep, psm2_epid_t epid, psm2_epconn_t *epconn)
{
	psm2_error_t err = PSM2_OK;

	PSM2_LOG_MSG("entering");
	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	/* Need to have an opened endpoint before we can resolve epids */
	if (ep == NULL) {
		err = psmi_handle_error(NULL, PSM2_EP_WAS_CLOSED,
					"PSM Endpoint is closed or does not exist");
		PSM2_LOG_MSG("leaving");
		return err;
	}

	if (epconn == NULL) {
		err = psmi_handle_error(ep, PSM2_PARAM_ERR,
					"Invalid output parameter");
		PSM2_LOG_MSG("leaving");
		return err;
	}

	psm2_epaddr_t epaddr = psmi_epid_lookup(ep, epid);
	if (epaddr) {
		/* Found connection for epid. Return info about endpoint to caller. */
		psmi_assert_always(epaddr->ptlctl->ep == ep);
		epconn->addr = epaddr;
		epconn->ep = ep;
		epconn->mq = ep->mq;
		PSM2_LOG_MSG("leaving");
		return err;
	}

	err = psmi_handle_error(ep, PSM2_EPID_UNKNOWN,
				"Endpoint connection status unknown");
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_ep_epid_lookup2);

psm2_error_t __psm2_epaddr_to_epid(psm2_epaddr_t epaddr, psm2_epid_t *epid)
{
	psm2_error_t err = PSM2_OK;
	PSM2_LOG_MSG("entering");
	if (epaddr && epid) {
		*epid = epaddr->epid;
	}
	else {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid input epaddr or output epid parameter");
	}
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_epaddr_to_epid);

psm2_error_t
__psm2_ep_epid_share_memory(psm2_ep_t ep, psm2_epid_t epid, int *result_o)
{
	uint32_t num_lids = 0;
	uint16_t *lids = NULL;
	int i;
	uint16_t epid_lid;
	int result = 0;
	psm2_error_t err;

	PSM2_LOG_MSG("entering");
	psmi_assert_always(ep != NULL);
	PSMI_ERR_UNLESS_INITIALIZED(ep);

	if ((!psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS)) ||
		(psmi_epid_version(epid) == PSMI_EPID_VERSION_SHM)) {
		/* If we are in the no hfi-mode, or the other process is,
		 * the epid doesn't help us - so assume both we're on the same
		 * machine and try to connect.
		 */
		result = 1;
	} else {
		epid_lid = (uint16_t) psm2_epid_nid(epid);
		err = psmi_ep_devlids(&lids, &num_lids, ep->gid_hi, ep->gid_lo);
		if (err) {
			PSM2_LOG_MSG("leaving");
			return err;
		}
		for (i = 0; i < num_lids; i++) {
			if (epid_lid == lids[i]) {
				/* we share memory if the lid is the same. */
				result = 1;
				break;
			}
		}
	}
	*result_o = result;
	PSM2_LOG_MSG("leaving");
	return PSM2_OK;
}
PSMI_API_DECL(psm2_ep_epid_share_memory)

psm2_error_t __psm2_ep_open_opts_get_defaults(struct psm2_ep_open_opts *opts)
{
	PSM2_LOG_MSG("entering");

	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	if (!opts)
		return PSM2_PARAM_ERR;

	/* Set in order in the structure. */
	opts->timeout = 30000000000LL;	/* 30 sec */
	opts->unit = HFI_UNIT_ID_ANY;
	opts->affinity = PSM2_EP_OPEN_AFFINITY_SET;
	opts->shm_mbytes = 0;	/* deprecated in psm2.h */
	opts->sendbufs_num = 1024;
	opts->network_pkey = psmi_hal_get_default_pkey();
	opts->port = HFI_PORT_NUM_ANY;
	opts->outsl = PSMI_SL_DEFAULT;
	opts->service_id = HFI_DEFAULT_SERVICE_ID;
	opts->path_res_type = PSM2_PATH_RES_NONE;
	opts->senddesc_num = 4096;
	opts->imm_size = 128;
	PSM2_LOG_MSG("leaving");
	return PSM2_OK;
}
PSMI_API_DECL(psm2_ep_open_opts_get_defaults)

psm2_error_t psmi_poll_noop(ptl_t *ptl, int replyonly);

psm2_error_t
__psm2_ep_open_internal(psm2_uuid_t const unique_job_key, int *devid_enabled,
		       struct psm2_ep_open_opts const *opts_i, psm2_mq_t mq,
		       psm2_ep_t *epo, psm2_epid_t *epido)
{
	psm2_ep_t ep = NULL;
	uint32_t num_units;
	size_t len;
	psm2_error_t err;
	psm2_epaddr_t epaddr = NULL;
	char buf[128], *p, *e;
	union psmi_envvar_val envvar_val;
	size_t ptl_sizes;
	struct psm2_ep_open_opts opts;
	ptl_t *amsh_ptl, *ips_ptl, *self_ptl;
	int i;

	/* First get the set of default options, we overwrite with the user's
	 * desired values afterwards */
	if ((err = psm2_ep_open_opts_get_defaults(&opts)))
		goto fail;

	if (opts_i != NULL) {
		if (opts_i->timeout != -1)
			opts.timeout = opts_i->timeout;
		if (opts_i->unit != -1)
			opts.unit = opts_i->unit;
		if (opts_i->affinity != -1)
			opts.affinity = opts_i->affinity;

		if (opts_i->sendbufs_num != -1)
			opts.sendbufs_num = opts_i->sendbufs_num;

		if (opts_i->network_pkey != psmi_hal_get_default_pkey())
			opts.network_pkey = opts_i->network_pkey;

		if (opts_i->port != 0)
			opts.port = opts_i->port;

		if (opts_i->outsl != -1)
			opts.outsl = opts_i->outsl;

		if (opts_i->service_id)
			opts.service_id = (uint64_t) opts_i->service_id;
		if (opts_i->path_res_type != PSM2_PATH_RES_NONE)
			opts.path_res_type = opts_i->path_res_type;

		if (opts_i->senddesc_num)
			opts.senddesc_num = opts_i->senddesc_num;
		if (opts_i->imm_size)
			opts.imm_size = opts_i->imm_size;
	}

	/* Get Service ID from environment */
	if (!psmi_getenv("PSM2_IB_SERVICE_ID",
			 "HFI Service ID for path resolution",
			 PSMI_ENVVAR_LEVEL_USER,
			 PSMI_ENVVAR_TYPE_ULONG_ULONG,
			 (union psmi_envvar_val)HFI_DEFAULT_SERVICE_ID,
			 &envvar_val)) {
		opts.service_id = (uint64_t) envvar_val.e_ulonglong;
	}

	/* Get Path resolution type from environment Possible choices are:
	 *
	 * NONE : Default same as previous instances. Utilizes static data.
	 * OPP  : Use OFED Plus Plus library to do path record queries.
	 * UMAD : Use raw libibumad interface to form and process path records.
	 */
	if (!psmi_getenv("PSM2_PATH_REC",
			 "Mechanism to query HFI path record (default is no path query)",
			 PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_STR,
			 (union psmi_envvar_val)"none", &envvar_val)) {
		if (!strcasecmp(envvar_val.e_str, "none"))
			opts.path_res_type = PSM2_PATH_RES_NONE;
		else if (!strcasecmp(envvar_val.e_str, "opp"))
			opts.path_res_type = PSM2_PATH_RES_OPP;
		else if (!strcasecmp(envvar_val.e_str, "umad"))
			opts.path_res_type = PSM2_PATH_RES_UMAD;
		else {
			_HFI_ERROR("Unknown path resolution type %s. "
				   "Disabling use of path record query.\n",
				   envvar_val.e_str);
			opts.path_res_type = PSM2_PATH_RES_NONE;
		}
	}

	/* If a specific unit is set in the environment, use that one. */
	if (!psmi_getenv("HFI_UNIT", "Device Unit number (-1 autodetects)",
			 PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_LONG,
			 (union psmi_envvar_val)HFI_UNIT_ID_ANY, &envvar_val)) {
		opts.unit = envvar_val.e_long;
	}

	/* Get user specified port number to use. */
	if (!psmi_getenv("HFI_PORT", "IB Port number (0 autodetects)",
			 PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_LONG,
			 (union psmi_envvar_val)HFI_PORT_NUM_ANY,
			 &envvar_val)) {
		opts.port = envvar_val.e_long;
	}

	/* Get service level from environment, path-query overrides it */
	if (!psmi_getenv
	    ("HFI_SL", "HFI outging ServiceLevel number (default 0)",
	     PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_LONG,
	     (union psmi_envvar_val)PSMI_SL_DEFAULT, &envvar_val)) {
		opts.outsl = envvar_val.e_long;
	}

	/* Get network key from environment. MVAPICH and other vendor MPIs do not
	 * specify it on ep open and we may require it for vFabrics.
	 * path-query will override it.
	 */
	if (!psmi_getenv("PSM2_PKEY",
			 "HFI PKey to use for endpoint",
			 PSMI_ENVVAR_LEVEL_USER,
			 PSMI_ENVVAR_TYPE_ULONG,
			 (union psmi_envvar_val)((unsigned int)(psmi_hal_get_default_pkey())),
			 &envvar_val)) {
		opts.network_pkey = (uint64_t) envvar_val.e_ulong;
	}

	/* BACKWARDS COMPATIBILITY:  Open MPI likes to choose its own PKEY of
	   0x7FFF.  That's no longer a valid default, so override it if the
	   client was compiled against PSM v1 */
	if (PSMI_VERNO_GET_MAJOR(psmi_verno_client()) < 2 &&
			opts.network_pkey == 0x7FFF) {
		opts.network_pkey = psmi_hal_get_default_pkey();;
	}

	/* Get number of default send buffers from environment */
	if (!psmi_getenv("PSM2_NUM_SEND_BUFFERS",
			 "Number of send buffers to allocate [1024]",
			 PSMI_ENVVAR_LEVEL_USER,
			 PSMI_ENVVAR_TYPE_UINT,
			 (union psmi_envvar_val)1024, &envvar_val)) {
		opts.sendbufs_num = envvar_val.e_uint;
	}

	/* Get immediate data size - transfers less than immediate data size do
	 * not consume a send buffer and require just a send descriptor.
	 */
	if (!psmi_getenv("PSM2_SEND_IMMEDIATE_SIZE",
			 "Immediate data send size not requiring a buffer [128]",
			 PSMI_ENVVAR_LEVEL_USER,
			 PSMI_ENVVAR_TYPE_UINT,
			 (union psmi_envvar_val)128, &envvar_val)) {
		opts.imm_size = envvar_val.e_uint;
	}

	/* Get number of send descriptors - by default this is 4 times the number
	 * of send buffers - mainly used for short/inlined messages.
	 */
	if (!psmi_getenv("PSM2_NUM_SEND_DESCRIPTORS",
			 "Number of send descriptors to allocate [4096]",
			 PSMI_ENVVAR_LEVEL_USER,
			 PSMI_ENVVAR_TYPE_UINT,
			 (union psmi_envvar_val)4096, &envvar_val)) {
		opts.senddesc_num = envvar_val.e_uint;
	}

	if (psmi_device_is_enabled(devid_enabled, PTL_DEVID_IPS)) {
		if ((err = psm2_ep_num_devunits(&num_units)) != PSM2_OK)
			goto fail;
	} else
		num_units = 0;

	/* do some error checking */
	if (opts.timeout < -1) {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid timeout value %lld",
					(long long)opts.timeout);
		goto fail;
	} else if (num_units && (opts.unit < -1 || opts.unit >= (int)num_units)) {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid Device Unit ID %d (%d units found)",
					opts.unit, num_units);
		goto fail;
	} else if ((opts.port < HFI_MIN_PORT || opts.port > HFI_MAX_PORT) &&
				opts.port != HFI_PORT_NUM_ANY) {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid Device port number %d",
					opts.port);
		goto fail;
	} else if (opts.affinity < 0
		   || opts.affinity > PSM2_EP_OPEN_AFFINITY_FORCE) {
		err =
		    psmi_handle_error(NULL, PSM2_PARAM_ERR,
				      "Invalid Affinity option: %d",
				      opts.affinity);
		goto fail;
	} else if (opts.outsl < PSMI_SL_MIN || opts.outsl > PSMI_SL_MAX) {
		err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
					"Invalid SL number: %lld",
					(unsigned long long)opts.outsl);
		goto fail;
	}

	/* Allocate end point structure storage */
	ptl_sizes =
	    (psmi_device_is_enabled(devid_enabled, PTL_DEVID_SELF) ?
	     psmi_ptl_self.sizeof_ptl() : 0) +
	    (psmi_device_is_enabled(devid_enabled, PTL_DEVID_IPS) ?
	     psmi_ptl_ips.sizeof_ptl() : 0) +
	    (psmi_device_is_enabled(devid_enabled, PTL_DEVID_AMSH) ?
	     psmi_ptl_amsh.sizeof_ptl() : 0);
	if (ptl_sizes == 0)
		return PSM2_EP_NO_DEVICE;

	ep = (psm2_ep_t) psmi_memalign(PSMI_EP_NONE, UNDEFINED, 64,
				      sizeof(struct psm2_ep) + ptl_sizes);
	epaddr = (psm2_epaddr_t) psmi_calloc(PSMI_EP_NONE, PER_PEER_ENDPOINT,
					    1, sizeof(struct psm2_epaddr));
	if (ep == NULL || epaddr == NULL) {
		err = psmi_handle_error(NULL, PSM2_NO_MEMORY,
					"Couldn't allocate memory for %s structure",
					ep == NULL ? "psm2_ep" : "psm2_epaddr");
		goto fail;
	}
	memset(ep, 0, sizeof(struct psm2_ep) + ptl_sizes);

	/* Copy PTL enabled status */
	for (i = 0; i < PTL_MAX_INIT; i++)
		ep->devid_enabled[i] = devid_enabled[i];

	/* Matched Queue initialization.  We do this early because we have to
	 * make sure ep->mq exists and is valid before calling ips_do_work.
	 */
	ep->mq = mq;

	/* Get ready for PTL initialization */
	memcpy(&ep->uuid, (void *)unique_job_key, sizeof(psm2_uuid_t));
	ep->epaddr = epaddr;
	ep->memmode = mq->memmode;
	ep->hfi_num_sendbufs = opts.sendbufs_num;
	ep->service_id = opts.service_id;
	ep->path_res_type = opts.path_res_type;
	ep->hfi_num_descriptors = opts.senddesc_num;
	ep->hfi_imm_size = opts.imm_size;
	ep->errh = psmi_errhandler_global;	/* by default use the global one */
	ep->ptl_amsh.ep_poll = psmi_poll_noop;
	ep->ptl_ips.ep_poll = psmi_poll_noop;
	ep->connections = 0;

	/* See how many iterations we want to spin before yielding */
	psmi_getenv("PSM2_YIELD_SPIN_COUNT",
		    "Spin poll iterations before yield",
		    PSMI_ENVVAR_LEVEL_HIDDEN,
		    PSMI_ENVVAR_TYPE_UINT,
		    (union psmi_envvar_val)PSMI_BLOCKUNTIL_POLLS_BEFORE_YIELD,
		    &envvar_val);
	ep->yield_spin_cnt = envvar_val.e_uint;

	/* Set skip_affinity flag if PSM is not allowed to set affinity */
	if (opts.affinity == PSM2_EP_OPEN_AFFINITY_SKIP)
		ep->skip_affinity = true;

	ptl_sizes = 0;
	amsh_ptl = ips_ptl = self_ptl = NULL;
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_AMSH)) {
		amsh_ptl = (ptl_t *) (ep->ptl_base_data + ptl_sizes);
		ptl_sizes += psmi_ptl_amsh.sizeof_ptl();
	}
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS)) {
		ips_ptl = (ptl_t *) (ep->ptl_base_data + ptl_sizes);
		ptl_sizes += psmi_ptl_ips.sizeof_ptl();
	}
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_SELF)) {
		self_ptl = (ptl_t *) (ep->ptl_base_data + ptl_sizes);
		ptl_sizes += psmi_ptl_self.sizeof_ptl();
	}

	if ((err = psmi_ep_open_device(ep, &opts, unique_job_key,
				       &(ep->context), &ep->epid)))
		goto fail;

	psmi_assert_always(ep->epid != 0);
	ep->epaddr->epid = ep->epid;

	_HFI_VDBG("psmi_ep_open_device() passed\n");

	/* Set our new label as soon as we know what it is */
	strncpy(buf, psmi_gethostname(), sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	p = buf + strlen(buf);

	/* If our rank is set, use it. If not, use context.subcontext notation */
	if (((e = getenv("MPI_RANKID")) != NULL && *e) ||
	    ((e = getenv("PSC_MPI_RANK")) != NULL && *e))
		len = snprintf(p, sizeof(buf) - strlen(buf), ":%d.", atoi(e));
	else
		len = snprintf(p, sizeof(buf) - strlen(buf), ":%d.%d.",
			       (uint32_t) psm2_epid_context(ep->epid),
			       (uint32_t) psmi_epid_subcontext(ep->epid));
	*(p + len) = '\0';
	ep->context_mylabel = psmi_strdup(ep, buf);
	if (ep->context_mylabel == NULL) {
		err = PSM2_NO_MEMORY;
		goto fail;
	}
	/* hfi_set_mylabel(ep->context_mylabel); */

	if ((err = psmi_epid_set_hostname(psm2_epid_nid(ep->epid), buf, 0)))
		goto fail;

	_HFI_VDBG("start ptl device init...\n");
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_SELF)) {
		if ((err = psmi_ptl_self.init(ep, self_ptl, &ep->ptl_self)))
			goto fail;
	}
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS)) {
		if ((err = psmi_ptl_ips.init(ep, ips_ptl, &ep->ptl_ips)))
			goto fail;
	}
	/* If we're shm-only, this device is enabled above */
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_AMSH)) {
		if ((err = psmi_ptl_amsh.init(ep, amsh_ptl, &ep->ptl_amsh)))
			goto fail;
	} else {
		/* We may have pre-attached as part of getting our rank for enabling
		 * shared contexts.  */
	}

	_HFI_VDBG("finish ptl device init...\n");

	/*
	 * Keep only IPS since only IPS support multi-rail, other devices
	 * are only setup once. IPS device can come to this function again.
	 */
	for (i = 0; i < PTL_MAX_INIT; i++) {
		if (devid_enabled[i] != PTL_DEVID_IPS) {
			devid_enabled[i] = -1;
		}
	}

	*epido = ep->epid;
	*epo = ep;

	return PSM2_OK;

fail:
	if (ep != NULL) {
		psmi_hal_close_context(&ep->context.psm_hw_ctxt);
		psmi_free(ep);
	}
	if (epaddr != NULL)
		psmi_free(epaddr);
	return err;
}

psm2_error_t
__psm2_ep_open(psm2_uuid_t const unique_job_key,
	      struct psm2_ep_open_opts const *opts_i, psm2_ep_t *epo,
	      psm2_epid_t *epido)
{
	psm2_error_t err;
	psm2_mq_t mq;
	psm2_epid_t epid;
	psm2_ep_t ep, tmp;
	uint32_t units[HFI_MAX_RAILS];
	uint16_t ports[HFI_MAX_RAILS];
	int i, num_rails = 0;
	char *uname = "HFI_UNIT";
	char *pname = "HFI_PORT";
	char uvalue[6], pvalue[6];
	int devid_enabled[PTL_MAX_INIT];
	union psmi_envvar_val devs;

	PSM2_LOG_MSG("entering");
	PSMI_ERR_UNLESS_INITIALIZED(NULL);

	if (!epo || !epido)
		return PSM2_PARAM_ERR;

	/* Allowing only one EP (unless explicitly enabled). */
	if (psmi_opened_endpoint_count > 0 && !psmi_multi_ep_enabled) {
		PSM2_LOG_MSG("leaving");
		return PSM2_TOO_MANY_ENDPOINTS;
	}

	/* Matched Queue initialization.  We do this early because we have to
	 * make sure ep->mq exists and is valid before calling ips_do_work.
	 */
	err = psmi_mq_malloc(&mq);
	PSMI_LOCK(psmi_creation_lock);
	if (err != PSM2_OK)
		goto fail;

	/* Set some of the MQ thresholds from the environment.
	   Do this before ptl initialization - the ptl may have other
	   constraints that will limit the MQ's settings. */
	err = psmi_mq_initialize_defaults(mq);
	if (err != PSM2_OK)
		goto fail;

	psmi_init_lock(&(mq->progress_lock));

	/* See which ptl devices we want to use for this ep to be opened */
	psmi_getenv("PSM2_DEVICES",
		    "Ordered list of PSM-level devices",
		    PSMI_ENVVAR_LEVEL_USER,
		    PSMI_ENVVAR_TYPE_STR,
		    (union psmi_envvar_val)PSMI_DEVICES_DEFAULT, &devs);

	if ((err = psmi_parse_devices(devid_enabled, devs.e_str)))
		goto fail;

	if (psmi_device_is_enabled(devid_enabled, PTL_DEVID_IPS)) {
		err = psmi_ep_multirail(&num_rails, units, ports);
		if (err != PSM2_OK)
			goto fail;

		/* If multi-rail is used, set the first ep unit/port */
		if (num_rails > 0) {
			snprintf(uvalue, 6, "%1d", units[0]);
			snprintf(pvalue, 6, "%1d", ports[0]);
			setenv(uname, uvalue, 1);
			setenv(pname, pvalue, 1);
		}
	}

#ifdef PSM_CUDA
	if (PSMI_IS_GDR_COPY_ENABLED)
		hfi_gdr_open();
#endif

	err = __psm2_ep_open_internal(unique_job_key,
				     devid_enabled, opts_i, mq, &ep, &epid);
	if (err != PSM2_OK)
		goto fail;

	if (psmi_opened_endpoint == NULL) {
		psmi_opened_endpoint = ep;
	} else {
		tmp = psmi_opened_endpoint;
		while (tmp->user_ep_next)
			tmp = tmp->user_ep_next;
		tmp->user_ep_next = ep;
	}
	psmi_opened_endpoint_count++;
	ep->mctxt_prev = ep->mctxt_next = ep;
	ep->mctxt_master = ep;
	mq->ep = ep;

	/* Active Message initialization */
	err = psmi_am_init_internal(ep);
	if (err != PSM2_OK)
		goto fail;

	*epo = ep;
	*epido = epid;

	if (psmi_device_is_enabled(devid_enabled, PTL_DEVID_IPS)) {
		for (i = 1; i < num_rails; i++) {
			snprintf(uvalue, 6, "%1d", units[i]);
			snprintf(pvalue, 6, "%1d", ports[i]);
			setenv(uname, uvalue, 1);
			setenv(pname, pvalue, 1);

			/* Create slave EP */
			err = __psm2_ep_open_internal(unique_job_key,
						     devid_enabled, opts_i, mq,
						     &tmp, &epid);
			if (err)
				goto fail;

			/* Point back to shared resources on the master EP */
			tmp->am_htable = ep->am_htable;

			/* Link slave EP after master EP. */
			PSM_MCTXT_APPEND(ep, tmp);
		}
	}

	_HFI_VDBG("psm2_ep_open() OK....\n");

fail:
	PSMI_UNLOCK(psmi_creation_lock);
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_ep_open)

psm2_error_t __psm2_ep_close(psm2_ep_t ep, int mode, int64_t timeout_in)
{
	psm2_error_t err = PSM2_OK;
#if _HFI_DEBUGGING
	uint64_t t_start = 0;
	if (_HFI_PRDBG_ON) {
		t_start = get_cycles();
	}
#endif

#ifdef PSM_CUDA
	/*
	 * The close on the gdr fd needs to be called before the
	 * close on the hfi fd as the the gdr device will hold
	 * reference count on the hfi device which will make the close
	 * on the hfi fd return without actually closing the fd.
	 */
	if (PSMI_IS_GDR_COPY_ENABLED)
		hfi_gdr_close();
#endif
	union psmi_envvar_val timeout_intval;
	psm2_ep_t tmp;
	psm2_mq_t mmq;

	PSM2_LOG_MSG("entering");
	PSMI_ERR_UNLESS_INITIALIZED(ep);
	psmi_assert_always(ep->mctxt_master == ep);

	PSMI_LOCK(psmi_creation_lock);

	psmi_am_fini_internal(ep);

	if (psmi_opened_endpoint == NULL) {
		err = psmi_handle_error(NULL, PSM2_EP_WAS_CLOSED,
					"PSM Endpoint is closed or does not exist");
		PSM2_LOG_MSG("leaving");
		PSMI_UNLOCK(psmi_creation_lock);
		return err;
	}

	tmp = psmi_opened_endpoint;
	while (tmp && tmp != ep) {
		tmp = tmp->user_ep_next;
	}
	if (!tmp) {
		err = psmi_handle_error(NULL, PSM2_EP_WAS_CLOSED,
					"PSM Endpoint is closed or does not exist");
		PSM2_LOG_MSG("leaving");
		PSMI_UNLOCK(psmi_creation_lock);
		return err;
	}

	psmi_getenv("PSM2_CLOSE_TIMEOUT",
		    "End-point close timeout over-ride.",
		    PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_UINT,
		    (union psmi_envvar_val)0, &timeout_intval);

	if (getenv("PSM2_CLOSE_TIMEOUT")) {
		timeout_in = timeout_intval.e_uint * SEC_ULL;
	} else if (timeout_in > 0) {
		/* The timeout parameter provides the minimum timeout. A heuristic
		 * is used to scale up the timeout linearly with the number of
		 * endpoints, and we allow one second per 100 endpoints. */
		timeout_in = max(timeout_in, (ep->connections * SEC_ULL) / 100);
	}

	if (timeout_in > 0 && timeout_in < PSMI_MIN_EP_CLOSE_TIMEOUT)
		timeout_in = PSMI_MIN_EP_CLOSE_TIMEOUT;

	/* Infinite and excessive close time-out are limited here to a max.
	 * The "rationale" is that there is no point waiting around forever for
	 * graceful termination. Normal (or forced) process termination should clean
	 * up the context state correctly even if termination is not graceful. */
	if (timeout_in <= 0 || timeout_in > PSMI_MAX_EP_CLOSE_TIMEOUT)
		timeout_in = PSMI_MAX_EP_CLOSE_TIMEOUT;
	_HFI_PRDBG("Closing endpoint %p with force=%s and to=%.2f seconds and "
		   "%d connections\n",
		   ep, mode == PSM2_EP_CLOSE_FORCE ? "YES" : "NO",
		   (double)timeout_in / 1e9, (int)ep->connections);

	/* XXX We currently cheat in the sense that we leave each PTL the allowed
	 * timeout.  There's no good way to do this until we change the PTL
	 * interface to allow asynchronous finalization
	 */


	/* Check if transfer ownership of receive thread is needed before closing ep.
	 * In case of PSM2_MULTI_EP support receive thread is created and assigned
	 * to first opened endpoint. Receive thread is killed when closing this
	 * endpoint.
	 */
	if (ep->user_ep_next != NULL) {
		/* Receive thread will be transfered and assigned to ep->user_ep_next
		 * only if currently working receive thread (which will be killed) is
		 * assigned to ep and there isn't any assigned to ep->user_ep_next.
		 */
		if ((psmi_ptl_ips_rcvthread.is_enabled(ep->ptl_ips.ptl)) &&
		    (!psmi_ptl_ips_rcvthread.is_enabled(ep->user_ep_next->ptl_ips.ptl)))
			psmi_ptl_ips_rcvthread.transfer_ownership(ep->ptl_ips.ptl, ep->user_ep_next->ptl_ips.ptl);
	}

	/*
	 * Before freeing the master ep itself,
	 * remove it from the global linklist.
	 * We do it here to let atexit handler in ptl_am directory
	 * to search the global linklist and free the shared memory file.
	 */
	if (psmi_opened_endpoint == ep) {
		/* Removing ep from global endpoint list. */
		psmi_opened_endpoint = ep->user_ep_next;
	} else {
		tmp = psmi_opened_endpoint;
		while (tmp->user_ep_next != ep) {
			tmp = tmp->user_ep_next;
		}
		/* Removing ep from global endpoint list. */
		tmp->user_ep_next = ep->user_ep_next;
	}
	psmi_opened_endpoint_count--;

	/*
	 * This do/while loop is used to close and free memory of endpoints.
	 *
	 * If MULTIRAIL feature is disable this loop will be passed only once
	 * and only endpoint passed in psm2_ep_close will be closed/removed.
	 *
	 * If MULTIRAIL feature is enabled then this loop will be passed
	 * multiple times (depending on number of rails). The order in which
	 * endpoints will be closed is shown below:
	 *
	 *                      |--this is master endpoint in case of multirail
	 *	                |  this endpoint is passed to psm2_ep_close and
	 *			V  this is only endpoint known to user.
	 *   +<-Ep0<-Ep1<-Ep2<-Ep3
	 *   |__________________|	Ep3->mctxt_prev points to Ep2
	 *	(3)  (2)  (1)  (4)	Ep2->mctxt_prev points to Ep1
	 *	 ^			Ep1->mctxt_prev points to Ep0
	 *	 |			Ep0->mctxt_prev points to Ep3 (master ep)
	 *	 |
	 *       |---- order in which endpoints will be closed.
	 *
	 * Closing MULTIRAILs starts by closing slaves (Ep2, Ep1, Ep0)
	 * If MULTIRAIL is enabled then Ep3->mctxt_prev will point to Ep2, if
	 * feature is disabled then Ep3->mctxt_prev will point to Ep3 and
	 * do/while loop will have one pass.
	 *
	 * In case of MULTIRAIL enabled Ep3 which is master endpoint will be
	 * closed as the last one.
	 */
	mmq = ep->mq;
	tmp = ep->mctxt_prev;
	do {
		ep = tmp;
		tmp = ep->mctxt_prev;

		PSMI_LOCK(ep->mq->progress_lock);

		PSM_MCTXT_REMOVE(ep);
		if (psmi_ep_device_is_enabled(ep, PTL_DEVID_AMSH))
			err =
			    psmi_ptl_amsh.fini(ep->ptl_amsh.ptl, mode,
					       timeout_in);

		if ((err == PSM2_OK || err == PSM2_TIMEOUT) &&
		    psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS))
			err =
			    psmi_ptl_ips.fini(ep->ptl_ips.ptl, mode,
					      timeout_in);

		/* If there's timeouts in the disconnect requests,
		 * still make sure that we still get to close the
		 *endpoint and mark it closed */
		if (psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS))
			psmi_context_close(&ep->context);

		psmi_epid_remove_all(ep);
		psmi_free(ep->epaddr);
		psmi_free(ep->context_mylabel);

		PSMI_UNLOCK(ep->mq->progress_lock);

		ep->mq = NULL;
		psmi_free(ep);

	} while ((err == PSM2_OK || err == PSM2_TIMEOUT) && tmp != ep);

	if (mmq) {
		psmi_destroy_lock(&(mmq->progress_lock));
		err = psmi_mq_free(mmq);
	}

	if (hfi_lids)
	{
		psmi_free(hfi_lids);
		hfi_lids = NULL;
		nlids = 0;
	}

	PSMI_UNLOCK(psmi_creation_lock);

	if (_HFI_PRDBG_ON) {
		_HFI_PRDBG_ALWAYS("Closed endpoint in %.3f secs\n",
				 (double)cycles_to_nanosecs(get_cycles() -
				 t_start) / SEC_ULL);
	}
	PSM2_LOG_MSG("leaving");
	return err;
}
PSMI_API_DECL(psm2_ep_close)

static
psm2_error_t
psmi_ep_open_device(const psm2_ep_t ep,
		    const struct psm2_ep_open_opts *opts,
		    const psm2_uuid_t unique_job_key,
		    struct psmi_context *context, psm2_epid_t *epid)
{
	psm2_error_t err = PSM2_OK;

	/* Skip affinity.  No affinity if:
	 * 1. User explicitly sets no-affinity=YES in environment.
	 * 2. User doesn't set affinity in environment and PSM is opened with
	 *    option affinity skip.
	 */
	if (psmi_ep_device_is_enabled(ep, PTL_DEVID_IPS)) {
		union psmi_envvar_val env_rcvthread;
		static int norcvthread;	/* only for first rail */

		ep->out_sl = opts->outsl;

		if ((err =
		     psmi_context_open(ep, opts->unit, opts->port,
				       unique_job_key, opts->timeout,
				       context)) != PSM2_OK)
			goto fail;

		_HFI_DBG("[%d]use unit %d port %d\n", getpid(),
			 psmi_hal_get_unit_id(ep->context.psm_hw_ctxt), 1);

		/* At this point, we have the unit id and port number, so
		 * check if pkey is not 0x0/0x7fff/0xffff, and match one
		 * of the pkey in table.
		 */
		if ((err =
		     psmi_ep_verify_pkey(ep, (uint16_t) opts->network_pkey,
					 &ep->network_pkey)) != PSM2_OK)
			goto fail;

		/* See if we want to activate support for receive thread */
		psmi_getenv("PSM2_RCVTHREAD",
			    "Recv thread flags (0 disables thread)",
			    PSMI_ENVVAR_LEVEL_USER, PSMI_ENVVAR_TYPE_UINT_FLAGS,
			    (union psmi_envvar_val)(norcvthread++ ? 0 :
						    PSMI_RCVTHREAD_FLAGS),
			    &env_rcvthread);

		/* If enabled, use the polling capability to implement a receive
		 * interrupt thread that can handle urg packets */
		if (env_rcvthread.e_uint) {
			psmi_hal_add_sw_status(PSM_HAL_PSMI_RUNTIME_RTS_RX_THREAD);
#ifdef PSMI_PLOCK_IS_NOLOCK
			psmi_handle_error(PSMI_EP_NORETURN, PSM2_INTERNAL_ERR,
					  "#define PSMI_PLOCK_IS_NOLOCK not functional yet "
					  "with RCVTHREAD on");
#endif
		}

		*epid = context->epid;
	} else if (psmi_ep_device_is_enabled(ep, PTL_DEVID_AMSH)) {
		int rank;

		/* In shm-only mode, we need to derive a valid epid
		 * based on our rank.  We try to get it from the
		 * environment if its available, or resort to using
		 * our PID as the rank.
		 */
		union psmi_envvar_val env_rankid;

		if (psmi_getenv
		    ("MPI_LOCALRANKID", "Shared context rankid",
		     PSMI_ENVVAR_LEVEL_HIDDEN, PSMI_ENVVAR_TYPE_INT,
		     (union psmi_envvar_val)-1, &env_rankid)) {
			if (psmi_getenv
			    ("PSC_MPI_NODE_RANK",
			     "Shared context rankid",
			     PSMI_ENVVAR_LEVEL_HIDDEN,
			     PSMI_ENVVAR_TYPE_INT,
			     (union psmi_envvar_val)-1, &env_rankid)) {
				rank = getpid();
			} else
				rank = env_rankid.e_int;
		} else
			rank = env_rankid.e_int;

		/*
		 * We use a LID of 0 for non-HFI communication.
		 * Since a jobkey is not available from IPS, pull the
		 * first 16 bits from the UUID.
		 */
		switch (PSMI_EPID_VERSION) {
			case PSMI_EPID_V1:
				*epid = PSMI_EPID_PACK_V1(((uint16_t *) unique_job_key)[0],
					   (rank >> 3), rank, 0,
					   PSMI_EPID_VERSION_SHM, rank);
				break;
			case PSMI_EPID_V2:
				/* Construct epid for this Endpoint */
				*epid = PSMI_EPID_PACK_V2_SHM(getpid(),
								PSMI_EPID_SHM_ONLY, /*is a only-shm epid */
								PSMI_EPID_VERSION);
				break;
			default:
				/* Epid version is greater than max supportd version. */
				psmi_assert_always(PSMI_EPID_VERSION <= PSMI_EPID_V2);
				break;
		}
	} else {
		/* Self-only, meaning only 1 proc max */
		switch (PSMI_EPID_VERSION) {
			case PSMI_EPID_V1:
				*epid = PSMI_EPID_PACK_V1(
					0, 0, 0, 0, PSMI_EPID_VERSION_SHM, 0x3ffffff);
				break;
			case PSMI_EPID_V2:
				*epid = PSMI_EPID_PACK_V2_SHM(0,
								PSMI_EPID_SHM_ONLY, /*is a only-shm epid */
								PSMI_EPID_VERSION);
				break;
			default:
				/* Epid version is greater than max supportd version. */
				psmi_assert_always(PSMI_EPID_VERSION <= PSMI_EPID_V2);
				break;
		}
	}

fail:
	return err;
}

/* Get a list of PTLs we want to use.  The order is important, it affects
 * whether node-local processes use shm or ips */
psm2_error_t
psmi_parse_devices(int devices[PTL_MAX_INIT], const char *devstring)
{
	char *devstr = NULL;
	char *b_new, *e, *ee, *b;
	psm2_error_t err = PSM2_OK;
	int len;
	int i = 0;

	psmi_assert_always(devstring != NULL);
	len = strlen(devstring) + 1;

	for (i = 0; i < PTL_MAX_INIT; i++)
		devices[i] = -1;

	devstr = (char *)psmi_calloc(PSMI_EP_NONE, UNDEFINED, 2, len);
	if (devstr == NULL)
		goto fail;

	b_new = (char *)devstr;
	e = b_new + len;
	strncpy(e, devstring, len);
	ee = e + len;
	i = 0;
	while (e < ee && *e && i < PTL_MAX_INIT) {
		while (*e && !isalpha(*e))
			e++;
		b = e;
		while (*e && isalpha(*e))
			e++;
		*e = '\0';
		if (*b) {
			if (!strcasecmp(b, "self")) {
				devices[i++] = PTL_DEVID_SELF;
				b_new = strcpy(b_new, "self,");
				b_new += 5;
			} else if (!strcasecmp(b, "shm") ||
					!strcasecmp(b, "shmem") ||
					!strcasecmp(b, "amsh")) {
				devices[i++] = PTL_DEVID_AMSH;
				strcpy(b_new, "amsh,");
				b_new += 5;
			} else if (!strcasecmp(b, "hfi") ||
					!strcasecmp(b, "ipath") ||
					!strcasecmp(b, "ips")) {
				devices[i++] = PTL_DEVID_IPS;
				strcpy(b_new, "ips,");
				b_new += 4;
			} else {
				err = psmi_handle_error(NULL, PSM2_PARAM_ERR,
							"%s set in environment variable PSM_PTL_DEVICES=\"%s\" "
							"is not one of the recognized PTL devices (%s)",
							b, devstring,
							PSMI_DEVICES_DEFAULT);
				goto fail;
			}
			e++;
		}
	}
	if (b_new != devstr)	/* we parsed something, remove trailing comma */
		*(b_new - 1) = '\0';

	_HFI_PRDBG("PSM Device allocation order: %s\n", devstr);
fail:
	if (devstr != NULL)
		psmi_free(devstr);
	return err;

}

int psmi_device_is_enabled(const int devid_enabled[PTL_MAX_INIT], int devid)
{
	int i;
	for (i = 0; i < PTL_MAX_INIT; i++)
		if (devid_enabled[i] == devid)
			return 1;
	return 0;
}

int psmi_ep_device_is_enabled(const psm2_ep_t ep, int devid)
{
	return psmi_device_is_enabled(ep->devid_enabled, devid);
}
