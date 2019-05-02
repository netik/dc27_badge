/*-
 * Copyright (c) 2017
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
#include "ble_l2cap.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"

#include "orchard-app.h"

#include "badge.h"

static uint8_t ble_rx_buf[BLE_IDES_L2CAP_MTU];
uint16_t ble_local_cid;

static void bleL2CapSetupReply (ble_l2cap_evt_ch_setup_request_t *);

void
bleL2CapDispatch (ble_evt_t * evt)
{
#ifdef BLE_L2CAP_VERBOSE
	ble_l2cap_evt_ch_setup_refused_t * refused;
	ble_l2cap_evt_ch_setup_t * setup;
#endif
	ble_l2cap_evt_ch_setup_request_t * request;
	ble_l2cap_evt_ch_rx_t * rx;
	ble_data_t rx_data;

	switch (evt->header.evt_id) {
		case BLE_L2CAP_EVT_CH_SETUP_REQUEST:
			request = &evt->evt.l2cap_evt.params.ch_setup_request;
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP setup requested %x\n",
			    evt->evt.l2cap_evt.local_cid);
			printf ("MTU: %d peer MPS: %d "
			     "TX MPS: %d credits: %d ",
				request->tx_params.tx_mtu,
				request->tx_params.peer_mps,
				request->tx_params.tx_mps,
				request->tx_params.credits);
			printf ("PSM: %x\n", request->le_psm);
#endif
			ble_local_cid = evt->evt.l2cap_evt.local_cid;
			bleL2CapSetupReply (request);
			break;

		case BLE_L2CAP_EVT_CH_SETUP_REFUSED:
#ifdef BLE_L2CAP_VERBOSE
                        refused = &evt->evt.l2cap_evt.params.ch_setup_refused;
			printf ("L2CAP setup refused: %x %x\n",
			    refused->source, refused->status);
#endif
			orchardAppRadioCallback (l2capConnectRefusedEvent,
			    evt, NULL, 0);
			break;

		case BLE_L2CAP_EVT_CH_SETUP:
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP setup completed\n");
			setup = &evt->evt.l2cap_evt.params.ch_setup;
			printf ("MTU: %d peer MPS: %d "
			     "TX MPS: %d credits: %d\n",
				setup->tx_params.tx_mtu,
				setup->tx_params.peer_mps,
				setup->tx_params.tx_mps,
				setup->tx_params.credits);
#endif
			ble_local_cid = evt->evt.l2cap_evt.local_cid;
			orchardAppRadioCallback (l2capConnectEvent,
			    evt, NULL, 0);
			break;

		case BLE_L2CAP_EVT_CH_RELEASED:
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP channel release\n");
#endif
			ble_local_cid = BLE_L2CAP_CID_INVALID;
			orchardAppRadioCallback (l2capDisconnectEvent,
			    evt, NULL, 0);
			break;

		case BLE_L2CAP_EVT_CH_SDU_BUF_RELEASED:
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP channel SDU buffer released\n");
#endif
			break;

		case BLE_L2CAP_EVT_CH_CREDIT:
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP credit received\n");
#endif
			orchardAppRadioCallback (l2capTxDoneEvent, evt,
			    NULL, 0);
			break;

		case BLE_L2CAP_EVT_CH_RX:
			rx = &evt->evt.l2cap_evt.params.rx;
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP SDU received\n");
			printf ("DATA RECEIVED: [%s]\n", rx->sdu_buf.p_data);
#endif
			orchardAppRadioCallback (l2capRxEvent, evt,
			    rx->sdu_buf.p_data, rx->sdu_buf.len);
			rx_data.p_data = ble_rx_buf;
			rx_data.len = BLE_IDES_L2CAP_MTU;
			sd_ble_l2cap_ch_rx (ble_conn_handle,
			    evt->evt.l2cap_evt.local_cid, &rx_data);
			break;

		case BLE_L2CAP_EVT_CH_TX:
#ifdef BLE_L2CAP_VERBOSE
			printf ("L2CAP SDU transmitted\n");
#endif
			orchardAppRadioCallback (l2capTxEvent, evt,
			    NULL, 0);
			break;

		default:
			printf ("unhandled L2CAP event: %d (%d)\n",
			    evt->header.evt_id - BLE_L2CAP_EVT_BASE,
			    evt->header.evt_id);
			break;
	}

	return;
}

int
bleL2CapConnect (uint16_t psm)
{
	ble_l2cap_ch_setup_params_t params;
	uint16_t cid;
	int r;

	params.rx_params.rx_mtu = BLE_IDES_L2CAP_MTU;
	params.rx_params.rx_mps = BLE_IDES_L2CAP_MPS;
	params.rx_params.sdu_buf.p_data = ble_rx_buf;
	params.rx_params.sdu_buf.len = BLE_IDES_L2CAP_MTU;

	params.le_psm = psm;
	params.status = 0;

	cid = BLE_L2CAP_CID_INVALID;

	r = sd_ble_l2cap_ch_setup (ble_conn_handle, &cid, &params);

	if (r != NRF_SUCCESS)
		printf ("L2CAP connect failed (0x%x)\n", r);

	return (r);
}

int
bleL2CapDisconnect (uint16_t cid)
{
	int r;

	r = sd_ble_l2cap_ch_release (ble_conn_handle, cid);

	if (r != NRF_SUCCESS)
		printf ("L2CAP disconnect failed (0x%x)\n", r);

	return (r);
}

static void
bleL2CapSetupReply (ble_l2cap_evt_ch_setup_request_t * request)
{
	ble_l2cap_ch_setup_params_t params;
	int r;

	params.rx_params.rx_mtu = BLE_IDES_L2CAP_MTU;
	params.rx_params.rx_mps = BLE_IDES_L2CAP_MPS;
	params.rx_params.sdu_buf.p_data = ble_rx_buf;
	params.rx_params.sdu_buf.len = BLE_IDES_L2CAP_MTU;

	params.le_psm = request->le_psm;
	params.status = 0;

	r = sd_ble_l2cap_ch_setup (ble_conn_handle, &ble_local_cid, &params);

	if (r != NRF_SUCCESS)
		printf ("L2CAP reply failed (0x%x)\n", r);

	return;
}

int
bleL2CapSend (uint8_t * txbuf, uint16_t txlen)
{
	ble_data_t data;
	int r;

	data.p_data = txbuf;
	data.len = txlen;

	r = sd_ble_l2cap_ch_tx (ble_conn_handle, ble_local_cid, &data);

	if (r != NRF_SUCCESS)
		printf ("L2CAP tx failed (0x%x)\n", r);

	return (r);
}

void
bleL2CapStart (void)
{
	ble_local_cid = BLE_L2CAP_CID_INVALID;

	return;
}
