/*
    Copyright (C) 2016 Stephane D'Alu

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/* Board identifier. */
#define BOARD_NRF52_IDES_OF_DEFCON
#define BOARD_NAME              "DC27 Ides of DEF CON"

/* Board oscillators-related settings. */
#define NRF5_XTAL_VALUE        32000000

/*
 * The NRF52840 uses two clocks: a high speed clock and a low speed one.
 * A 32MHz crystal is required for the high speed clock. The low speed
 * clock source can be one of three things:
 *
 * NRF_CLOCK_LF_SRC_RC (0)              Internal 32.768KHz RC oscillator
 * NRF_CLOCK_LF_SRC_XTAL (1)            External 32.768KHz crystal (P0.0/P0.1)
 * NRF_CLOCK_LF_SRC_SYNTH (2)           Synthesized from 32MHz high speed clock
 *
 * Using the SYNTH option gives us the best results with the smallest parts
 * count. (The BMD-340 module already has a 32MHz crystal inside.)
 */

#define NRF5_LFCLK_SOURCE      1

/*
 * IO pins assignments. GPIO port 1
 */


#define IOPORT1_I2S_AMPSD	2U
#define IOPORT1_TOUCH_CS        3U
#define IOPORT1_SCREEN_CD       4U
#define IOPORT1_UART_RTS        5U
#define IOPORT1_UART_TX         6U
#define IOPORT1_UART_CTS        7U
#define IOPORT1_UART_RX         8U
#define IOPORT1_NFC1            9U
#define IOPORT1_NFC2           10U
#define IOPORT1_BTN1           11U
#define IOPORT1_BTN2           12U
#define IOPORT1_SDCARD_CS      13U
#define IOPORT1_SAO_GPIO1      14U
#define IOPORT1_SAO_GPIO2      15U

#define IOPORT1_RESET          18U

#define IOPORT1_BTN3           24U
#define IOPORT1_BTN4           25U
#define IOPORT1_I2C_SCK        26U
#define IOPORT1_I2C_SDA        27U
#define IOPORT1_SPI_SCK        28U
#define IOPORT1_SPI_MISO       29U
#define IOPORT1_SPI_MOSI       30U
#define IOPORT1_SCREEN_CS      31U

/*
 * IO pins assignments. GPIO port 2
 * Note: To support a second bank of GPIO pins, Nordic essentially
 * expanded the pin definition from 5 bits to 6, where the 6th bit
 * represents the port index (0 or 1). So to simplify things,
 * all of the pins for this bank have an extra MSB set.
 */

#define IOPORT2_I2S_SDOUT      (1U | 0x20)
#define IOPORT2_I2S_SCK        (2U | 0x20)
#define IOPORT2_I2S_LRCK       (3U | 0x20)
#define IOPORT2_I2S_MCK        (4U | 0x20)

#define IOPORT2_SPI_SCK        (6U | 0x20)
#define IOPORT2_SPI_MISO       (7U | 0x20)
#define IOPORT2_SPI_MOSI       (8U | 0x20)

#define IOPORT2_BTN5           (9U | 0x20)
#define IOPORT2_BTN6          (10U | 0x20)
#define IOPORT2_BTN7          (11U | 0x20)
#define IOPORT2_BTN8          (12U | 0x20)
#define IOPORT2_BTN9          (13U | 0x20)
#define IOPORT2_BTN10         (14U | 0x20)
#define IOPORT2_JOYINTR       (15U | 0x20)

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
