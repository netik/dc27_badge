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

#ifndef _JOYPAD_LLD_H_
#define _JOYPAD_LLD_H_

#define BUTTON_ENTER_PORT	IOPORT1
#define BUTTON_UP_PORT		IOPORT1
#define BUTTON_DOWN_PORT	IOPORT1
#define BUTTON_LEFT_PORT	IOPORT1
#define BUTTON_RIGHT_PORT	IOPORT1

#define BUTTON_UP_PIN		BTN1
#define BUTTON_DOWN_PIN		BTN2
#define BUTTON_LEFT_PIN		BTN3
#define BUTTON_RIGHT_PIN	BTN4
#define BUTTON_ENTER_PIN	BTN5

/* Joypad event codes */

typedef enum _OrchardAppEventKeyFlag {
	keyPress = 0,
	keyRelease = 1,
} OrchardAppEventKeyFlag;

typedef enum _OrchardAppEventKeyCode {
	keyUp = 0x80,
	keyDown = 0x81,
	keyLeft = 0x82,
	keyRight = 0x83,
	keySelect = 0x84,
} OrchardAppEventKeyCode;

/* Joypad events */

typedef struct _joyInfo {
	ioportid_t port;
	uint8_t pin;
	uint8_t bit;
	OrchardAppEventKeyCode code;
} joyInfo;


#define JOY_ENTER	0x01
#define JOY_UP		0x02
#define JOY_DOWN	0x04
#define JOY_LEFT	0x08
#define JOY_RIGHT	0x10

#define JOY_ENTER_SHIFT	0
#define JOY_UP_SHIFT	1
#define JOY_DOWN_SHIFT	2
#define JOY_LEFT_SHIFT	3
#define JOY_RIGHT_SHIFT	4

extern void joyStart (void);
#endif /* _JOYPAD_LLD_H_ */
