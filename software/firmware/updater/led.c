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

/*
 * This module implements some very basic support for the LED controller
 * on the DC27 badge board. We want to use the LEDs as a progress and
 * success/failure indicator during the flash update, since we can't
 * easily use the screen. We assume that the badge firmware already
 * did most of the initialization of the controller chip and just make
 * sure here select the PWM register page so that we can write to the
 * PWM control registers. We use a single scrolling blue LED to indicate
 * flashing progress. For an error we illuminate all LEDs red, and if
 * the firmware update succeeds we set them all green.
 */

#include "ch.h"
#include "hal.h"
#include "led.h"

#include <string.h>
#include <stdio.h>

static void ledShow (void);
static void ledReset (void);
static void ledSet (uint8_t, uint8_t, uint8_t, uint8_t);
static void ledPageSet (uint8_t);
static void ledRegSet (uint8_t, uint8_t);
static uint8_t ledRegGet (uint8_t);

static const uint8_t led_address[ISSI_LED_COUNT][3] =
{
	/* Here LEDs are in Green, Red, Blue order*/
	/* D201-D208 */
	{0x90, 0xA0, 0xB0}, {0x92, 0xA2, 0xB2}, {0x94, 0xA4, 0xB4},
	{0x96, 0xA6, 0xB6}, {0x98, 0xA8, 0xB8}, {0x9A, 0xAA, 0xBA},
	{0x9C, 0xAC, 0xBC}, {0x9E, 0xAE, 0xBE},

	/* D209-D216 */
	{0x60, 0x70, 0x80}, {0x62, 0x72, 0x82}, {0x64, 0x74, 0x84},
	{0x66, 0x76, 0x86}, {0x68, 0x78, 0x88}, {0x6A, 0x7A, 0x8A},
	{0x6C, 0x7C, 0x8C}, {0x6E, 0x7E, 0x8E},

	/* D217-D224 */
	{0x30, 0x40, 0x50}, {0x32, 0x42, 0x52}, {0x34, 0x44, 0x54},
	{0x36, 0x46, 0x56}, {0x38, 0x48, 0x58}, {0x3A, 0x4A, 0x5A},
	{0x3C, 0x4C, 0x5C}, {0x3E, 0x4E, 0x5E},

	/* D225-D232 */
	{0x00, 0x10, 0x20}, {0x02, 0x12, 0x22}, {0x04, 0x14, 0x24},
	{0x06, 0x16, 0x26}, {0x08, 0x18, 0x28}, {0x0A, 0x1A, 0x2A},
	{0x0C, 0x1C, 0x2C}, {0x0E, 0x1E, 0x2E},
};

static uint8_t led_memory[ISSI_REG_MAX + 2];

static uint8_t led_pos;

void
ledInit (void)
{
	int i;

	/* Select function page */

	ledPageSet (ISSI_PAGE_FUNCTION);

	/* Reset the chip */

	(void) ledRegGet (ISSI_REG_RESET);

	/* Select function page */

	ledPageSet (ISSI_PAGE_FUNCTION);

	/* Set configuration - disable soft shutdown */

	ledRegSet (ISSI_REG_CONFIG, ISSI_REG_CONFIG_SSD_OFF);

	/* Set global current control to max */

	ledRegSet (ISSI_REG_GCC, 0xFF);

	/* Turn on LEDs */

	ledPageSet (ISSI_PAGE_LED);

	for (i = 0; i < 0x18; i++)
		ledRegSet (i, 0xFF);

	/* Select PWM register page */

	ledPageSet (ISSI_PAGE_PWM);

	return;
}

static void
ledReset (void)
{
	volatile uint32_t * p = (uint32_t *)(NRF_TWI1_BASE + 0xFFC);

	/* Stop and restart the I2C bus to clear any errors */

	i2c_lld_stop (&I2CD2);
	I2CD2.state = I2C_STOP;

	/*
	 * Power cycle the controller by writing to the magic
	 * undocumented power control register at offset 0xFFC.
	 */

	*p = 0;
 	(void)p;
	*p = 1;

	i2c_lld_start (&I2CD2);
	I2CD2.state = I2C_READY;

	return;
}

static void
ledPageSet (uint8_t page)
{
	/* Unlock command register */

	ledRegSet (ISSI_REG_COMMAND_UNLOCK, ISSI_CMDUNLOCK_ENABLE);

	/* Select register page */

	ledRegSet (ISSI_REG_COMMAND, page);

	return;
}

static uint8_t
ledRegGet (uint8_t reg)
{
	uint8_t rxbuf;
	uint8_t txbuf;

	txbuf = reg;

	(void)i2cMasterTransmitTimeout (&I2CD2, ISSI_I2C_ADDR,
	    &txbuf, 1, &rxbuf, 1, ISSI_TIMEOUT);

	return (rxbuf);
}

static void
ledRegSet (uint8_t reg, uint8_t val)
{
	uint8_t txbuf[2];

	txbuf[0] = reg;
	txbuf[1] = val;

	(void)i2cMasterTransmitTimeout (&I2CD2, ISSI_I2C_ADDR,
	    txbuf, 2, NULL, 0, ISSI_TIMEOUT);

	return;
}

static void
ledShow (void)
{
	msg_t r;

	/*
	 * Assume that we're still on the PWM control page.
	 * Write the whole LED "frame buffer" in one transaction.
	 * Note: the first element in the led_memory[] array is
	 * the offset of the first LED PWM duty cycle register,
	 * which happens to be 0. After that's set, the rest
	 * of the buffer is written to the PWM duty cycle
	 * registers in sequence using the autoincrement feature.
	 */

	r = i2cMasterTransmitTimeout (&I2CD2, ISSI_I2C_ADDR,
	    led_memory, sizeof(led_memory), NULL, 0, ISSI_TIMEOUT);

	if (r != MSG_OK)
		ledReset ();

	return;
}

static void
ledSet (uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
	if (index > ISSI_LED_COUNT)
		return;

	led_memory[led_address[index][0] + 1] = g;
	led_memory[led_address[index][1] + 1] = r;
	led_memory[led_address[index][2] + 1] = b;

	return;
}

void
ledProgress (void)
{
	ledSet (led_pos, 0, 0, LED_INTENSITY);
	ledShow ();
	ledSet (led_pos, 0, 0, 0);
	led_pos++;
	if (led_pos == ISSI_LED_COUNT)
		led_pos = 0;
	return;
}

void
ledError (void)
{
	int i;
	for (i = 0; i < ISSI_LED_COUNT; i++)
		ledSet (i, LED_INTENSITY, 0, 0);
	ledShow ();
	return;
}

void
ledSuccess (void)
{
	int i;
	for (i = 0; i < ISSI_LED_COUNT; i++)
		ledSet (i, 0, LED_INTENSITY, 0);
	ledShow ();
	return;
}
