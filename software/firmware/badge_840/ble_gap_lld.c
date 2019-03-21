/*-
 * Copyright (c) 2017-2019
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
#include "ble_gap.h"
#include "ble_hci.h"

#include "ble_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"

#include "orchard-app.h"

#include "badge.h"

static ble_gap_lesc_p256_pk_t peer_pk;
static ble_gap_lesc_p256_pk_t my_pk;

static uint8_t * bleGapAdvBlockStart (uint8_t *);
static uint32_t bleGapAdvBlockAdd (void * pElem, uint8_t len, uint8_t etype,
	uint8_t * pkt, uint8_t * size);
static uint32_t bleGapAdvBlockFinish (uint8_t *, uint8_t);
static int bleGapScanStart (void);
static int bleGapAdvStart (void);

static uint8_t ble_adv_block[BLE_GAP_ADV_MAX_SIZE];
static uint8_t ble_scan_block[BLE_GAP_ADV_MAX_SIZE];
static uint8_t ble_scan_buffer[BLE_GAP_SCAN_BUFFER_EXTENDED_MAX];
static uint8_t ble_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

uint16_t ble_conn_handle = BLE_CONN_HANDLE_INVALID;
uint8_t ble_gap_role;
ble_gap_addr_t ble_peer_addr;

static int
bleGapScanStart (void)
{
	ble_gap_scan_params_t scan;
	ble_data_t scan_buffer;

	int r;

	memset(&scan, 0, sizeof(scan));

	scan.extended = 0;
	scan.scan_phys = BLE_GAP_PHY_AUTO;
	scan.timeout = BLE_IDES_SCAN_TIMEOUT;
	scan.window = MSEC_TO_UNITS(100, UNIT_0_625_MS);
	scan.interval = MSEC_TO_UNITS(200, UNIT_0_625_MS);
	scan.active = 1;

	scan_buffer.p_data = ble_scan_buffer;
	scan_buffer.len = sizeof(ble_scan_buffer);

	r = sd_ble_gap_scan_start (&scan, &scan_buffer);

	if (r != NRF_SUCCESS && r != NRF_ERROR_INVALID_STATE)
		printf ("GAP failed to start scanning: %d\r\n", r);

	return (r);
}

static int
bleGapAdvStart (void)
{
	uint8_t * pkt;
	uint8_t size;
	uint16_t val;
	int r;

	pkt = bleGapAdvBlockStart (&size);

	/* Set our appearance */

	val = BLE_APPEARANCE_DC27;
	r = bleGapAdvBlockAdd (&val, 2,
	    BLE_GAP_AD_TYPE_APPEARANCE, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set manufacturer ID */

	val = BLE_COMPANY_ID_IDES;
	r = bleGapAdvBlockAdd (&val, 2,
	    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set flags */

	val = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED |
            BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
	r = bleGapAdvBlockAdd (&val, 1,
	    BLE_GAP_AD_TYPE_FLAGS, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Set role */

	val = 2 /*BLE_GAP_ROLE_PERIPH*/;
	r = bleGapAdvBlockAdd (&val, 1,
	    BLE_GAP_AD_TYPE_LE_ROLE, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Advertise device info UUID */

	val = BLE_UUID_DEVICE_INFORMATION_SERVICE;
	r = bleGapAdvBlockAdd (&val, 2,
	    BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, pkt, &size);

	if (r != NRF_SUCCESS)
		return (r);

	/* Begin advertisement */

	r = bleGapAdvBlockFinish (pkt, BLE_GAP_ADV_MAX_SIZE - size);

	return (r);
}

void
bleGapDispatch (ble_evt_t * evt)
{
	ble_gap_sec_keyset_t sec_keyset;
	ble_gap_sec_params_t sec_params;
	ble_gap_data_length_limitation_t dlim;
	ble_gap_evt_timeout_t * timeout;
	ble_gap_evt_data_length_update_request_t * dup;
	ble_gap_evt_conn_param_update_request_t * update;
	ble_gap_adv_report_type_t rtype;
#ifdef BLE_GAP_VERBOSE
	ble_gap_evt_conn_sec_update_t * sec;
	ble_gap_conn_sec_mode_t * secmode;
#endif
	ble_gap_addr_t * addr;
	ble_data_t scan_buffer;
	ble_peer_entry * p;
	uint8_t * data;
	uint8_t datalen;
	uint8_t * name;
	uint8_t len;
	int r;

	switch (evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			ble_conn_handle = evt->evt.gap_evt.conn_handle;
			addr = &evt->evt.gap_evt.params.connected.peer_addr;
			memcpy (&ble_peer_addr, addr, sizeof(ble_gap_addr_t));
			ble_gap_role = evt->evt.gap_evt.params.connected.role;
#ifdef BLE_GAP_VERBOSE
			printf ("gap connected (handle: %x) ",
			    ble_conn_handle);
			printf ("peer: %x:%x:%x:%x:%x:%x ",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0]);
			printf ("role; %d\r\n", ble_gap_role);
#endif
			sd_ble_gap_tx_power_set (BLE_GAP_TX_POWER_ROLE_CONN,
			    ble_conn_handle, 8);
			if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
				bleGapScanStart ();
			orchardAppRadioCallback (connectEvent, evt, NULL, 0);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
#ifdef BLE_GAP_VERBOSE
			printf ("GAP disconnected...\r\n");
#endif
			ble_conn_handle = BLE_CONN_HANDLE_INVALID;
			orchardAppRadioCallback (disconnectEvent, evt,
			    NULL, 0);
			r = sd_ble_gap_adv_stop (ble_adv_handle);
#ifdef notdef
			if (r != NRF_SUCCESS) {
				printf ("GAP stop advertisement after "
				    "disconnect failed! 0x%x\r\n", r);
			}
#endif
			r = sd_ble_gap_adv_start (ble_adv_handle,
			    BLE_IDES_APP_TAG);
			if (r != NRF_SUCCESS) {
				printf ("GAP restart advertisement after "
				    "disconnect failed! 0x%x\r\n", r);
			}
			break;

		case BLE_GAP_EVT_AUTH_STATUS:
#ifdef BLE_GAP_VERBOSE
			printf ("GAP auth status... (%d)\r\n",
			    evt->evt.gap_evt.params.auth_status.auth_status);
#endif
			break;

		case BLE_GAP_EVT_CONN_SEC_UPDATE:
#ifdef BLE_GAP_VERBOSE
			sec = &evt->evt.gap_evt.params.conn_sec_update;
			secmode = &sec->conn_sec.sec_mode;
			printf ("gap connections security update...\r\n");
			printf ("security mode: %d level: %d keylen: %d\r\n",
			    secmode->sm, secmode->lv,
			    sec->conn_sec.encr_key_size);
#endif
			break;

		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			memset (&sec_keyset, 0, sizeof(sec_keyset));
			memset (&sec_params, 0, sizeof(sec_params));
		        sec_keyset.keys_own.p_pk  = &my_pk;
        		sec_keyset.keys_peer.p_pk = &peer_pk;
			sec_params.min_key_size = 7;
			sec_params.max_key_size = 16;
			sec_params.io_caps = BLE_GAP_IO_CAPS_NONE;
#ifdef BLE_GAP_VERBOSE
			r =
#endif
			sd_ble_gap_sec_params_reply (ble_conn_handle,
			    BLE_GAP_SEC_STATUS_SUCCESS, &sec_params,
			    &sec_keyset);
#ifdef BLE_GAP_VERBOSE
			printf ("gap security param request...(%d)\r\n", r);
#endif
			break;
		case BLE_GAP_EVT_ADV_REPORT:
			addr = &evt->evt.gap_evt.params.adv_report.peer_addr;
			len = evt->evt.gap_evt.params.adv_report.data.len;
			name = evt->evt.gap_evt.params.adv_report.data.p_data;
			datalen = evt->evt.gap_evt.params.adv_report.data.len;
			data = evt->evt.gap_evt.params.adv_report.data.p_data;
			rtype = evt->evt.gap_evt.params.adv_report.type;

			if (bleGapAdvBlockFind (&name, &len,
			    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME) !=
			    NRF_SUCCESS) {
				name = NULL;
				len = 0;
			}

			p = blePeerFind (addr->addr);
			blePeerAdd (addr->addr, name, len,
			    evt->evt.gap_evt.params.adv_report.rssi);

			/*
			 * If this packet (either advertisement or scan
			 * response) contains a name, or
			 * if this is an advertisement with no name and
			 * the device is not scannable (so we know we'll
			 * never get a name), or
			 * if this is a scan response and we've already
			 * gotten an advertisement and neither one had
			 * a name,
			 * then send an advertisement notification to
			 * our apps.
			 */

			if (name != NULL ||
			    (rtype.scannable == 0) ||
			    (rtype.scan_response && p != NULL)) {
				orchardAppRadioCallback (advertisementEvent,
				    evt, data, datalen);
			}

			/*
			 * Ok, this is a little nutty. In BLE, when you
			 * advertise, you broadcast a single packet. This
			 * packet can contain your appearance and supported
			 * UUIDs. It can also contain the device name. And
			 * it can also specify if the device is scannable
			 * or not. If devices are scannable, then you can
			 * send a scan request to them, and they will reply
			 * with a scan response packet, which is another 31
			 * bytes and may contain additional information that
			 * wouldn't fit in the advertisement broadcast.
			 *
			 * It seems that the way the Nordic SoftDevice works,
			 * once you receive an advertisement event indicating
			 * scannability, you have to immediately call the
			 * scan start function again, otherwise you won't
			 * receive the scan response packet. I don't think
			 * this is clearly documented anywhere; like many
			 * subtle details, it's just strongly implied by
			 * the example code in the Nordic SDK.
			 */

			memset (ble_scan_buffer, 0, sizeof(ble_scan_buffer));
			scan_buffer.p_data = ble_scan_buffer;
			scan_buffer.len = sizeof(ble_scan_buffer);

			sd_ble_gap_scan_start (NULL, &scan_buffer);
#ifdef BLE_GAP_SCAN_VERBOSE
			printf ("GAP advertisement report, type: %d...\r\n",
			    rtype);
			printf ("Peer: %x:%x:%x:%x:%x:%x rssi: %d\r\n",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0],
			    evt->evt.gap_evt.params.adv_report.rssi);
			printf ("Report type is: ");
			if (rtype.connectable)
				printf ("connectable ");
			if (rtype.scannable)
				printf ("scannable ");
			if (rtype.directed)
				printf ("directed ");
			else
				printf ("undirected ");
			if (rtype.scan_response)
				printf ("scan response ");
			printf ("\r\n");
#endif
			break;
		case BLE_GAP_EVT_SCAN_REQ_REPORT:
#ifdef BLE_GAP_SCAN_VERBOSE
			addr =
			    &evt->evt.gap_evt.params.scan_req_report.peer_addr;
			printf ("GAP scan request received from: ");
			printf ("%x:%x:%x:%x:%x:%x rssi: %d\r\n",
			    addr->addr[5], addr->addr[4], addr->addr[3],
			    addr->addr[2], addr->addr[1], addr->addr[0],
			    evt->evt.gap_evt.params.scan_req_report.rssi);
#endif
			break;
		case BLE_GAP_EVT_TIMEOUT:
			timeout = &evt->evt.gap_evt.params.timeout;
#ifdef BLE_GAP_TIMEOUT_VERBOSE
			printf ("GAP timeout event, src: %d\r\n",
			    timeout->src);
#endif
			if (timeout->src == BLE_GAP_TIMEOUT_SRC_SCAN)
				bleGapScanStart ();
			if (timeout->src == BLE_GAP_TIMEOUT_SRC_CONN) {
				orchardAppRadioCallback (connectTimeoutEvent,
			 	     evt, NULL, 0);
				ble_conn_handle = BLE_CONN_HANDLE_INVALID;
				bleGapStart ();
			}
			break;

		case BLE_GAP_EVT_ADV_SET_TERMINATED:
#ifdef BLE_GAP_TIMEOUT_VERBOSE
			printf ("GAP advertisement timeout\r\n");
#endif
			if (ble_conn_handle == BLE_CONN_HANDLE_INVALID) {
				r = sd_ble_gap_adv_start (ble_adv_handle,
				    BLE_IDES_APP_TAG);
				if (r != NRF_SUCCESS)
					printf ("GAP restart advertisement "
					    "failed! 0x%x\r\n", r);
			}
			break;

		case BLE_GAP_EVT_CONN_PARAM_UPDATE:
#ifdef BLE_GAP_VERBOSE
			printf ("GAP connection parameter update\r\n");
#endif
			break;

		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
#ifdef BLE_GAP_VERBOSE
			printf ("GAP connection parameter update request\r\n");
#endif
			update =
			    &evt->evt.gap_evt.params.conn_param_update_request;

			sd_ble_gap_conn_param_update (ble_conn_handle,
			    &update->conn_params);
			break;

		case BLE_GAP_EVT_PHY_UPDATE:
#ifdef BLE_GAP_VERBOSE
			printf ("GAP PHY update completed, ");
			printf ("RX PHY: %d TX PHY: %d status: %d\r\n",
			    evt->evt.gap_evt.params.phy_update.rx_phy,
			    evt->evt.gap_evt.params.phy_update.tx_phy,
			    evt->evt.gap_evt.params.phy_update.status);
#endif
			break;

		case  BLE_GAP_EVT_DATA_LENGTH_UPDATE:
#ifdef BLE_GAP_VERBOSE
			printf ("Data length update\r\n");
#endif
			break;

		case  BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
			dup =
			  &evt->evt.gap_evt.params.data_length_update_request;
			memset (&dlim, 0, sizeof(dlim));
			r = sd_ble_gap_data_length_update (ble_conn_handle,
			    &dup->peer_params, &dlim);
#ifdef BLE_GAP_VERBOSE
			printf ("Data length update: %d\n", r);
#endif
			if (r != NRF_SUCCESS) {
				printf ("Data length update failed, "
				   "RX overrun: %d TX overrun: %d\r\n",
				    dlim.rx_payload_limited_octets,
				    dlim.tx_payload_limited_octets);
			}
			break;

		default:
			printf ("invalid GAP event %d (%d)\r\n",
			    evt->header.evt_id - BLE_GAP_EVT_BASE,
			    evt->header.evt_id);
			break;
	}

	return;
}

static uint8_t *
bleGapAdvBlockStart (uint8_t * size)
{
	uint8_t * pkt;

	pkt = ble_adv_block;
	*size = BLE_GAP_ADV_MAX_SIZE;
	memset (pkt, 0, BLE_GAP_ADV_MAX_SIZE);

	return (pkt);
}

static uint32_t
bleGapAdvBlockAdd (void * pElem, uint8_t len, uint8_t etype,
	uint8_t * pkt, uint8_t * size)
{
	uint8_t * p;

	if ((len + 2) > *size)
            return (NRF_ERROR_NO_MEM);

	p = pkt;
	p += BLE_GAP_ADV_MAX_SIZE - *size;
	p[0] = len + 1;
	p[1] = etype;
	memcpy (&p[2], pElem, len);

	*size -= (2 + len);

	return (NRF_SUCCESS);
}

static uint32_t
bleGapAdvBlockFinish (uint8_t * pkt, uint8_t len)
{
	ble_gap_adv_params_t adv_params;
	ble_gap_adv_data_t adv_data;
	uint32_t r;
	uint8_t * p2;
	uint8_t s2;
	uint8_t ble_name[BLE_GAP_DEVNAME_MAX_LEN];
	uint16_t namelen = BLE_GAP_DEVNAME_MAX_LEN;

	memset (&adv_params, 0, sizeof(adv_params));
	memset (&adv_data, 0, sizeof(adv_data));

	adv_data.adv_data.p_data = pkt;
	adv_data.adv_data.len = len;

	p2 = ble_scan_block;
	s2 = BLE_GAP_ADV_MAX_SIZE;
	memset (p2, 0, BLE_GAP_ADV_MAX_SIZE);

	/*
	 * Set our full name. We use the scan data block for this,
	 * which allows us to have a full 31 bytes, separate from
	 * the advertisement packet.
	 */

	sd_ble_gap_device_name_get (ble_name, &namelen);

	r = bleGapAdvBlockAdd (ble_name, namelen,
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, p2, &s2);

	adv_data.scan_rsp_data.p_data = p2;
	adv_data.scan_rsp_data.len = BLE_GAP_ADV_MAX_SIZE - s2;

	adv_params.properties.type =
	    BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;

	adv_params.interval = MSEC_TO_UNITS(33, UNIT_0_625_MS);
	adv_params.duration = MSEC_TO_UNITS(2000, UNIT_10_MS);
	adv_params.filter_policy =  BLE_GAP_SCAN_FP_ACCEPT_ALL;
	adv_params.primary_phy = BLE_GAP_PHY_AUTO;
	adv_params.set_id = 1;
	/*adv_params.scan_req_notification = 1;*/

	r = sd_ble_gap_adv_set_configure (&ble_adv_handle,
	    &adv_data, &adv_params);

	if (r != NRF_SUCCESS)
            return (r);

	r = sd_ble_gap_adv_start (ble_adv_handle, BLE_IDES_APP_TAG);

	sd_ble_gap_tx_power_set (BLE_GAP_TX_POWER_ROLE_ADV, ble_adv_handle, 8);

	return (r);
}

uint32_t
bleGapAdvBlockFind (uint8_t ** pkt, uint8_t * len, uint8_t id)
{
	uint8_t * p;
	uint8_t l = 0;
	uint8_t t = 0;

	if (*len == 0 || *len > BLE_GAP_ADV_MAX_SIZE)
		return (NRF_ERROR_INVALID_PARAM);

	p = *pkt;

	while (p < (*pkt + *len)) {
		l = p[0];
		t = p[1];

		/* Sanity check. */

		if (l == 0 || l > BLE_GAP_ADV_MAX_SIZE)
			return (NRF_ERROR_INVALID_PARAM);

		if (t == id)
			break;
		else
			p += (l + 1);
	}

	if (t == id && l != 0) {
		*pkt = p + 2;
		*len = l - 1;
		return (NRF_SUCCESS);
	}

	return (NRF_ERROR_NOT_FOUND);
}

void
bleGapStart (void)
{
	ble_gap_conn_sec_mode_t perm;
	uint8_t * ble_name = (uint8_t *)BLE_NAME_IDES;
	int r;

	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&perm);
	r = sd_ble_gap_device_name_set (&perm, ble_name,
	    strlen ((char *)ble_name));

	if (r != NRF_SUCCESS)
		printf ("GAP device name set failed: 0x%x\r\n", r);

	r = sd_ble_gap_appearance_set (BLE_APPEARANCE_DC27);

	if (r != NRF_SUCCESS)
		printf ("GAP appearance set failed: 0x%x\r\n", r);

	bleGapAdvStart ();
	bleGapScanStart ();

	return;
}

