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

#if (NRF5_SPI_USE_SPI3 == TRUE)
#define SPI_BUS SPID4
#else
#define SPI_BUS SPID1
#endif

static void init_board(GDisplay *g) {
	(void) g;

	/*
	 * Set initial pin state.
	 * Note: Since the display is the only slave on the SPI bus
	 * in our current design, we could set the chip select pin
	 * to low and just leave it there so that the screen is always
	 * selected. However we would like to be able to read pixels
	 * from the screen as well as write, and setting both the chip
	 * select and command/data select pins the right way is integral
	 * to getting the chip to handle commands correctly. If we leave
	 * the CS pin always set, and issue a read command, then subsequent
	 * pixel write commands will fail.
	 */

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
	__DSB();

	SPI_BUS.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENCLR;
	spi_lld_send (&SPI_BUS, 1, &b);
	while (SPI_BUS.port->TXD.MAXCNT != SPI_BUS.port->TXD.AMOUNT)
		;
	__DSB();
	SPI_BUS.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENSET;

	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);
	__DSB();

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
	__DSB();
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
#if NRF5_SPI_USE_SPI3 == TRUE
	uint16_t b;

	palClearPad (IOPORT1, IOPORT1_SCREEN_CS);
	__DSB();
	SPI_BUS.port->INTENCLR = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENCLR;
	spi_lld_receive (&SPI_BUS, 2, &b);
	while (SPI_BUS.port->RXD.MAXCNT != SPI_BUS.port->RXD.AMOUNT)
		;
	__DSB();
	SPI_BUS.port->INTENSET = SPIM_INTENSET_END_Msk;
	(void)SPI_BUS.port->INTENSET;

	palSetPad (IOPORT1, IOPORT1_SCREEN_CS);
	__DSB();
	return (b);
#else
	return 0;
#endif
}

#endif /* _GDISP_LLD_BOARD_H */
