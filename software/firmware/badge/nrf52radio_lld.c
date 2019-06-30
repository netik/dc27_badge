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

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "nrf52radio_lld.h"
#include "nrf_error.h"

#include <stdio.h>
#include <string.h>

static thread_reference_t radioThreadReference;

const BLE_CHAN ble_chan_map[BLE_CHANNELS] =  {
	{ 2404, 	 0 },
	{ 2406, 	 1 },
	{ 2408, 	 2 },
	{ 2410, 	 3 },
	{ 2412, 	 4 },
	{ 2414, 	 5 },
	{ 2416, 	 6 },
	{ 2418, 	 7 },
	{ 2420, 	 8 },
	{ 2422, 	 9 },
	{ 2424, 	10 },
	{ 2428, 	11 },
	{ 2430, 	12 },
	{ 2432, 	13 },
	{ 2434, 	14 },
	{ 2436, 	15 },
	{ 2438, 	16 },
	{ 2440, 	17 },
	{ 2442, 	18 },
	{ 2444, 	19 },
	{ 2446, 	20 },
	{ 2448, 	21 },
	{ 2450, 	22 },
	{ 2452, 	23 },
	{ 2454, 	24 },
	{ 2456, 	25 },
	{ 2458, 	26 },
	{ 2460, 	27 },
	{ 2462, 	28 },
	{ 2464, 	29 },
	{ 2466, 	30 },
	{ 2468, 	31 },
	{ 2470, 	32 },
	{ 2472, 	33 },
	{ 2474, 	34 },
	{ 2476, 	35 },
	{ 2478, 	36 },
	{ 2402, 	37 },
	{ 2426, 	38 },
	{ 2480, 	39 }
};

OSAL_IRQ_HANDLER(Vector44)
{
	OSAL_IRQ_PROLOGUE();
	osalSysLockFromISR ();
	osalThreadResumeI (&radioThreadReference, MSG_OK);
	NRF_RADIO->INTENCLR = 0xFFFFFFFF;
	osalSysUnlockFromISR ();
	OSAL_IRQ_EPILOGUE();
	return;
}

static void
radioReset (void)
{
	NRF_RADIO->SHORTS          = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_DISABLE   = 1;

	/* Wait for disable event */

	while (NRF_RADIO->EVENTS_DISABLED == 0) {
	}

	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_RXEN = 0;
	NRF_RADIO->TASKS_TXEN = 0;

	NRF_RADIO->POWER = 0;
	NRF_RADIO->POWER = 1;
	(void)NRF_RADIO->POWER;

	return;
}

