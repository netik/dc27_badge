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

#if (NRF5_SPI_USE_SPI0 == TRUE)
#define SPI_BUS SPID1
#endif

#if (NRF5_SPI_USE_SPI3 == TRUE)
#define SPI_BUS SPID4
#endif

static void init_board(GDisplay *g) {
	(void) g;

	/* Set all pins to initial (high) state */

	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);

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

	spiAcquireBus (&SPI_BUS);
	palClearPad (IOPORT1, IOPORT1_SCREEN_CS);

	return;
}

static void release_bus(GDisplay *g) {
	(void) g;

	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);
	spiReleaseBus (&SPI_BUS);

	return;
}

static void write_index(GDisplay *g, uint16_t index) {
#if NRF5_SPI_USE_SPI3 == TRUE
	uint8_t b;

	b = index & 0xFF;

	palClearPad (IOPORT1, IOPORT1_SCREEN_CD);

	SPI_BUS.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENCLR;
	spi_lld_send (&SPI_BUS, 1, &b);
	while (SPI_BUS.port->TXD.MAXCNT != SPI_BUS.port->TXD.AMOUNT)
		;
	SPI_BUS.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENSET;

	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);

#endif
	(void) g;

	return;
}

static void write_data(GDisplay *g, uint16_t data) {
#if NRF5_SPI_USE_SPI3 == TRUE
	uint8_t b;

	b = data & 0xFF;

	SPI_BUS.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENCLR;
	spi_lld_send (&SPI_BUS, 1, &b);
	while (SPI_BUS.port->TXD.MAXCNT != SPI_BUS.port->TXD.AMOUNT)
		;
	SPI_BUS.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENSET;
#endif

	(void) g;

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
