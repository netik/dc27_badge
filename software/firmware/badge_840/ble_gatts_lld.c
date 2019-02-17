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

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gatts.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_gatts_lld.h"

#include "badge.h"

static ble_gatts_char_handles_t mf_handle;	
static ble_gatts_char_handles_t sysid_handle;	
static ble_gatts_char_handles_t model_handle;	
static ble_gatts_char_handles_t sw_handle;	
static uint16_t ble_gatts_handle;

void
bleGattsDispatch (ble_evt_t * evt)
{
	ble_gatts_evt_rw_authorize_request_t *	req;
	ble_gatts_rw_authorize_reply_params_t	rep;
#ifdef BLE_GATTS_VERBOSE
	ble_gatts_evt_write_t *			write;
	int r;
#endif

	switch (evt->header.evt_id) {
		case BLE_GATTS_EVT_WRITE:
#ifdef BLE_GATTS_VERBOSE
			write = &evt->evt.gatts_evt.params.write;
			printf ("write to handle %x uuid %x\r\n",
			    write->handle, write->uuid.uuid);
#endif
			break;
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
#ifdef BLE_GATTS_VERBOSE
			printf ("RW authorization request received\r\n");
#endif
			req = &evt->evt.gatts_evt.params.authorize_request;
			rep.type = req->type;
			if (req->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
				rep.params.write.gatt_status =
				    BLE_GATT_STATUS_SUCCESS;
				rep.params.write.update = 1;
				rep.params.write.offset =
				    req->request.write.offset;
				rep.params.write.len =
				    req->request.write.len;
				rep.params.write.p_data =
				    req->request.write.data;
#ifdef BLE_GATTS_VERBOSE
				r =
#endif
				sd_ble_gatts_rw_authorize_reply (
				    ble_conn_handle, &rep);
#ifdef BLE_GATTS_VERBOSE
				printf ("sent reply: %d\r\n", r);
#endif
			}
			break;
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
#ifdef BLE_GATTS_VERBOSE
			printf ("system attribute access pending\r\n");
			r = 
#endif
			sd_ble_gatts_sys_attr_set (ble_conn_handle,
			    NULL, 0, 0);
#ifdef BLE_GATTS_VERBOSE
			printf ("set attribute: %d\r\n", r);
#endif
			break;
		default:
			printf ("invalid GATTS event %d (%d)\r\n",
			    evt->header.evt_id - BLE_GATTS_EVT_BASE,
			    evt->header.evt_id);
			break;
	}
	return;
}

/*
 * Add a string type characteristic
 */

static int
bleGattsStrCharAdd (uint16_t ble_gatts_handle, uint16_t uuid,
    uint8_t rw, char * str, ble_gatts_char_handles_t * char_handle)
{
	ble_gatts_char_md_t	ble_char_md;
	ble_gatts_attr_md_t	ble_attr_md;
	ble_gatts_attr_t	ble_attr_char_val;
	ble_uuid_t		ble_char_uuid;
	int r;

	memset (&ble_char_md, 0, sizeof(ble_char_md));
	memset (&ble_attr_md, 0, sizeof (ble_attr_md));
	memset (&ble_attr_char_val, 0, sizeof (ble_attr_char_val));

	/* Set up attribute metadata */

	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.write_perm);
	ble_attr_md.vloc = BLE_GATTS_VLOC_STACK;

	/* Write requests trigger an authorizationr request */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_attr_md.wr_auth = 1;

	/* Set up GATT attribute data */

	ble_char_uuid.uuid = uuid;
	ble_char_uuid.type = BLE_UUID_TYPE_BLE;

	ble_attr_char_val.p_uuid = &ble_char_uuid;
	ble_attr_char_val.p_attr_md = &ble_attr_md;
	ble_attr_char_val.init_len = strlen (str);
	ble_attr_char_val.max_len = strlen (str);
	ble_attr_char_val.p_value = (uint8_t *)str;

	/* Set up GATT characteristic metadata */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_READ)
		ble_char_md.char_props.read = 1;
	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_char_md.char_props.write = 1;

	/* Add the characteristic to the stack. */

	r = sd_ble_gatts_characteristic_add (ble_gatts_handle,
	    &ble_char_md, &ble_attr_char_val, char_handle);

	return (r);
}

void
bleGattsStart (void)
{
	ble_uuid_t		ble_service_uuid;
#ifdef notyet
	ble_uuid128_t		ble_base_uuid = {BLE_IDES_UUID_BASE};
#endif
	int r;

	/*
	 * In order for GATTS to work, we need to do three things:
	 * 1) Register a vendor-specific UUID (optional)
	 * 2) Register a service under that UUID
	 * 3) Add one or more characteristics to the service
	 */

#ifdef notyet
	memset (&ble_service_uuid, 0, sizeof(ble_service_uuid));

	r = sd_ble_uuid_vs_add (&ble_base_uuid, &ble_service_uuid.type);

	printf ("UUID ADD: %x TYPE: %d\r\n", r, ble_service_uuid.type);
#endif

	/* Add the service to the stack */

	ble_service_uuid.type = BLE_UUID_TYPE_BLE;
	ble_service_uuid.uuid = BLE_UUID_DEVICE_INFORMATION_SERVICE;

	r = sd_ble_gatts_service_add (BLE_GATTS_SRVC_TYPE_PRIMARY,
	    &ble_service_uuid, &ble_gatts_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding GATT service failed\r\n");

	/* Now add the characteristics */

	r = bleGattsStrCharAdd (ble_gatts_handle,
            BLE_UUID_MANUFACTURER_NAME_STRING_CHAR,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_MANUFACTUTER_STRING, &mf_handle);
	    
	if (r != NRF_SUCCESS)
		printf ("Adding manufacturer string characteristic\r\n");

	r = bleGattsStrCharAdd (ble_gatts_handle,
            BLE_UUID_MODEL_NUMBER_STRING_CHAR,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_MODEL_NUMBER_STRING, &model_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding model string characteristic\r\n");

	r = bleGattsStrCharAdd (ble_gatts_handle,
            BLE_UUID_SYSTEM_ID_CHAR,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_SYSTEM_ID_STRING, &sysid_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding system ID string characteristic\r\n");

	r = bleGattsStrCharAdd (ble_gatts_handle,
            BLE_UUID_SOFTWARE_REVISION_STRING_CHAR,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_SW_VERSION_STRING, &sw_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding software version string characteristic\r\n");

	return;
}
