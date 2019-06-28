/*
    Copyright (C) 2016 Stéphane D'Alu

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

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
const PALConfig pal_default_config =
{
  .pads = {
        PAL_MODE_UNCONNECTED,         /* P0.0 : XTAL(32.768KHz)*/
        PAL_MODE_UNCONNECTED,         /* P0.1 : XTAL(32.768KHz)*/
        PAL_MODE_OUTPUT_PULLUP,       /* P0.2 : Audio shutdown */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.3 : Touch CS       */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.4 : Screen CD      */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.5 : UART_RTS       */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.6 : UART_TX        */
        PAL_MODE_UNCONNECTED,         /* P0.7 : UART_CTS       */
        PAL_MODE_UNCONNECTED,         /* P0.8 : UART_RX        */
        PAL_MODE_UNCONNECTED,         /* P0.9 : NFC antenna    */
        PAL_MODE_UNCONNECTED,         /* P0.10: NFC antenna    */
        PAL_MODE_INPUT_PULLUP,        /* P0.11: BTN1 (A:up)    */
        PAL_MODE_INPUT_PULLUP,        /* P0.12: BTN2 (A:down)  */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.13: SD card CS     */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.14: SAO GPIO1      */
        PAL_MODE_INPUT_PULLUP,        /* P0.15: SAO GPIO2      */
        PAL_MODE_UNCONNECTED,         /* P0.16: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.17: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.18: RESET          */
        PAL_MODE_UNCONNECTED,         /* P0.19: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.20: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.21: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.22: <unused>       */
        PAL_MODE_UNCONNECTED,         /* P0.23: <unused>       */
        PAL_MODE_INPUT_PULLUP,        /* P0.24: BTN3 (A:left)  */
        PAL_MODE_INPUT_PULLUP,        /* P0.25: BTN4 (A:right) */
        PAL_MODE_OUTPUT_OPENDRAIN,    /* P0.26: I2C SCK        */
        PAL_MODE_OUTPUT_OPENDRAIN,    /* P0.27: I2C SDA        */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.28: SPI SCK        */
        PAL_MODE_INPUT_PULLUP,        /* P0.29: SPI MISO       */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.30: SPI MOSI       */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.31: Screen CS      */
  },
  .port = IOPORT1
};

/* 
 * The NRF52840 has a second bank of 16 GPIO pins on a separate port.
 * ChibiOS doesn't have an internal call to initialize a secondary
 * GPIO bank, but that's ok: we can do it in the BSP.
 */

const PALConfig pal_default_config_p1 =
{
  .pads = {
        PAL_MODE_INPUT_PULLUP,        /* P1.0 :                */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.1 : I2S SDOUT      */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.2 : I2S SCK        */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.3 : I2S LRCK       */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.4 : I2S MCK        */
        PAL_MODE_UNCONNECTED,         /* P1.5 :                */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.6 : SPI SCK        */
        PAL_MODE_INPUT_PULLUP,        /* P1.7 : SPI MISO       */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.8 : SPI MOSI       */
        PAL_MODE_INPUT_PULLUP,        /* P1.9 : BTN6 (B:up)    */
        PAL_MODE_INPUT_PULLUP,        /* P1.10: BTN7 (B:down)  */
        PAL_MODE_INPUT_PULLUP,        /* P1.11: BTN8 (B:left)  */
        PAL_MODE_INPUT_PULLUP,        /* P1.12: BTN9 (B:right) */
        PAL_MODE_INPUT_PULLUP,        /* P1.13: BTN5 (A:sel)   */
        PAL_MODE_INPUT_PULLUP,        /* P1.14: BTN10 (B:sel)  */
        PAL_MODE_INPUT_PULLUP,        /* P1.15: JOYPAD_INTR    */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
        PAL_MODE_UNCONNECTED,         /* P1.XX:                */
  },
  .port = IOPORT2
};

#endif

/**
 * @brief   Early initialization code.
 * @details This initialization is performed just after reset before BSS and
 *          DATA segments initialization.
 */
void __early_init(void)
{
}

/**
 * @brief   Late initialization code.
 * @note    This initialization is performed after BSS and DATA segments
 *          initialization and before invoking the main() function.
 */
void boardInit(void)
{
	palInit (&pal_default_config_p1);
}