static void
radioInit (void)
{
	/* Make sure DC-DC converter is enabled */

	NRF_POWER->DCDCEN = 1;

	/* Make sure HF clock is enabled */

	hal_lld_init ();

	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm;
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;

	NRF_RADIO->TXADDRESS  = (0x00 << RADIO_TXADDRESS_TXADDRESS_Pos) &
	    RADIO_TXADDRESS_TXADDRESS_Msk;

	/* For BLE 1Mbps */

	NRF_RADIO->PCNF0 =
	    (1 << RADIO_PCNF0_S0LEN_Pos) |
	    (0 << RADIO_PCNF0_S1LEN_Pos) |
	    (8 << RADIO_PCNF0_LFLEN_Pos) |
	    (RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos) |
	    (RADIO_PCNF0_S1INCL_Include << RADIO_PCNF0_S1INCL_Pos);

	NRF_RADIO->PCNF1 =
	    (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |
	    (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) |
	    (3 << RADIO_PCNF1_BALEN_Pos) |
	    (0 << RADIO_PCNF1_STATLEN_Pos) |
	    (37 << RADIO_PCNF1_MAXLEN_Pos);

	NRF_RADIO->MODECNF0 =
	    (RADIO_MODECNF0_RU_Default << RADIO_MODECNF0_RU_Pos) |
	    (RADIO_MODECNF0_DTX_Center << RADIO_MODECNF0_DTX_Pos);

	NRF_RADIO->CRCPOLY = 0x0000065B;
	NRF_RADIO->CRCINIT = 0x00555555;
	NRF_RADIO->CRCCNF =
	    (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos) |
	    (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos);

	NRF_RADIO->FREQUENCY = 0;
	NRF_RADIO->DATAWHITEIV = 0x40;

	/* Force disable */

	NRF_RADIO->TASKS_DISABLE = 1;
	while (NRF_RADIO->STATE) {
	}

	NRF_RADIO->EVENTS_DISABLED = 0;

	return;
}

/*
 * Sets the radio frequency.
 * The NRF52 radio can be programmed anywhere between 2400MHz and 2499MHz
 * in 1MHz steps. Some of these frequencies are defined by the BLE spec to
 * correspond to a set of 40 channel numbers from 0 to 39. Channels 37,
 * 38 and 39 are reserved for advertising.
 *
 * The BLE spec requires data whitening to be used for BLE communication,
 * and it also requires that the data whitening input vector be set to
 * the channel number that corresponds to the current frequency. We keep
 * a lookup table of channels and search to see if the requested frequency
 * matchesa known channel so that we can select the correct data whitening
 * initial value.
 */

int
nrf52radioChanSet (uint16_t f)
{
	int i;

	if (f > 2499 || f < 2360)
		return (NRF_ERROR_INVALID_PARAM);

	for (i = 0; i < BLE_CHANNELS; i++) {
		if (ble_chan_map[i].ble_freq == f)
			break;
	}

	if (i == BLE_CHANNELS)
		NRF_RADIO->DATAWHITEIV = 0x40;
	else
		NRF_RADIO->DATAWHITEIV = ble_chan_map[i].ble_chan | 0x40;

	if (f < 2400)
		NRF_RADIO->FREQUENCY = (f - 2400) ||
		    (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos);
	else
		NRF_RADIO->FREQUENCY = (f - 2360) |
		    (RADIO_FREQUENCY_MAP_Low << RADIO_FREQUENCY_MAP_Pos);

	return (NRF_SUCCESS);
}

void
nrf52radioStart (void)
{
	radioReset ();
	radioInit ();
	nvicEnableVector (RADIO_IRQn, NRF5_RADIO_IRQ_PRIORITY);
}

void
nrf52radioStop (void)
{
	nvicDisableVector (RADIO_IRQn);
	radioReset ();

	return;
}

void
nrf52radioRxEnable (void)
{
	/* Enable ready interrupt */

	NRF_RADIO->INTENSET = RADIO_INTENSET_READY_Enabled <<
	    RADIO_INTENSET_READY_Pos;
	NRF_RADIO->EVENTS_READY = 0;

	osalSysLock ();
	NRF_RADIO->TASKS_RXEN = 1;
	(void) osalThreadSuspendS (&radioThreadReference);
	NRF_RADIO->EVENTS_READY = 0;
	osalSysUnlock ();

	return;
}

void
nrf52radioRxDisable (void)
{
	/* Enable disable interrupt */

	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Enabled <<
	    RADIO_INTENSET_DISABLED_Pos;
	NRF_RADIO->EVENTS_DISABLED = 0;

	osalSysLock ();
	NRF_RADIO->TASKS_DISABLE = 1;
	(void) osalThreadSuspendS (&radioThreadReference);
	NRF_RADIO->EVENTS_DISABLED = 0;
	osalSysUnlock ();

	return;
}

int
nrf52radioRssiGet (int8_t * rssi)
{
	if (rssi == NULL)
		return (NRF_ERROR_INVALID_PARAM);

	/* Enable RSSI done interrupt */

	NRF_RADIO->INTENSET = RADIO_INTENSET_RSSIEND_Enabled <<
	    RADIO_INTENSET_RSSIEND_Pos;
	NRF_RADIO->EVENTS_RSSIEND = 0;

	osalSysLock ();
	NRF_RADIO->TASKS_RSSISTART = 1;
	(void) osalThreadSuspendS (&radioThreadReference);
	NRF_RADIO->TASKS_RSSISTOP = 0;
	NRF_RADIO->EVENTS_RSSIEND = 0;
	/*NRF_RADIO->TASKS_RXEN = 1;*/
	osalSysUnlock ();

	*rssi = NRF_RADIO->RSSISAMPLE;

	return (NRF_SUCCESS);
}

int
nrf52radioRx (uint8_t * pkt, uint8_t len, int8_t * rssi, uint32_t timeout)
{
	msg_t r;

	*rssi = 0xFF;

	NRF_RADIO->PREFIX0 &= ~RADIO_PREFIX0_AP0_Msk;
	NRF_RADIO->PREFIX0 |=
	    (BLE_ADV_ACCESS_ADDRESS >> 24) & RADIO_PREFIX0_AP0_Msk;
	NRF_RADIO->BASE0 = (BLE_ADV_ACCESS_ADDRESS << 8);

	NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Enabled <<
	    RADIO_RXADDRESSES_ADDR0_Pos;

	NRF_RADIO->PCNF1 = NRF_RADIO->PCNF1 & ~(RADIO_PCNF1_MAXLEN_Msk);
	NRF_RADIO->PCNF1 |= len << RADIO_PCNF1_MAXLEN_Pos;

	NRF_RADIO->PACKETPTR = (uint32_t)pkt;

	/*
	 * Set up shortcuts
	 * The Nordic radio goes through several states, and can
	 * generate an interrupt when it goes from one state to
	 * another. We don't want to have to check for interrupt
	 * events and walk the chip through each stage of transition
	 * though.
	 *
	 * That's where the shortcuts come in. You can use them
	 * to tell the chip to automatically trigger tasks when
	 * certain events occur, rather than have software do it
	 * manually, which is slow.
	 *
	 * In our case, we want to start the receiver, wait for
	 * an address match on a packet (an advertisement), then
	 * trigger an RSSI reading, so that when the packet is
	 * received, we can tell what the received signal strength
	 * was.
	 */

	NRF_RADIO->SHORTS =
	    ((1 << RADIO_SHORTS_READY_START_Pos) |
	    (1 << RADIO_SHORTS_ADDRESS_RSSISTART_Pos) |
	    (1 << RADIO_SHORTS_END_DISABLE_Pos) |
	    (1 << RADIO_SHORTS_DISABLED_RSSISTOP_Pos));

	NRF_RADIO->INTENSET = RADIO_INTENSET_CRCOK_Enabled <<
	    RADIO_INTENSET_CRCOK_Pos;
	NRF_RADIO->EVENTS_CRCOK = 0;

	osalSysLock ();
	NRF_RADIO->TASKS_RXEN = 1;
	r = osalThreadSuspendTimeoutS (&radioThreadReference,
	    OSAL_MS2I(timeout));
	if (r == MSG_TIMEOUT) {
		NRF_RADIO->SHORTS = 0;
		NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Enabled <<
		    RADIO_INTENSET_DISABLED_Pos;
		NRF_RADIO->EVENTS_DISABLED = 0;
		NRF_RADIO->TASKS_DISABLE = 1;
		(void) osalThreadSuspendS (&radioThreadReference);
		NRF_RADIO->EVENTS_DISABLED = 0;
	} else {
		NRF_RADIO->EVENTS_CRCOK = 0;
	}
	osalSysUnlock ();

	if (r == MSG_TIMEOUT)
		return (NRF_ERROR_TIMEOUT);

	if (NRF_RADIO->CRCSTATUS == 1) {
		*rssi = NRF_RADIO->RSSISAMPLE;
		return (NRF_SUCCESS);
	}

	return (NRF_ERROR_INVALID_DATA);
}
