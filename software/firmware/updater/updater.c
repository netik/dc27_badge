/*-
 * Copyright (c) 2016-2019
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
 * This module implements a firmware update utility for the Nordic Semi
 * NRF52840 board with SD card connected via SPI channel 0.
 *
 * The firmware update process works in conjunction with a firmware update
 * app in the firmware already present in NRF52's flash. The updater utility
 * itself is a standalone binary image which is stored on the SD card. It's
 * linked to execute in the RAM normally reserved for the SoftDevice. This
 * is so that it doesn't conflict with the RAM used by ChibiOS. To peform an
 * update, the existing firmware loads the UPDATER.BIN image into RAM and
 * then simply jumps to it. At that point, the updater takes over full
 * control of the system.
 *
 * The updater makes use of the FatFs code to access the SD card and the
 * NVMC peripheral to write to the NRF52's flash. The FatFs code is
 * set up to use SPI channel 0 using the mmc_spi_lld.c module. The
 * one complication is that the mmc_spi_lld.c code requires a timer.
 * This is provided using one of the NRF52's internal timers. The
 * trouble is that it that the way the code is written, it depends on
 * the timer triggering periodic interrupts. On the Cortex-M4 processor,
 * interrupts are routed via the vector table which is stored at page 0
 * in flash, which connects the timer interrupt to a handler in the firmware.
 * We need to override this somehow so that we can get the timer interrupts
 * to call an interrupt handler in the updater instead.
 *
 * Fortunately, the NRF52 is a Cortex-M4 CPU, which supports the VTOR
 * register. This allows us to change the vector table offset and set up
 * replacement vector table in RAM. We can then program this table so that
 * the PIT vector is tied to the updater's interrupt service routine.
 *
 * The process to capture the interrupts is a little tricky. We have to
 * mask off all interrupts first and then disable all interrupt event
 * sources. In addition to the NVIC IRQs, we also have to turn off the
 * SysTick timer. Once we do that, no other interrupts should occur,
 * unless we crash the system and trigger a fault.
 *
 * Once the interrupt sources are disabled, we can then remap the
 * vector table and install a new PIT interrupt handler. Then we can
 * initialize the FatFs library and access the SD card.
 *
 * Note that we assume that the SPI channel has already been initalized
 * by the firmware that loaded the updater utility (since that requires
 * the use of the SD card too, this is a safe assumption to make).
 *
 * Once the firmware is updated, the updater will trigger soft reset of
 * the CPU to restart the system.
 *
 * Once it runs, the updater takes over full control of the CPU. All
 * previous RAM contents remain, but are ignored. The code is kept small
 * in order to fit in about 5KB of memory so that there will be space
 * left over for some data buffers and the stack.
 *
 * The memory map when the updater runs is shown below:
 *
 * 0x2000_0100			Start of updater binary
 * 0x2000_4000			stack
 * 0x2000_4400			FatFs structure
 * 0x2000_8000			4096-byte data buffer
 */

#include "ch.h"
#include "hal.h"
#include "updater.h"
#include "mmc.h"
#include "flash.h"
#include "led.h"

#include "ff.h"
#include "ffconf.h"

/*
 * Per the manual, the vector table must be aligned on an 8-bit
 * boundary.
 */

__attribute__((aligned(0x80))) isr vectors[16 + CORTEX_NUM_VECTORS];

isr * origvectors = (isr *)0;

#define UPDATE_SIZE	FLASH_PAGE_SIZE /* 4096 bytes */

extern uint8_t bss;
extern uint8_t end;

static void
mmc_callback(GPTDriver *gptp)
{
	(void)gptp;
	disk_timerproc ();
	return;
}

extern void Vector4C (void); /* SPI */
extern void Vector50 (void); /* I2C */
extern void Vector68 (void); /* TIMER2 */

static const GPTConfig gpt3_config = {
    .frequency  = NRF5_GPT_FREQ_62500HZ,
    .callback   = mmc_callback,
    .resolution = 32,
};

static const I2CConfig i2c2_config = {
	400000,			/* clock */
	IOPORT1_I2C_SCK,	/* scl_pad */
	IOPORT1_I2C_SDA		/* sda_pad */
};

