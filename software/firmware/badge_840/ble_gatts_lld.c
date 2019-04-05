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

#include <stdio.h>
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

#include "orchard-app.h"

ble_gatts_char_handles_t pw_handle;	
ble_gatts_char_handles_t ul_handle;	
ble_gatts_char_handles_t ch_handle;	
uint16_t ble_gatts_ides_handle;
const ble_uuid128_t ble_ides_base_uuid = {BLE_IDES_UUID_BASE};
static ble_gatts_char_handles_t mf_handle;	
static ble_gatts_char_handles_t sysid_handle;	
static ble_gatts_char_handles_t model_handle;	
static ble_gatts_char_handles_t sw_handle;	
static uint16_t ble_gatts_devid_handle;

extern uint32_t _data_start;

static void
bleGattsWriteHandle (ble_gatts_evt_write_t * req,
    ble_gatts_rw_authorize_reply_params_t * rep)
{
	rep->params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
	rep->params.write.update = 1;
	rep->params.write.offset = req->offset;
	rep->params.write.len = req->len;
	rep->params.write.p_data = req->data;

	if (req->handle == ul_handle.value_handle) {
		if (req->len > sizeof (ble_unlocks)) {
			rep->params.write.gatt_status =
			    BLE_GATT_STATUS_ATTERR_INVALID_ATT_VAL_LENGTH;
#ifdef BLE_GATTS_VERBOSE
			printf ("unlock update size incorrect\n");
#endif
		} else if (strncmp (ble_password, BLE_IDES_PASSWORD,
			    strlen (BLE_IDES_PASSWORD))) {
			rep->params.write.gatt_status =
			    BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
#ifdef BLE_GATTS_VERBOSE
			printf ("unlock update permission denied\n");
#endif
		} else {
			memcpy (&ble_unlocks, req->data, req->len);
			chEvtBroadcast (&unlocks_updated);
#ifdef BLE_GATTS_VERBOSE
			printf ("new unlock value: 0x%lx\n", ble_unlocks);
#endif
		}
	}

	if (req->handle == pw_handle.value_handle) {
		if (req->len > sizeof (ble_password)) {
			rep->params.write.gatt_status =
			    BLE_GATT_STATUS_ATTERR_INVALID_ATT_VAL_LENGTH;
#ifdef BLE_GATTS_VERBOSE
			printf ("password update size incorrect\n");
#endif
		} else if (strncmp ((char *)req->data, BLE_IDES_PASSWORD,
			    strlen (BLE_IDES_PASSWORD))) {
			rep->params.write.gatt_status =
			    BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
#ifdef BLE_GATTS_VERBOSE
			printf ("password incorrect\n");
#endif
		} else {
			memset (ble_password, 0, sizeof (ble_password));
			memcpy (ble_password, req->data, req->len);
		}
	}

	return;
}

