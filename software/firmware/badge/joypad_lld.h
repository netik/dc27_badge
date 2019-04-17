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

#ifndef _JOYPAD_LLD_H_
#define _JOYPAD_LLD_H_

#define BUTTON_A_UP_PORT	IOPORT1
#define BUTTON_A_DOWN_PORT	IOPORT1
#define BUTTON_A_LEFT_PORT	IOPORT1
#define BUTTON_A_RIGHT_PORT	IOPORT1
#define BUTTON_A_ENTER_PORT	IOPORT2

#define BUTTON_B_UP_PORT	IOPORT2
#define BUTTON_B_DOWN_PORT	IOPORT2
#define BUTTON_B_LEFT_PORT	IOPORT2
#define BUTTON_B_RIGHT_PORT	IOPORT2
#define BUTTON_B_ENTER_PORT	IOPORT2

#define BUTTON_A_UP_PIN		IOPORT1_BTN1
#define BUTTON_A_DOWN_PIN	IOPORT1_BTN2
#define BUTTON_A_LEFT_PIN	IOPORT1_BTN3
#define BUTTON_A_RIGHT_PIN	IOPORT1_BTN4
#define BUTTON_A_ENTER_PIN	(IOPORT2_BTN5 & 0x1F)

#define BUTTON_B_UP_PIN		(IOPORT2_BTN6 & 0x1F)
#define BUTTON_B_DOWN_PIN	(IOPORT2_BTN7 & 0x1F)
#define BUTTON_B_LEFT_PIN	(IOPORT2_BTN8 & 0x1F)
#define BUTTON_B_RIGHT_PIN	(IOPORT2_BTN9 & 0x1F)
#define BUTTON_B_ENTER_PIN	(IOPORT2_BTN10 & 0x1F)

/* Joypad event codes */

typedef enum _OrchardAppEventKeyFlag {
	keyPress = 0,
	keyRelease = 1,
} OrchardAppEventKeyFlag;

typedef enum _OrchardAppEventKeyCode {
	keyAUp = 0x80,
	keyADown = 0x81,
	keyALeft = 0x82,
	keyARight = 0x83,
	keyASelect = 0x84,
	keyBUp = 0x85,
	keyBDown = 0x86,
	keyBLeft = 0x87,
	keyBRight = 0x88,
	keyBSelect = 0x89,
} OrchardAppEventKeyCode;

/* Joypad events */

typedef struct _joyInfo {
	ioportid_t port;
	uint8_t pin;
	uint16_t bit;
	OrchardAppEventKeyCode code;
} joyInfo;


#define JOY_A_UP	0x0001
#define JOY_A_DOWN	0x0002
#define JOY_A_LEFT	0x0004
#define JOY_A_RIGHT	0x0008
#define JOY_A_ENTER	0x0010
#define JOY_B_UP	0x0020
#define JOY_B_DOWN	0x0040
#define JOY_B_LEFT	0x0080
#define JOY_B_RIGHT	0x0100
#define JOY_B_ENTER	0x0200

#define JOY_A_UP_SHIFT		0
#define JOY_A_DOWN_SHIFT	1
#define JOY_A_LEFT_SHIFT	2
#define JOY_A_RIGHT_SHIFT	3
#define JOY_A_ENTER_SHIFT	4
#define JOY_B_UP_SHIFT		5
#define JOY_B_DOWN_SHIFT	6
#define JOY_B_LEFT_SHIFT	7
#define JOY_B_RIGHT_SHIFT	8
#define JOY_B_ENTER_SHIFT	9

extern void joyStart (void);
#endif /* _JOYPAD_LLD_H_ */