static const SPIConfig spi1_config = {
 	NULL,			/* enc_cp */
	NRF5_SPI_FREQ_8MBPS,	/* freq */
	IOPORT2_SPI_SCK,	/* sckpad */
	IOPORT2_SPI_MOSI,	/* mosipad */
	IOPORT2_SPI_MISO,	/* misopad */
	IOPORT1_SDCARD_CS,	/* sspad */
	FALSE,			/* lsbfirst */
	2,			/* mode */
	0xFF			/* dummy data for spiIgnore() */
};

int
updater (void)
{
	FIL f;
	UINT br;
	int i;
	int total;
	uint8_t * p;
	uint8_t * src = (uint8_t *)UPDATER_DATBUF;
	uint8_t * dst = (uint8_t *)0x0;
	FATFS * fs = (FATFS *)UPDATER_FATFS;

	/* Disable all interrupts */

	__disable_irq();
	SysTick->CTRL = 0;

	/* Clear the BSS. */

	p = (uint8_t *)&bss;
	total = &end - &bss;

	for (i = 0; i < total; i++)
		p[i] = 0;

	/*
	 * Copy the original vector table. We do this just as a debug
	 * aid: if we crash before getting to copy the firmware,
	 * the hardfault handler will catch us and we may be able to
	 * get a stack trace.
	 */

	for (i = 0; i < 16 + CORTEX_NUM_VECTORS; i++)
		vectors[i] = origvectors[i];

	/* Turn off all IRQs. */

	for (i = 0; i < CORTEX_NUM_VECTORS; i++)
		nvicDisableVector (i);

	/* Modify a few vectors to point to our handlers */

	vectors[16 + SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn] = Vector4C;
	vectors[16 + SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn] = Vector50;
	vectors[16 + TIMER2_IRQn] = Vector68;

	/* Now shift the vector table location. */

	SCB->VTOR = (uint32_t)vectors;

	/* Now safe to re-enable interrupts */

	__enable_irq();

	/*
	 * Initialize the GPIO subsystem
	 * Note: boardInit() sets up the second GPIO bank.
	 */

	palInit (&pal_default_config);
	boardInit ();

	/* Initialize I2C */

	i2cInit ();
	i2cStart (&I2CD2, &i2c2_config);

	/* Initialize LEDs. */

	ledInit ();

	/* Enable the SPI driver */

	spiInit ();
	spiStart (&SPID1, &spi1_config);

	/* Set up I/O pins. */

	palSetPad (IOPORT1, IOPORT1_SPI_MOSI);
	palSetPad (IOPORT1, IOPORT1_SPI_MISO);
	palSetPad (IOPORT1, IOPORT1_SPI_SCK);
	palSetPad (IOPORT1, IOPORT1_SDCARD_CS);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);
	palSetPad (IOPORT1, IOPORT1_TOUCH_CS);

	/* Enable a timer (for the fatfs timer) */

	gptInit ();
	gptStart (&GPTD3, &gpt3_config);

	/* Initalize flash library */

	flashStart ();

	/*
	 * Initialize and mount the SD card
	 * If this fails, turn the LEDs red to signal a problem
	 * to the user.
	 */

	if (f_mount (fs, "0:", 1) != FR_OK ||
	    f_open (&f, "BADGE.BIN", FA_READ) != FR_OK) {
		ledError ();
		while (1) {}
	}

	/*
	 * Read data from the SD card and copy it to flash. We
	 * can't easily write to the screen, so instead we use the
	 * LED array to signal progress to the user. We keep one
	 * lit LED cycling through the array as a progress indicator.
	 * Once flashing is done, we light all the LEDs up green.
	 */

	while (1) {
		f_read (&f, src, UPDATE_SIZE, &br);
		if (br == 0)
			break;
		flashErase (dst);
		flashProgram (src, dst);
		dst += br;
		ledProgress ();
	}

#ifdef notdef
	/*
	 * This is a bit of a hack, but it reduces the size of the
	 * updater.bin image by over 450 bytes. Strictly speaking,
	 * we don't need to call the close function here after
	 * we're done flashing the new firmware. We've already
	 * read all the data from the file on the SD card, and we
	 * don't care about the internal state of the FatFs code
	 * since we're about to hard reset the system anyway.
	 */
	f_close (&f);
#endif

	ledSuccess ();

	/* Reboot and load the new firmware */

	NVIC_SystemReset ();

	/* NOTREACHED */
	return (0);
}
