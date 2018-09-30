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
#define NRF5_LFCLK_SOURCE      1

/*
 * GPIO pins. 
 */
/* Defined by board */
#define BTN1           11U
#define BTN2           12U
#define BTN3           24U
#define BTN4           25U
#define BTN5           18U
#define LED1           12U
#define LED2           14U
#define LED3           15U
#define LED4           16U
#define UART_RTS        5U
#define UART_TX         6U
#define UART_CTS        7U
#define UART_RX         8U
#define NFC1            9U
#define NFC2           10U
#define I2C_SCL        27U
#define I2C_SDA        26U

/* Our definitions */
#define SPI_SCK        25U
#define SPI_MOSI       24U
#define SPI_MISO       23U
#define SPI_SS         22U

/* Analog input */
#define AIN0            2U
#define AIN1            3U
#define AIN2            4U
#define AIN3            5U
#define AIN4           28U
#define AIN5           29U
#define AIN6           30U
#define AIN7           31U
#define AREF0         AIN0
#define AREF1         AIN1

/*
 * IO pins assignments.
 */
/* Defined by board */
#define IOPORT1_BTN1           11U
#define IOPORT1_BTN2           12U
#define IOPORT1_BTN3           24U
#define IOPORT1_BTN4           25U
#define IOPORT1_LED1           13U
#define IOPORT1_LED2           14U
#define IOPORT1_LED3           14U
#define IOPORT1_LED4           16U
#define IOPORT1_UART_RTS        5U
#define IOPORT1_UART_TX         6U
#define IOPORT1_UART_CTS        7U
#define IOPORT1_UART_RX         8U
#define IOPORT1_NFC1            9U
#define IOPORT1_NFC2           10U
#define IOPORT1_RESET          18U

/* Our definitions */

#define IOPORT1_I2C_SCK         7U
#define IOPORT1_I2C_SDA         8U

#define IOPORT1_SPI_SCK        28U
#define IOPORT1_SPI_MISO       29U
#define IOPORT1_SPI_MOSI       30U
#define IOPORT1_SCREEN_CS      31U

#define IOPORT1_SDCARD_CS      13U
#define IOPORT2_SCREEN_CD       2U
#define IOPORT2_TOUCH_CS        1U

#define IOPORT1_QSPI_SCK       19U
#define IOPORT1_QSPI_CS        17U
#define IOPORT1_QSPI_DIO0      20U
#define IOPORT1_QSPI_DIO1      21U
#define IOPORT1_QSPI_DIO2      22U
#define IOPORT1_QSPI_DIO3      23U


#define IOPORT1_SCREEN_D0      24U
#define IOPORT1_SCREEN_D1      26U
#define IOPORT1_SCREEN_D2      26U
#define IOPORT1_SCREEN_D3      27U
#define IOPORT1_SCREEN_D4      28U
#define IOPORT1_SCREEN_D5      29U
#define IOPORT1_SCREEN_D6      30U
#define IOPORT1_SCREEN_D7      31U

/* Analog inpupt */
#define IOPORT1_AIN0            2U
#define IOPORT1_AIN1            3U
#define IOPORT1_AIN2            4U
#define IOPORT1_AIN3            5U
#define IOPORT1_AIN4           28U
#define IOPORT1_AIN5           29U
#define IOPORT1_AIN6           30U
#define IOPORT1_AIN7           31U
#define IOPORT1_AREF0           IOPORT1_AIN0
#define IOPORT1_AREF1           IOPORT1_AIN1

/* Arduino naming */
#define IOPORT1_A0              3U
#define IOPORT1_A1              4U
#define IOPORT1_A2             28U
#define IOPORT1_A3             39U
#define IOPORT1_A4             30U
#define IOPORT1_A5             31U
#define IOPORT1_D0             11U
#define IOPORT1_D1             12U
#define IOPORT1_D2             13U
#define IOPORT1_D3             14U
#define IOPORT1_D4             15U
#define IOPORT1_D5             16U
#define IOPORT1_D6             17U
#define IOPORT1_D7             18U
#define IOPORT1_D8             19U
#define IOPORT1_D9             20U
#define IOPORT1_D10            22U
#define IOPORT1_D11            23U
#define IOPORT1_D12            24U
#define IOPORT1_D13            25U


/*
 * IO lines assignments.
 */
