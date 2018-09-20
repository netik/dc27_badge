/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

/*
 * For the DC27 board, we use the display in SPI mode on bus SPIM3
 * at 32MHz. This should give us the best possible screen performance.
 */

#include "ch.h"
#include "hal.h"
#include "hal_spi.h"

__attribute__ ((noinline))
static void init_board(GDisplay *g) {
	(void) g;

	/* Set all pins to initial (high) state */

	palSetPad (IOPORT2, IOPORT2_SCREEN_CD);
	palSetPad (IOPORT2, IOPORT2_SCREEN_CS);

	return;
}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;
}

static GFXINLINE void set_backlight(GDisplay *g, uint8_t percent) {
	(void) g;
	(void) percent;
}

static void acquire_bus(GDisplay *g) {
	(void) g;
	spiAcquireBus (&SPID1);
	palClearPad (IOPORT2, IOPORT2_SCREEN_CS);
	return;
}

static void release_bus(GDisplay *g) {
	(void) g;
	palSetPad (IOPORT2, IOPORT2_SCREEN_CS);
	spiReleaseBus (&SPID1);
	return;
}

__attribute__ ((noinline))
static void write_index(GDisplay *g, uint16_t index) {
	uint8_t b;

	(void) g;

	b = index & 0xFF;

	palClearPad (IOPORT2, IOPORT2_SCREEN_CD);

	spiSend (&SPID1, 1, &b);

	palSetPad (IOPORT2, IOPORT2_SCREEN_CD);
	return;
}

__attribute__ ((noinline))
static void write_data(GDisplay *g, uint16_t data) {
	uint8_t b;

	(void) g;

	b = data & 0xFF;
	spiSend (&SPID1, 1, &b);

	return;
}

static GFXINLINE void setreadmode(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setwritemode(GDisplay *g) {
	(void) g;
}

static GFXINLINE uint16_t read_data(GDisplay *g) {
	(void) g;
	return 0;
}

#endif /* _GDISP_LLD_BOARD_H */
