/*-
 * Copyright (c) 2016-2017
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
 * This module implements support for the Shenzen XPTEK XPT2046 4-wire
 * resistive touch screen controller chip. The XPT2046 provides a
 * 4-wire SPI interface and includes a set of ADC blocks for performing
 * sensing with resistive touch units. It provides X and Y axis readings
 * readings as well as Z axis presure sense reading and a temperature sensor.
 * Commands are sent as a single 8-bit transaction and responses are
 * received as either an 8-bit or 16-bit transaction. Sensor readings are
 * returned as 12-bit quantities.
 *
 * SEE ALSO:
 * https://ldm-systems.ru/f/doc/catalog/HY-TFT-2,8/XPT2046.pdf
 */

#include "ch.h"
#include "hal.h"
#include "osal.h"
#include "hal_spi.h"
#include "hal_pal.h"
#include "badge.h"

#include "xpt2046_reg.h"
#include "xpt2046_lld.h"

/******************************************************************************
*
* xptGet - issue command to XPT2046 chip and return result
*
* This function issues a command to the XPT2046 chip and returns a result.
* The command is an instruction to sample a value from one of the ADC
* channels. Sensor results can be returned as either 8-bit or 12-bit values.
* This function always requests 12-bit results. Valid commands are be:
*
* XPT_CHAN_X		X axis position
* XPT_CHAN_Y		Y axis position
* XPT_CHAN_Z1           Z1 pressure sense
* XPT_CHAN_Z2           Z2 pressure sense
* XPT_CHAN_VBAT		battery voltage
* XPT_CHAN_TEMP0        temperature sensor
* XPT_CHAN_TEMP1        temperature sensor
*
* The chip is kept powered down between readings to conserve power.
*
* RETURNS: 16-bit sensor value for X, Y or Z axis, or temperature reading
*/

uint16_t xptGet (uint8_t cmd)
{
	uint8_t		reg;
	uint8_t		v[2];
	uint16_t	val;
	uint32_t	freq;

	reg = cmd | XPT_CTL_START;

	spiAcquireBus (&SPID1);
	freq = SPID1.port->FREQUENCY;
	SPID1.port->FREQUENCY = NRF5_SPI_FREQ_4MBPS;

	palClearPad (IOPORT1, IOPORT1_TOUCH_CS);

	/* Send command byte */

	spiSend (&SPID1, 1, &reg);

	/* Receive response byte 1 */

	spiReceive (&SPID1, 1, &v[0]);

	/* Receive response byte 2 */

	spiReceive (&SPID1, 1, &v[1]);

	palSetPad (IOPORT1, IOPORT1_TOUCH_CS);
	SPID1.port->FREQUENCY = freq;
	spiReleaseBus (&SPID1);

	val = v[1] >> 3;
	val |= v[0] << 5;

	/* Only the 12 lower bits matter */

	val &= 0xFFF;

	return (val);
}

#ifdef notdef
void xptTest(void)
{
	uint16_t	v;
	uint16_t	z1;
	uint16_t	z2;

	while (1) {
		z1 = xptGet (XPT_CHAN_Z1);
		z2 = xptGet (XPT_CHAN_Z2);
		if ((z2 - z1) < XPT_TOUCH_THRESHOLD) {
			printf ("press(%d:%d): ", z1, z2);
			v = xptGet (XPT_CHAN_X);
			printf ("X: %d ", v);
			v = xptGet (XPT_CHAN_Y);
			printf ("Y: %d\r\n", v);
		}
		chThdSleepMilliseconds (100);
	}

	return;
}
#endif