/* Board defined */
#define LINE_BTN1      PAL_LINE(IOPORT1, IOPORT1_BTN1)
#define LINE_BTN2      PAL_LINE(IOPORT1, IOPORT1_BTN2)
#define LINE_BTN3      PAL_LINE(IOPORT1, IOPORT1_BTN3)
#define LINE_BTN4      PAL_LINE(IOPORT1, IOPORT1_BTN4)
#define LINE_LED1      PAL_LINE(IOPORT1, IOPORT1_LED1)
#define LINE_LED2      PAL_LINE(IOPORT1, IOPORT1_LED2)
#define LINE_LED3      PAL_LINE(IOPORT1, IOPORT1_LED3)
#define LINE_LED4      PAL_LINE(IOPORT1, IOPORT1_LED4)
#define LINE_UART_RTS  PAL_LINE(IOPORT1, IOPORT1_UART_RTS)
#define LINE_UART_TX   PAL_LINE(IOPORT1, IOPORT1_UART_TX)
#define LINE_UART_CTS  PAL_LINE(IOPORT1, IOPORT1_UART_CTS)
#define LINE_UART_RX   PAL_LINE(IOPORT1, IOPORT1_UART_RX)
#define LINE_NFC1      PAL_LINE(IOPORT1, IOPORT1_NFC1)
#define LINE_NFC2      PAL_LINE(IOPORT1, IOPORT1_NFC2)
#define LINE_I2C_SCL   PAL_LINE(IOPORT1, IOPORT1_I2C_SCL)
#define LINE_I2C_SDA   PAL_LINE(IOPORT1, IOPORT1_I2C_SDA)

/* Our definitions */
#define LINE_SPI_SCK   PAL_LINE(IOPORT1, IOPORT1_SPI_SCK)
#define LINE_SPI_MOSI  PAL_LINE(IOPORT1, IOPORT1_SPI_MOSI)
#define LINE_SPI_MISO  PAL_LINE(IOPORT1, IOPORT1_SPI_MISO)
#define LINE_SPI_SS    PAL_LINE(IOPORT1, IOPORT1_SPI_SS)

/* Analog line */
#define LINE_AIN0      PAL_LINE(IOPORT1, IOPORT1_AIN0)
#define LINE_AIN1      PAL_LINE(IOPORT1, IOPORT1_AIN1)
#define LINE_AIN2      PAL_LINE(IOPORT1, IOPORT1_AIN2)
#define LINE_AIN3      PAL_LINE(IOPORT1, IOPORT1_AIN3)
#define LINE_AIN4      PAL_LINE(IOPORT1, IOPORT1_AIN4)
#define LINE_AIN5      PAL_LINE(IOPORT1, IOPORT1_AIN5)
#define LINE_AIN6      PAL_LINE(IOPORT1, IOPORT1_AIN6)
#define LINE_AIN7      PAL_LINE(IOPORT1, IOPORT1_AIN7)
#define LINE_AREF0     PAL_LINE(IOPORT1, IOPORT1_AREF0)
#define LINE_AREF1     PAL_LINE(IOPORT1, IOPORT1_AREF1)

/* Arduino naming */
#define LINE_A0        PAL_LINE(IOPORT1, IOPORT1_A0)
#define LINE_A1        PAL_LINE(IOPORT1, IOPORT1_A1)
#define LINE_A2        PAL_LINE(IOPORT1, IOPORT1_A2)
#define LINE_A3        PAL_LINE(IOPORT1, IOPORT1_A3)
#define LINE_A4        PAL_LINE(IOPORT1, IOPORT1_A4)
#define LINE_A5        PAL_LINE(IOPORT1, IOPORT1_A5)
#define LINE_D0        PAL_LINE(IOPORT1, IOPORT1_D0)
#define LINE_D1        PAL_LINE(IOPORT1, IOPORT1_D1)
#define LINE_D2        PAL_LINE(IOPORT1, IOPORT1_D2)
#define LINE_D3        PAL_LINE(IOPORT1, IOPORT1_D3)
#define LINE_D4        PAL_LINE(IOPORT1, IOPORT1_D4)
#define LINE_D5        PAL_LINE(IOPORT1, IOPORT1_D5)
#define LINE_D6        PAL_LINE(IOPORT1, IOPORT1_D6)
#define LINE_D7        PAL_LINE(IOPORT1, IOPORT1_D7)
#define LINE_D8        PAL_LINE(IOPORT1, IOPORT1_D8)
#define LINE_D9        PAL_LINE(IOPORT1, IOPORT1_D9)
#define LINE_D10       PAL_LINE(IOPORT1, IOPORT1_D10)
#define LINE_D11       PAL_LINE(IOPORT1, IOPORT1_D11)
#define LINE_D12       PAL_LINE(IOPORT1, IOPORT1_D12)
#define LINE_D13       PAL_LINE(IOPORT1, IOPORT1_D13)

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