int
bleGapConnect (ble_gap_addr_t * peer)
{
	ble_gap_scan_params_t sparams;
	ble_gap_conn_params_t cparams;
	int r;

	/* Don't try to connect if already connected. */

	if (ble_conn_handle != BLE_CONN_HANDLE_INVALID)
		return (NRF_ERROR_INVALID_STATE);

	memset (&sparams, 0, sizeof(sparams));
	memset (&cparams, 0, sizeof(cparams));

	sparams.extended = 0;
	sparams.scan_phys = BLE_GAP_PHY_AUTO;
	sparams.timeout = BLE_IDES_SCAN_TIMEOUT;
	sparams.window = MSEC_TO_UNITS(100, UNIT_0_625_MS);
	sparams.interval = MSEC_TO_UNITS(200, UNIT_0_625_MS);

	cparams.min_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS);
	cparams.max_conn_interval = MSEC_TO_UNITS(10, UNIT_1_25_MS);
	cparams.slave_latency = 0;
	cparams.conn_sup_timeout = MSEC_TO_UNITS(500, UNIT_10_MS);

	r = sd_ble_gap_connect (peer, &sparams, &cparams, BLE_IDES_APP_TAG);

	if (r != NRF_SUCCESS) {
		printf ("GAP connect failed: 0x%x\r\n", r);
		bleGapStart ();
		return (r);
	}

	return (NRF_SUCCESS);
}

int
bleGapDisconnect (void)
{
	int r;

	/* Don't try to disconnect if already disconnected. */

	if (ble_conn_handle == BLE_CONN_HANDLE_INVALID)
		return (NRF_ERROR_INVALID_STATE);

	r = sd_ble_gap_disconnect (ble_conn_handle,
	    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

	if (r != NRF_SUCCESS)
		printf ("GAP disconnect failed: 0x%x\r\n", r);

	bleGapAdvStart ();
	return (r);
}
