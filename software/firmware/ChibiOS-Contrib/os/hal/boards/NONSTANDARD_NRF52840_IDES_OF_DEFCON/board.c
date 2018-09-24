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
        PAL_MODE_UNCONNECTED,         /* P0.0 : XTAL (32MHz)   */
        PAL_MODE_UNCONNECTED,         /* P0.1 : XTAL (32MHz)   */
        PAL_MODE_UNCONNECTED,         /* P0.2 :                */
        PAL_MODE_UNCONNECTED,         /* P0.3 :                */
        PAL_MODE_UNCONNECTED,         /* P0.4 :                */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.5 : UART_RTS       */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.6 : UART_TX        */
        PAL_MODE_UNCONNECTED,         /* P0.7 : UART_CTS       */
        PAL_MODE_UNCONNECTED,         /* P0.8 : UART_RX        */
        PAL_MODE_UNCONNECTED,         /* P0.9 : NFC antenna    */
        PAL_MODE_UNCONNECTED,         /* P0.10: NFC antenna    */
        PAL_MODE_INPUT_PULLUP,        /* P0.11: BTN1           */
        PAL_MODE_INPUT_PULLUP,        /* P0.12: BTN2           */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.13: LED1           */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.14: LED2           */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.15: LED3           */
        PAL_MODE_OUTPUT_PUSHPULL,     /* P0.16: LED4           */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.17: QSPI CS        */
        PAL_MODE_UNCONNECTED,         /* P0.18: RESET          */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.19: QSPI CLK       */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.20: QSPI DIO0      */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.21: QSPI_DIO1      */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.22: QSPI_DIO2      */
        PAL_MODE_OUTPUT_PULLUP,       /* P0.23: QSPI_DIO3      */
        PAL_MODE_INPUT_PULLUP,        /* P0.24: BTN3           */
        PAL_MODE_INPUT_PULLUP,        /* P0.25: BTN4           */
        PAL_MODE_UNCONNECTED,         /* P0.26:                */
        PAL_MODE_UNCONNECTED,         /* P0.27:                */
        PAL_MODE_UNCONNECTED,         /* P0.28:                */
        PAL_MODE_UNCONNECTED,         /* P0.29:                */
        PAL_MODE_UNCONNECTED,         /* P0.30:                */
        PAL_MODE_UNCONNECTED,         /* P0.31:                */
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
        PAL_MODE_UNCONNECTED,         /* P1.0 :                */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.1 : Touch CS       */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.2 : Screen CD      */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.3 : SD card CS     */
        PAL_MODE_UNCONNECTED,         /* P1.4 :                */
        PAL_MODE_UNCONNECTED,         /* P1.5 : SPI_SCK        */
        PAL_MODE_UNCONNECTED,         /* P1.6 : SPI_MISO       */
        PAL_MODE_UNCONNECTED,         /* P1.7 : SPI_MOSI       */
        PAL_MODE_OUTPUT_PULLUP,       /* P1.8 : Screen CS      */
        PAL_MODE_UNCONNECTED,         /* P1.9 :                */
        PAL_MODE_UNCONNECTED,         /* P1.10:                */
        PAL_MODE_UNCONNECTED,         /* P1.11:                */
        PAL_MODE_UNCONNECTED,         /* P1.12:                */
        PAL_MODE_UNCONNECTED,         /* P1.13:                */
        PAL_MODE_UNCONNECTED,         /* P1.14:                */
        PAL_MODE_UNCONNECTED,         /* P1.15:                */
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
