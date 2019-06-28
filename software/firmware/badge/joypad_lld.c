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
 * This module implements joypad support for the DC27 Ides of DEF CON board.
 *
 * The production board has two 4-position joypads with press-to-select
 * function. That yields 5 GPIO inputs per pad, for a total of 10 pins.
 * The nRF52840 chip has more than enough GPIOs for this, however the GPIOTE
 * module only has 8 channels, meaning we can only trigger interrupt events
 * for at most 8 pins at any given time.
 *
 * To deal with this, all joypad inputs are also connected via switching
 * diodes to an 11th pin, which is used as a shared interrupt input. This
 * allows us to use just one GPIOTE channel for input sensing. When a pin
 * is driven low, it will also drive the interrupt pin low, which will
 * trigger an interrupt. The joypad thread will sample the state of all
 * joypad pins and signal individual changes.
 *
 * This means we have to keep polling all pins for as long as any one
 * button is pushed down. This is a little inefficient, but once all
 * buttons are released, the joypad thread will go idle again. Note that
 * we are also effectively emulating a shared level-triggered active-low
 * interrupt in software, since the GPIOTE module seems to only want to
 * trigger interrupts on low-to-high or high-to-low edge transitions. 
 */
 
#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "joypad_lld.h"

#include "orchard-app.h"
#include "orchard-events.h"

#include "badge.h"

extern event_source_t orchard_app_key;

static void joyInterrupt (EXTDriver *, expchannel_t);

static thread_reference_t joyThreadReference;
static THD_WORKING_AREA(waJoyThread, 512);

/* Default state is all buttons pressed. */

static uint16_t joyState = 0xFFFF;
static uint8_t joyService = 0x0;
static volatile uint8_t plevel;

OrchardAppEvent joyEvent;

static const joyInfo joyTbl[10] = {
	/* Buttons for joypad A */

	{ BUTTON_A_UP_PORT, BUTTON_A_UP_PIN, JOY_A_UP, keyAUp },
	{ BUTTON_A_DOWN_PORT, BUTTON_A_DOWN_PIN, JOY_A_DOWN, keyADown },
	{ BUTTON_A_LEFT_PORT, BUTTON_A_LEFT_PIN, JOY_A_LEFT, keyALeft },
	{ BUTTON_A_RIGHT_PORT, BUTTON_A_RIGHT_PIN, JOY_A_RIGHT, keyARight },
	{ BUTTON_A_ENTER_PORT, BUTTON_A_ENTER_PIN, JOY_A_ENTER, keyASelect },

	/* Buttons for joypad B */
 
	{ BUTTON_B_UP_PORT, BUTTON_B_UP_PIN, JOY_B_UP, keyBUp },
	{ BUTTON_B_DOWN_PORT, BUTTON_B_DOWN_PIN, JOY_B_DOWN, keyBDown },
	{ BUTTON_B_LEFT_PORT, BUTTON_B_LEFT_PIN, JOY_B_LEFT, keyBLeft },
	{ BUTTON_B_RIGHT_PORT, BUTTON_B_RIGHT_PIN, JOY_B_RIGHT, keyBRight },
	{ BUTTON_B_ENTER_PORT, BUTTON_B_ENTER_PIN, JOY_B_ENTER, keyBSelect }
};

static const EXTConfig ext_config = {
	{
		{ EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART |
		  IOPORT2_JOYINTR << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART |
		  IOPORT1_SAO_GPIO2 << EXT_MODE_GPIO_OFFSET, joyInterrupt },
		{ 0 , NULL },
		{ 0 , NULL },
		{ 0 , NULL },
		{ 0 , NULL },
		{ 0 , NULL },
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
	plevel = palReadPad (IOPORT2, (IOPORT2_JOYINTR & 0x1F));
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
	uint16_t pad;

	pad = palReadPad (joyTbl[s].port, joyTbl[s].pin);

	pad <<= s;

	if (pad ^ (joyState & joyTbl[s].bit)) {
		joyState &= ~joyTbl[s].bit;
		joyState |= pad;

#ifdef JOYPAD_VERBOSE
		if (pad)
			printf ("button %x released\r\n", joyTbl[s].code);
		else
			printf ("button %x pressed\r\n", joyTbl[s].code);
#endif

		joyEvent.key.code = joyTbl[s].code;
		if (pad)
			joyEvent.key.flags = keyRelease;
		else
			joyEvent.key.flags = keyPress;

		joyEvent.type = keyEvent;

		if (orchard_app_key.next != NULL)
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
	int i;
	uint8_t service;

	(void)arg;
    
	chRegSetThreadName ("JoyEvent");
    
	while (1) {
		osalSysLock ();

		/* Keep checking for events until we're completely idle. */

		if (joyService == 0 && plevel)
			osalThreadSuspendS (&joyThreadReference);
		service = joyService;
		joyService = 0;
		osalSysUnlock ();

		/* Delay a little bit for debouncing. */

		chThdSleep (1);

		if (service & 0x2) {
			joyEvent.key.code = keyPuz;
			joyEvent.key.flags = keyPress;
			joyEvent.type = keyEvent;
			if (orchard_app_key.next != NULL)
				chEvtBroadcast (&orchard_app_key);
		}

		/* Poll all the inputs */

		for (i = 0; i < 10; i++)
			joyHandle (i);
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

	plevel = 1;

	chThdCreateStatic (waJoyThread, sizeof(waJoyThread),
	    NORMALPRIO + 1, joyThread, NULL);

	/* Set all pins to high state */

	palSetPad (BUTTON_A_UP_PORT, BUTTON_A_UP_PIN);
	palSetPad (BUTTON_A_DOWN_PORT, BUTTON_A_DOWN_PIN);
	palSetPad (BUTTON_A_LEFT_PORT, BUTTON_A_LEFT_PIN);
	palSetPad (BUTTON_A_RIGHT_PORT, BUTTON_A_RIGHT_PIN);
	palSetPad (BUTTON_A_ENTER_PORT, BUTTON_A_ENTER_PIN);

	palSetPad (BUTTON_B_UP_PORT, BUTTON_B_UP_PIN);
	palSetPad (BUTTON_B_DOWN_PORT, BUTTON_B_DOWN_PIN);
	palSetPad (BUTTON_B_LEFT_PORT, BUTTON_B_LEFT_PIN);
	palSetPad (BUTTON_B_RIGHT_PORT, BUTTON_B_RIGHT_PIN);
	palSetPad (BUTTON_B_ENTER_PORT, BUTTON_B_ENTER_PIN);

	palSetPad (PUZ_PORT, PUZ_PIN);

	palSetPad (IOPORT2, (IOPORT2_JOYINTR & 0x1F));

	/* Start the ChibiOS external input module */

	extStart (&EXTD1, &ext_config);

	return;
}
