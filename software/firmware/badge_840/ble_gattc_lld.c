/*-
 * Copyright (c) 2019
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gattc.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"

#include "orchard-app.h"

static thread_reference_t bleGattcThreadReference;
static uint32_t bleDetectEvents;

void
bleGattcDispatch (ble_evt_t * evt)
{
	ble_gattc_evt_prim_srvc_disc_rsp_t * pri;
	ble_gattc_evt_attr_info_disc_rsp_t * attrs;
#ifdef BLE_GATTC_VERBOSE
	ble_gattc_evt_char_disc_rsp_t * chars;
	ble_gattc_evt_char_val_by_uuid_read_rsp_t * ruuid;
	ble_gattc_service_t * s;
#endif
	int i;
	int next;

	switch (evt->header.evt_id) {
		case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
			pri = &evt->evt.gattc_evt.params.prim_srvc_disc_rsp;
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC services discovery event\n");

			printf ("COUNT: %d\n", pri->count);
			for (i = 0; i < pri->count; i++) {
				s = &pri->services[i];
				printf ("UUID: %x TYPE: %d ",
				    s->uuid.uuid, s->uuid.type);
				printf ("RANGES: %d %d\n",
				    s->handle_range.start_handle,
				    s->handle_range.end_handle);
			}
#endif

			orchardAppRadioCallback (gattcServiceDiscoverEvent,
			    evt, NULL, 0);

			if (evt->evt.gattc_evt.gatt_status ==
			    BLE_GATT_STATUS_SUCCESS) {
				i = pri->count - 1;
				next = pri->services[i].handle_range.end_handle;
				sd_ble_gattc_primary_services_discover (
				    ble_conn_handle, next, NULL);
			}

			if (evt->evt.gattc_evt.gatt_status !=
			    BLE_GATT_STATUS_SUCCESS && pri->count == 0) {
				osalSysLock ();
				bleDetectEvents = BLE_GATTC_SERVICE_DISCOVERED;
				osalThreadResumeS (&bleGattcThreadReference,
				    MSG_OK);
				osalSysUnlock ();
			}
			break;
		case BLE_GATTC_EVT_CHAR_DISC_RSP:
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC characteristics discovery event\n");
			chars = &evt->evt.gattc_evt.params.char_disc_rsp;
			printf ("CHARS: %d\n", chars->count);
#endif
			orchardAppRadioCallback (gattcCharDiscoverEvent,
			    evt, NULL, 0);
			break;
		case BLE_GATTC_EVT_DESC_DISC_RSP:
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC descriptors discovery event\n");
#endif
			break;
		case BLE_GATTC_EVT_ATTR_INFO_DISC_RSP:
			attrs = &evt->evt.gattc_evt.params.attr_info_disc_rsp;
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC attribute discovery event\n");
			printf ("ATTRS: %d FORMAT: %d\n",
			    attrs->count, attrs->format);
#endif
			orchardAppRadioCallback (gattcAttrDiscoverEvent,
			    evt, NULL, 0);
			osalSysLock ();
			if (attrs->count == 1 && attrs->format ==
			    BLE_GATTC_ATTR_INFO_FORMAT_128BIT &&
			    memcpy (attrs->info.attr_info128[0].uuid.uuid128,
			        ble_ides_base_uuid.uuid128, 16) == 0)
				bleDetectEvents = BLE_GATTC_SERVICE_DISCOVERED;
			else
				bleDetectEvents = BLE_GATTC_ERROR;
			osalThreadResumeS (&bleGattcThreadReference,
			    MSG_OK);
			osalSysUnlock ();
			break;
		case BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP:
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC read by UUID\n");
			ruuid =
			  &evt->evt.gattc_evt.params.char_val_by_uuid_read_rsp;
			printf ("COUNT: %d\n", ruuid->count);
			printf ("VALUE LEN: %d\n", ruuid->value_len);
			printf ("STATUS: %x\n", evt->evt.gattc_evt.gatt_status);
#endif

			orchardAppRadioCallback (gattcCharReadByUuidEvent,
			    evt, NULL, 0);

			osalSysLock ();
			if (evt->evt.gattc_evt.gatt_status ==
			    BLE_GATT_STATUS_SUCCESS)
				bleDetectEvents = BLE_GATTC_CHAR_READ;
			else
				bleDetectEvents = BLE_GATTC_ERROR;
			osalThreadResumeS (&bleGattcThreadReference,
			    MSG_OK);
			osalSysUnlock ();

			break;
		case BLE_GATTC_EVT_READ_RSP:
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC read\n");
#endif
			orchardAppRadioCallback (gattcCharReadEvent,
			    evt, NULL, 0);
			break;
		case BLE_GATTC_EVT_WRITE_RSP:
#ifdef BLE_GATTC_VERBOSE
			printf ("GATTC write\n");
#endif
			orchardAppRadioCallback (gattcCharWriteEvent,
			    evt, NULL, 0);
			break;
		default:
			printf ("unhandled GATTC event %d (%d)\r\n",
			    evt->header.evt_id - BLE_GATTC_EVT_BASE,
			    evt->header.evt_id);
			break;
	}
	return;
}

void
bleGattcStart (void)
{
	return;
}

int
bleGattcDiscover (bool blocking)
{
	int r;
	msg_t tr = MSG_OK;
	ble_gattc_handle_range_t ranges;
	ble_uuid_t uuid;

	if (ble_conn_handle == BLE_CONN_HANDLE_INVALID)
		return (NRF_ERROR_INVALID_STATE);

	bleDetectEvents = 0;

#ifdef notdef
	r = sd_ble_gattc_primary_services_discover (ble_conn_handle,
	    BLE_UUID_TYPE_BLE, NULL);

	if (blocking == FALSE)
		return (NRF_ERROR_BUSY);

	osalSysLock ();
	if (bleDetectEvents == 0)
		tr = osalThreadSuspendTimeoutS (&bleGattcThreadReference,
		    OSAL_MS2I(5000));
	osalSysUnlock ();

	if (tr == MSG_TIMEOUT)
		return (-1);
#endif

	ranges.start_handle = ble_gatts_ides_handle;
	ranges.end_handle = ble_gatts_ides_handle;

/*
	r = sd_ble_gattc_attr_info_discover (ble_conn_handle, &ranges);
*/

	uuid.type = BLE_UUID_TYPE_BLE;
	uuid.uuid = BLE_UUID_PRIMARY_SERVICE_DECLARATION;

	r = sd_ble_gattc_char_value_by_uuid_read (ble_conn_handle,
	    &uuid, &ranges);

	if (r != NRF_SUCCESS)
		return (r);

	osalSysLock ();
	if (bleDetectEvents == 0)
		tr = osalThreadSuspendTimeoutS (&bleGattcThreadReference,
		    OSAL_MS2I(5000));
	osalSysUnlock ();

	if (tr == MSG_TIMEOUT)
		return (NRF_ERROR_TIMEOUT);

	if (bleDetectEvents == BLE_GATTC_CHAR_READ)
		return (NRF_SUCCESS);

	return (NRF_ERROR_NOT_FOUND);
}
