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

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"

#include "ble.h"
#include "ble_lld.h"
#include "ble_gap.h"
#include "ble_l2cap.h"

#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

#include "badge.h"

static void
radio_discover (BaseSequentialStream *chp, int argc, char *argv[])
{
	uint8_t uuid[16];
	uint16_t len;
	int r;

	len = sizeof(uuid);
	r = bleGattcRead (ble_gatts_ides_handle, uuid, &len, TRUE);

	if (r == NRF_SUCCESS) {
		if (memcmp (uuid, ble_ides_base_uuid.uuid128, 16) == 0)	 
		printf ("Badge service found\n");
	} else
		printf ("Discovery failed\n");
	return;
}

static void
radio_read (BaseSequentialStream *chp, int argc, char *argv[])
{
	uint8_t buf[16];
	uint16_t handle;
	uint16_t len;
	int r;

	handle = atoi (argv[1]);
	len = sizeof(buf);

	r = bleGattcRead (handle, buf, &len, TRUE);

	if (r == NRF_SUCCESS) {
		printf ("Read %d bytes\n", len);
	} else
		printf ("Read failed\n");
	return;
}

static void
radio_write (BaseSequentialStream *chp, int argc, char *argv[])
{
	uint8_t buf[16];
	uint16_t len;
	uint16_t handle;
	int r;

	handle = atoi (argv[1]);
	strcpy ((char *)buf, argv[2]);
	len = strlen (argv[2]);

	r = bleGattcRead (handle, buf, &len, TRUE);

	if (r == NRF_SUCCESS) {
		printf ("Write succeeded\n");
	} else
		printf ("Write failed\n");
	return;
}

static void
radio_disconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleGapDisconnect ();
	return;
}

static void
radio_connect (BaseSequentialStream *chp, int argc, char *argv[])
{
	ble_gap_addr_t peer;
	unsigned int addr[6];

	memset (&peer, 0, sizeof(peer));

	if (argc != 2) {
		printf ("No peer specified\n");
		return;
	}

	sscanf (argv[1], "%x:%x:%x:%x:%x:%x",
	    &addr[5], &addr[4], &addr[3], &addr[2], &addr[1], &addr[0]);

	peer.addr_id_peer = TRUE;
	peer.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
	peer.addr[0] = addr[0];
	peer.addr[1] = addr[1];
	peer.addr[2] = addr[2];
	peer.addr[3] = addr[3];
	peer.addr[4] = addr[4];
	peer.addr[5] = addr[5];

	bleGapConnect (&peer);

	return;
}

static void
radio_connectcancel (BaseSequentialStream *chp, int argc, char *argv[])
{
	sd_ble_gap_connect_cancel ();
	return;
}

static void
radio_l2capconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleL2CapConnect (BLE_IDES_PSM);
	return;
}

static void
radio_l2capdisconnect (BaseSequentialStream *chp, int argc, char *argv[])
{
	bleL2CapDisconnect (ble_local_cid);
	return;
}

static void
radio_send (BaseSequentialStream *chp, int argc, char *argv[])
{
	if (argc != 2) {
		printf ("No message specified\n");
		return;
	}

	bleL2CapSend (argv[1]);
	return;
}

void
cmd_radio (BaseSequentialStream *chp, int argc, char *argv[])
{

	if (argc == 0) {
		printf ("Radio commands:\n");
		printf ("connect [addr]       Connect to peer\n");
		printf ("cancel               Connect cancel\n");
		printf ("disconnect           Disconnect from peer\n");
		printf ("l2capconnect         Create L2CAP channel\n");
		printf ("send [msg]           Transmit to peer\n");
		printf ("enable               Enable BLE radio\n");
		printf ("disable              Disable BLE radio\n");
		printf ("peerlist             Display nearby peers\n");
		printf ("discover             Discover GATTC services\n");
		printf ("read                 Perform GATTC read\n");
		printf ("write                Perform GATTC write\n");
		return;
	}

	if (strcmp (argv[0], "connect") == 0)
		radio_connect (chp, argc, argv);
	else if (strcmp (argv[0], "cancel") == 0)
		radio_connectcancel (chp, argc, argv);
	else if (strcmp (argv[0], "disconnect") == 0)
		radio_disconnect (chp, argc, argv);
	else if (strcmp (argv[0], "l2capconnect") == 0)
		radio_l2capconnect (chp, argc, argv);
	else if (strcmp (argv[0], "l2capdisconnect") == 0)
		radio_l2capdisconnect (chp, argc, argv);
	else if (strcmp (argv[0], "send") == 0)
		radio_send (chp, argc, argv);
	else if (strcmp (argv[0], "disable") == 0)
		bleDisable ();
	else if (strcmp (argv[0], "enable") == 0)
		bleEnable ();
	else if (strcmp (argv[0], "peerlist") == 0)
		blePeerShow ();
	else if (strcmp (argv[0], "discover") == 0)
		radio_discover (chp, argc, argv);
	else if (strcmp (argv[0], "read") == 0)
		radio_read (chp, argc, argv);
	else if (strcmp (argv[0], "write") == 0)
		radio_write (chp, argc, argv);
	else
		printf ("Unrecognized radio command\n");

	return;
}

orchard_command ("radio", cmd_radio);
