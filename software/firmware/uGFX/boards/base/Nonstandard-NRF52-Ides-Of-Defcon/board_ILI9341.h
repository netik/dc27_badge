/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

/*
 * For the DC26 board, we use the display in parallel I/O mode. This is
 * because the NRF52832's SPI bus has a max serial clock rate of 8MHz. The
 * KW01 had a max rate of 12MHz, which allowed us to achieve a max frame
 * rate of 8 frames/second. At 8MHz, the frame rate would be even worse
 * (maybe 5fps max). This is unacceptable. Unfortunately while the NRF52840
 * chip has a much faster SPI bus, it's not clear if it will be in production
 * in time to have the DC26 badge ready for DEF CON, so our only option
 * is to stick with the NRF52832 and set up the screen for parallel I/O
 * instead of serial.
 *
 * The NRF52832 only has 32 GPIO pins. The best display performance would
 * be achieved by using 16-bit parallel mode, but that would require more
 * than half the available pins, which would prevent us from accomodating
 * all the other desired peripherals for the DC26 design. So we compromise
 * and use 8-bit mode instead. This means we need 8 GPIO pins for display
 * data. To keep things simple, we use P24 through P31 so that we don't
 * need to perform excessive bit manipulation (all 8 bits remain contiguous).
 *
 * We take advantage of the 'set' and 'clear' GPIO registers to allow us to
 * toggle the display control and data pins without affecting other pins.
 * In this there are no overlapped accesses with other code using the GPIO
 * pins and we can avoid the need for a mutex.
 *
 * In addition to the 8 data pins, we need a bare minimum of two additional
 * control pins: one for the command/data select and one for write control.
 * The ILI9341 also include a chip select pin, however we can get away with
 * leaving this pin permanenly asserted (grounded) because all the data pins
 * are used exclusively by the display controller, so there's no other device
 * that might collide with it.
 */

#define ILI9341_WR		0x00000800	/* Write signal */
#define ILI9341_CD		0x00001000	/* Command/data select */

#define ILI9341_DATA		0xFF000000
#define ILI9341_PIXEL_LO(x)	(((x) << 24) & ILI9341_DATA)
#define ILI9341_PIXEL_HI(x)	(((x) << 16) & ILI9341_DATA)

__attribute__ ((noinline))
static void init_board(GDisplay *g) {
	(void) g;

	/* Set all pins to initial (high) state */

	palSetPad (IOPORT1, IOPORT1_SCREEN_WR);
	palSetPad (IOPORT1, IOPORT1_SCREEN_CD);

	palSetPad (IOPORT1, IOPORT1_SCREEN_D0);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D1);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D2);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D3);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D4);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D5);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D6);
	palSetPad (IOPORT1, IOPORT1_SCREEN_D7);

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
	return;
}

static void release_bus(GDisplay *g) {
	(void) g;
	return;
}

__attribute__ ((noinline))
static void write_index(GDisplay *g, uint16_t index) {
	(void) g;

	/*
         * Assert command/data pin (select command),
         * assert the write command pin, clear data bits.
         */

	IOPORT1->OUTCLR = ILI9341_WR|ILI9341_CD|ILI9341_DATA;

	/*
	 * Write data bits and then de-assert the write command pin
	 * to latch the data into the display controller.
	 */

	IOPORT1->OUTSET = ILI9341_PIXEL_LO(index);
	IOPORT1->OUTSET = ILI9341_WR;

	/* Deassert command/data and write pins. */

	IOPORT1->OUTSET = ILI9341_CD;

	return;
}

__attribute__ ((noinline))
static void write_data(GDisplay *g, uint16_t data) {
	(void) g;

	/* Assert write command pin and clear data */

	IOPORT1->OUTCLR = ILI9341_WR|ILI9341_DATA;

	/*
	 * Write data bits and then de-assert the write command pin
	 * to latch the data into the display controller.
	 */

	IOPORT1->OUTSET = ILI9341_PIXEL_LO(data);
	IOPORT1->OUTSET = ILI9341_WR;

	return;
}

__attribute__ ((noinline))
static void write_data16(GDisplay *g, uint16_t data) {
	(void) g;

	IOPORT1->OUTCLR = ILI9341_WR|ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_HI(data);
	IOPORT1->OUTSET = ILI9341_WR;

	IOPORT1->OUTCLR = ILI9341_WR|ILI9341_DATA;
	IOPORT1->OUTSET = ILI9341_PIXEL_LO(data);
	IOPORT1->OUTSET = ILI9341_WR;

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