void
bleGattsDispatch (ble_evt_t * evt)
{
	ble_gatts_evt_rw_authorize_request_t *	req;
	ble_gatts_rw_authorize_reply_params_t	rep;
#ifdef BLE_GATTS_VERBOSE
	ble_gatts_evt_exchange_mtu_request_t *	mtu;
	ble_gatts_evt_write_t *			write;
	int r;
#endif

	switch (evt->header.evt_id) {
		case BLE_GATTS_EVT_WRITE:
#ifdef BLE_GATTS_VERBOSE
			write = &evt->evt.gatts_evt.params.write;
			printf ("write to handle %x uuid %x\n",
			    write->handle, write->uuid.uuid);
#endif
			orchardAppRadioCallback (gattsWriteEvent,
			    evt, NULL, 0);
			break;
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
#ifdef BLE_GATTS_VERBOSE
			printf ("RW authorization request received\n");
#endif
			req = &evt->evt.gatts_evt.params.authorize_request;
			rep.type = req->type;
			if (req->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
				bleGattsWriteHandle (&req->request.write,
				    &rep);
#ifdef BLE_GATTS_VERBOSE
				r =
#endif
				sd_ble_gatts_rw_authorize_reply (
				    ble_conn_handle, &rep);
#ifdef BLE_GATTS_VERBOSE
				printf ("sent reply: %d\n", r);
#endif
			}
			orchardAppRadioCallback (gattsReadWriteAuthEvent,
			    evt, NULL, 0);
			break;
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
#ifdef BLE_GATTS_VERBOSE
			printf ("system attribute access pending\n");
			r = 
#endif
			sd_ble_gatts_sys_attr_set (ble_conn_handle,
			    NULL, 0, 0);
#ifdef BLE_GATTS_VERBOSE
			printf ("set attribute: %d\n", r);
#endif
			break;
		case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
#ifdef BLE_GATTS_VERBOSE
			mtu = &evt->evt.gatts_evt.params.exchange_mtu_request;
			printf ("GATTS MTU exchange: %d\n",
			    mtu->client_rx_mtu);
#endif
#ifdef BLE_GATTS_VERBOSE
			r =
#endif
			sd_ble_gatts_exchange_mtu_reply (ble_conn_handle,
			    BLE_GATT_ATT_MTU_DEFAULT);
#ifdef BLE_GATTS_VERBOSE
			printf ("MTU exchange reply: %x\n", r);
#endif
			break;
		default:
			printf ("unhandled GATTS event %d (%d)\n",
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
bleGattsStrCharAdd (uint16_t ble_gatts_handle, ble_uuid_t * uuid,
    uint8_t rw, char * str, ble_gatts_char_handles_t * char_handle,
    uint8_t * description)
{
	ble_gatts_char_md_t	ble_char_md;
	ble_gatts_attr_md_t	ble_desc_md;
	ble_gatts_attr_md_t	ble_attr_md;
	ble_gatts_attr_t	ble_attr_char_val;
	ble_gatts_char_pf_t	ble_char_pf;
	int r;

	memset (&ble_char_md, 0, sizeof(ble_char_md));
	memset (&ble_desc_md, 0, sizeof(ble_attr_md));
	memset (&ble_attr_md, 0, sizeof(ble_attr_md));
	memset (&ble_attr_char_val, 0, sizeof (ble_attr_char_val));
	memset (&ble_char_pf, 0, sizeof(ble_gatts_char_pf_t));

	/* Set up attribute metadata */

	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.write_perm);
	if (str < (char *)&_data_start)
		ble_attr_md.vloc = BLE_GATTS_VLOC_STACK;
	else
		ble_attr_md.vloc = BLE_GATTS_VLOC_USER;

	/* Write requests trigger an authorizationr request */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_attr_md.wr_auth = 1;

	/* Set up GATT attribute data */

	ble_attr_char_val.p_uuid = uuid;
	ble_attr_char_val.p_attr_md = &ble_attr_md;
	ble_attr_char_val.init_len = strlen (str);
	ble_attr_char_val.max_len = strlen (str);
	ble_attr_char_val.p_value = (uint8_t *)str;

	/* Set up GATT characteristic metadata */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_READ)
		ble_char_md.char_props.read = 1;
	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_char_md.char_props.write = 1;

	/* Set up presentation format and descrtiption string. */

	ble_char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;
	ble_char_pf.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
	/* The description and unit come from the BLE spec. */
	ble_char_pf.desc = 0x0106; /* main */
	ble_char_pf.unit = 0x2700; /* unitless */
	ble_char_md.p_char_pf = &ble_char_pf;

	if (description != NULL) {
		BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_desc_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS (&ble_desc_md.write_perm);
		ble_desc_md.vloc = BLE_GATTS_VLOC_USER;
		ble_char_md.p_char_user_desc = description;
		ble_char_md.char_user_desc_size =
		    strlen ((char *)description) + 1;
		ble_char_md.char_user_desc_max_size =
		    strlen ((char *)description) + 1;
		ble_char_md.p_user_desc_md = &ble_desc_md;
	}

	/* Add the characteristic to the stack. */

	r = sd_ble_gatts_characteristic_add (ble_gatts_handle,
	    &ble_char_md, &ble_attr_char_val, char_handle);

	return (r);
}

/*
 * Add a uint32_t type characteristic
 */

static int
bleGattsIntCharAdd (uint16_t ble_gatts_handle, ble_uuid_t * uuid,
    uint8_t rw, uint32_t * val, ble_gatts_char_handles_t * char_handle,
    uint8_t * description)
{
	ble_gatts_char_md_t	ble_char_md;
	ble_gatts_attr_md_t	ble_desc_md;
	ble_gatts_attr_md_t	ble_attr_md;
	ble_gatts_attr_t	ble_attr_char_val;
	ble_gatts_char_pf_t	ble_char_pf;
	int r;

	memset (&ble_char_md, 0, sizeof (ble_char_md));
	memset (&ble_desc_md, 0, sizeof (ble_attr_md));
	memset (&ble_attr_md, 0, sizeof (ble_attr_md));
	memset (&ble_attr_char_val, 0, sizeof (ble_attr_char_val));
	memset (&ble_char_pf, 0, sizeof (ble_gatts_char_pf_t));

	/* Set up attribute metadata */

	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_attr_md.write_perm);
	if (val < &_data_start)
		ble_attr_md.vloc = BLE_GATTS_VLOC_STACK;
	else
		ble_attr_md.vloc = BLE_GATTS_VLOC_USER;

	/* Write requests trigger an authorizationr request */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_attr_md.wr_auth = 1;

	/* Set up GATT attribute data */

	ble_attr_char_val.p_uuid = uuid;
	ble_attr_char_val.p_attr_md = &ble_attr_md;
	ble_attr_char_val.init_len = sizeof (val);
	ble_attr_char_val.max_len = sizeof (val);
	ble_attr_char_val.p_value = (uint8_t *)val;

	/* Set up GATT characteristic metadata */

	if (rw & BLE_GATTS_AUTHORIZE_TYPE_READ)
		ble_char_md.char_props.read = 1;
	if (rw & BLE_GATTS_AUTHORIZE_TYPE_WRITE)
		ble_char_md.char_props.write = 1;

	/* Set up presentation format and descrtiption string. */

	ble_char_pf.format = BLE_GATT_CPF_FORMAT_UINT32;
	ble_char_pf.name_space = BLE_GATT_CPF_NAMESPACE_BTSIG;
	/* The description and unit come from the BLE spec. */
	ble_char_pf.desc = 0x0106; /* main */
	ble_char_pf.unit = 0x2700; /* unitless */
	ble_char_md.p_char_pf = &ble_char_pf;

	if (description != NULL) {
		BLE_GAP_CONN_SEC_MODE_SET_OPEN (&ble_desc_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS (&ble_desc_md.write_perm);
		ble_desc_md.vloc = BLE_GATTS_VLOC_USER;
		ble_char_md.p_char_user_desc = description;
		ble_char_md.char_user_desc_size =
		    strlen ((char *)description) + 1;
		ble_char_md.char_user_desc_max_size =
		    strlen ((char *)description) + 1;
		ble_char_md.p_user_desc_md = &ble_desc_md;
	}

	/* Add the characteristic to the stack. */

	r = sd_ble_gatts_characteristic_add (ble_gatts_handle,
	    &ble_char_md, &ble_attr_char_val, char_handle);

	return (r);
}

void
bleGattsStart (void)
{
	ble_uuid_t		ble_service_uuid;
	int r;

	/*
	 * In order for GATTS to work, we need to do three things:
	 * 1) Register a vendor-specific UUID (optional)
	 * 2) Register a service under that UUID
	 * 3) Add one or more characteristics to the service
	 */

	/* Add device ID the service to the stack */

	ble_service_uuid.type = BLE_UUID_TYPE_BLE;
	ble_service_uuid.uuid = BLE_UUID_DEVICE_INFORMATION_SERVICE;

	r = sd_ble_gatts_service_add (BLE_GATTS_SRVC_TYPE_PRIMARY,
	    &ble_service_uuid, &ble_gatts_devid_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding GATT service failed\n");

	/* Now add the characteristics for the device ID service */

	ble_service_uuid.uuid = BLE_UUID_MANUFACTURER_NAME_STRING_CHAR;

	r = bleGattsStrCharAdd (ble_gatts_devid_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_MANUFACTUTER_STRING, &mf_handle, NULL);
 
	if (r != NRF_SUCCESS)
		printf ("Adding manufacturer string "
		    "characteristic failed (%x)\n", r);

	ble_service_uuid.uuid = BLE_UUID_MODEL_NUMBER_STRING_CHAR;

	r = bleGattsStrCharAdd (ble_gatts_devid_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_MODEL_NUMBER_STRING, &model_handle, NULL);

	if (r != NRF_SUCCESS)
		printf ("Adding model string "
		    "characteristic failed (%x)\n", r);

	ble_service_uuid.uuid = BLE_UUID_SYSTEM_ID_CHAR;

	r = bleGattsStrCharAdd (ble_gatts_devid_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_SYSTEM_ID_STRING, &sysid_handle, NULL);

	if (r != NRF_SUCCESS)
		printf ("Adding system ID string "
		    "characteristic failed (%x)\n", r);

	ble_service_uuid.uuid = BLE_UUID_SOFTWARE_REVISION_STRING_CHAR;

	r = bleGattsStrCharAdd (ble_gatts_devid_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ,
	    BLE_SW_VERSION_STRING, &sw_handle, NULL);

	if (r != NRF_SUCCESS)
		printf ("Adding software version string "
		    "characteristic failed (%x)\n", r);

	/* Add our custom service UUID */

	memset (&ble_service_uuid, 0, sizeof(ble_service_uuid));

	r = sd_ble_uuid_vs_add (&ble_ides_base_uuid, &ble_service_uuid.type);

	/* Add the service to the stack */

	ble_service_uuid.uuid = BLE_UUID_IDES_BADGE_SERVICE;

	r = sd_ble_gatts_service_add (BLE_GATTS_SRVC_TYPE_PRIMARY,
	    &ble_service_uuid, &ble_gatts_ides_handle);

	if (r != NRF_SUCCESS)
		printf ("Adding custom GATT service failed\n");

	/* Now add characteristics for our custom service */

	ble_service_uuid.uuid = BLE_UUID_IDES_BADGE_PASSWORD;

	r = bleGattsStrCharAdd (ble_gatts_ides_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ | BLE_GATTS_AUTHORIZE_TYPE_WRITE,
            ble_password, &pw_handle, (uint8_t *)"Security Password");
 
	if (r != NRF_SUCCESS)
		printf ("Adding password string "
		    "characteristic failed (%x)\n", r);

	ble_service_uuid.uuid = BLE_UUID_IDES_BADGE_UNLOCKS;

	r = bleGattsIntCharAdd (ble_gatts_ides_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ | BLE_GATTS_AUTHORIZE_TYPE_WRITE,
	    &ble_unlocks, &ul_handle, (uint8_t *)"Unlocks");
 
	if (r != NRF_SUCCESS)
		printf ("Adding unlock "
		    "characteristic failed (%x)\n", r);

	ble_service_uuid.uuid = BLE_UUID_IDES_BADGE_CHATREQUEST;

	r = bleGattsIntCharAdd (ble_gatts_ides_handle, &ble_service_uuid,
	    BLE_GATTS_AUTHORIZE_TYPE_READ | BLE_GATTS_AUTHORIZE_TYPE_WRITE,
	    &ble_chatreq, &ch_handle, (uint8_t *)"Chat Request");
 
	if (r != NRF_SUCCESS)
		printf ("Adding chat request "
		    "characteristic failed (%x)\n", r);

	return;
}
