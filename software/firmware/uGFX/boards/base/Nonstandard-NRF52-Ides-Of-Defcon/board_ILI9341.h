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

static void init_board(GDisplay *g) {
	(void) g;

	/* Set all pins to initial (high) state */
#ifdef IOPORT2
	palSetPad (IOPORT2, IOPORT2_SCREEN_CD);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);
#endif
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
	spiAcquireBus (&SPID4);
#ifdef IOPORT2
	palClearPad (IOPORT1, IOPORT1_SCREEN_CS);
#endif
	return;
}

static void release_bus(GDisplay *g) {
	(void) g;
#ifdef IOPORT2
	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);
#endif
	spiReleaseBus (&SPID4);
	return;
}

static void write_index(GDisplay *g, uint16_t index) {
	uint8_t b;

	(void) g;

	b = index & 0xFF;

#ifdef IOPORT2
	palClearPad (IOPORT2, IOPORT2_SCREEN_CD);
#endif

	SPID4.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPID4.port->INTENCLR;
	spi_lld_send (&SPID4, 1, &b);
	while (SPID4.port->TXD.MAXCNT != SPID4.port->TXD.AMOUNT)
		;
	SPID4.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPID4.port->INTENSET;

#ifdef IOPORT2
	palSetPad (IOPORT2, IOPORT2_SCREEN_CD);
#endif

	return;
}

static void write_data(GDisplay *g, uint16_t data) {
	uint8_t b;

	(void) g;

	b = data & 0xFF;

	SPID4.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPID4.port->INTENCLR;
	spi_lld_send (&SPID4, 1, &b);
	while (SPID4.port->TXD.MAXCNT != SPID4.port->TXD.AMOUNT)
		;
	SPID4.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPID4.port->INTENSET;

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
