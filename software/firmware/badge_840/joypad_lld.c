/*- 
 * Copyright (c) 2017-2018
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

/*
 * This module implements joypad support for the NRF52840 board. The
 * buttons are implemented using GPIOs, which are set for input mode
 * with internal pullups enabled. The GPIOTE module is used for sensing
 * state change events. The pins are configured to trigger interrupts
 * for both rising and falling edges so that we can sense both
 * press and release.
 *
 * Note that the NRF52-DK board only has 4 buttons. (The 5th is reset.)
 */
 
#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "joypad_lld.h"
#include "i2s_lld.h"

#include "orchard-app.h"
#include "orchard-events.h"

#include "badge.h"

extern event_source_t orchard_app_key;

static void joyInterrupt (EXTDriver *, expchannel_t);

static thread_reference_t joyThreadReference;
static THD_WORKING_AREA(waJoyThread, 128);

/* Default state is all buttons pressed. */

static uint8_t joyState = 0xFF;
static uint8_t joyService = 0x0;

OrchardAppEvent joyEvent;

static const joyInfo joyTbl[7] = {
	{ BUTTON_ENTER_PORT, BUTTON_ENTER_PIN, JOY_ENTER, keySelect },
	{ BUTTON_UP_PORT, BUTTON_UP_PIN, JOY_UP, keyUp },
	{ BUTTON_DOWN_PORT, BUTTON_DOWN_PIN, JOY_DOWN, keyDown },
	{ BUTTON_LEFT_PORT, BUTTON_LEFT_PIN, JOY_LEFT, keyLeft },
	{ BUTTON_RIGHT_PORT, BUTTON_RIGHT_PIN, JOY_RIGHT, keyRight },
	{ BUTTON_A_PORT, BUTTON_A_PIN, JOY_A, keyA },
	{ BUTTON_B_PORT, BUTTON_B_PIN, JOY_B, keyB },
};

static const EXTConfig ext_config = {
	{
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  (0x20|IOPORT2_BTN5) << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  IOPORT1_BTN1 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  IOPORT1_BTN2 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  IOPORT1_BTN3 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  IOPORT1_BTN4 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  (0x20|IOPORT2_BTN6) << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  (0x20|IOPORT2_BTN7) << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ 0 , NULL }
	}
};

/******************************************************************************
*
* joyInterrupt - interrupt service routine for button events
*
* This function is the callback invoked for all GPIOTE events. It will
* wake the button event processing thread whenever a press or release
* occurs.
*
* RETURNS: N/A
*/

static void
joyInterrupt (EXTDriver *extp, expchannel_t chan)
{
	osalSysLockFromISR ();
	joyService |= 1 << chan;
	osalThreadResumeI (&joyThreadReference, MSG_OK);
	osalSysUnlockFromISR ();
	return;
}

/******************************************************************************
*
* joyHandle - process joypad button events
*
* This function is invoked by the event handler thread to process all
* joypad events. The argument <s> indicates the shift offset of the button
* that triggered the event. The current state of the button is checked
* so that events can be dispatched to listening applications.
*
* RETURNS: N/A
*/

static void
joyHandle (uint8_t s)
{
	uint8_t pad;
	int i;

	for (i = 0; i < 3; i++) {
		chThdSleep (1);
		pad = palReadPad (joyTbl[s].port, joyTbl[s].pin);
	}

	pad <<= s;

	if (pad ^ (joyState & joyTbl[s].bit)) {
		joyState &= ~joyTbl[s].bit;
		joyState |= pad;

		if (pad)
			printf ("button %x released\r\n", joyTbl[s].code);
		else
			printf ("button %x pressed\r\n", joyTbl[s].code);

		joyEvent.key.code = joyTbl[s].code;
		if (pad)
			joyEvent.key.flags = keyRelease;
		else
			joyEvent.key.flags = keyPress;

		joyEvent.type = keyEvent;

		if (joyEvent.key.flags == keyPress) {
			i2sPlay("click.raw");
			i2sWait ();
		}

		chEvtBroadcast (&orchard_app_key);
	}

	return;
}

/******************************************************************************
*
* joyThread - joypad event thread
*
* This is the joypad event handler main thread loop. It sleeps until woken
* up by a button event interrupt, then invokes event handler routine until
* the trggering button event is serviced.
*
* RETURNS: N/A
*/

static THD_FUNCTION(joyThread, arg)
{
	uint8_t service;
	uint8_t busy;
	int i;

	(void)arg;
    
	chRegSetThreadName ("JoyEvent");
    
	while (1) {
		osalSysLock ();
		osalThreadSuspendS (&joyThreadReference);
		osalSysUnlock ();

again:
		busy = 0;
		for (i = 0; i < 7; i++) {
			osalSysLock ();
			service = joyService & (1 << i);
			joyService &= (uint8_t)~(1 << i);
			osalSysUnlock ();

			if (service) {
				joyHandle (i);
				busy++;
			}
		}
		/* Keep checking for events until we're completely idle. */
		if (busy != 0)
			goto again;

	}

	/* NOTREACHED */
}

/******************************************************************************
*
* joyStart - joypad driver start routine
*
* This function initializes the joypad driver. The event handler thread is
* started and the initial state of the pads is set, then the ChibiOS EXT
* driver is started to handle GPIO interrupt events.
*
* RETURNS: N/A
*/

void
joyStart (void)
{
	/* Launch button handler thread. */

	chThdCreateStatic (waJoyThread, sizeof(waJoyThread),
	    NORMALPRIO + 1, joyThread, NULL);

	palSetPad (BUTTON_ENTER_PORT, BUTTON_ENTER_PIN);
	palSetPad (BUTTON_UP_PORT, BUTTON_UP_PIN);
	palSetPad (BUTTON_DOWN_PORT, BUTTON_DOWN_PIN);
	palSetPad (BUTTON_LEFT_PORT, BUTTON_LEFT_PIN);
	palSetPad (BUTTON_RIGHT_PORT, BUTTON_RIGHT_PIN);
	palSetPad (BUTTON_A_PORT, BUTTON_A_PIN);
	palSetPad (BUTTON_B_PORT, BUTTON_B_PIN);

	extStart (&EXTD1, &ext_config);
	return;
}
