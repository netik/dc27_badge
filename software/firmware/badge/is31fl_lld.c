/*****************************************************************************
 * Made with beer and late nights in California.
 *
 * (C) Copyright 2017-2018 AND!XOR LLC (http://andnxor.com/).
 *
 * PROPRIETARY AND CONFIDENTIAL UNTIL AUGUST 7th, 2018 then,
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ADDITIONALLY:
 * If you find this source code useful in anyway, use it in another electronic
 * conference badge, or just think it's neat. Consider buying us a beer
 * (or two) and/or a badge (or two). We are just as obsessed with collecting
 * badges as we are in making them.
 *
 * Contributors:
 * 	@andnxor
 * 	@zappbrandnxor
 * 	@hyr0n1
 * 	@exc3ls1or
 * 	@lacosteaef
 * 	@bitstr3m
 *
 * Ported to ChiBiOS by John Adams and Bill Paul 3/2019
 *****************************************************************************/
#include <stdint.h>
#include "ch.h"
#include "hal.h"
#include "badge.h"
#include "led.h"
#include "is31fl_lld.h"

#define INIT_ATTEMPT_PERIOD_MS (10 * 1000) /* 10 second attempt period */

/* Prototypes */
bool drv_is31fl_init(void);

static bool m_initialized = false;
static uint32_t m_last_init_attempt = 0;

uint8_t hal_i2c_read_reg_byte(uint8_t i2c_address, uint8_t reg) {
  uint8_t txbuf;
  uint8_t rxbuf;
  msg_t r;

  rxbuf = 0xff;

  txbuf = reg;

  r = i2cMasterTransmitTimeout(&I2CD2, LED_I2C_ADDR, &txbuf, 1, &rxbuf, 1, ISSI_TIMEOUT);
  if (r != MSG_OK) {
    printf("i2cstatus = %ld\r\n", r);
    return 0;
  }

  return rxbuf;
}

bool hal_i2c_write_reg_byte(uint8_t i2c_address, uint8_t reg, uint8_t value) {
  uint8_t txbuf[2];
  msg_t r;
#ifdef LED_VERBOSE
  i2cflags_t e;
#endif

  txbuf[0] = reg;
  txbuf[1] = value;

  r = i2cMasterTransmitTimeout(&I2CD2, LED_I2C_ADDR, txbuf, 2, NULL, 0, ISSI_TIMEOUT);

#ifdef LED_VERBOSE
  if (r != MSG_OK) {
    printf("i2cstatus = %ld\r\n", r);
    e = i2cGetErrors(&I2CD2);

    printf("LED write failed = %ld ", i2cGetErrors(&I2CD2));

    if (e & I2C_NO_ERROR) { printf("I2C_NO_ERROR "); }
    if (e & I2C_BUS_ERROR) { printf("I2C_BUS_ERROR "); }
    if (e & I2C_ARBITRATION_LOST) { printf("I2C_ARBITRATION_LOST "); }
    if (e & I2C_ACK_FAILURE) { printf("I2C_ACK_FAILURE "); }
    if (e & I2C_OVERRUN) { printf("I2C_OVERRUN "); }
    if (e & I2C_PEC_ERROR) { printf("I2C_PEC_ERROR "); }
    if (e & I2C_TIMEOUT) { printf("I2C_TIMEOUT "); }
    if (e & I2C_SMB_ALERT) { printf("I2C_SMB_ALERT "); }

    printf("\r\n");
  }
#endif

  return (r == MSG_OK);
}

bool drv_is31fl_init(void) {
  // Avoid initializing too often
  if (m_last_init_attempt > 0) {
    if (m_initialized ||
        (chVTGetSystemTime() - m_last_init_attempt) < INIT_ATTEMPT_PERIOD_MS) {
      return m_initialized;
    }
  }

  m_last_init_attempt = chVTGetSystemTime();

  // disable soft shut down and enable the OSD checks
  drv_is31fl_set_page(ISSI_PAGE_FUNCTION);
  if (!hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_CONFIG,
                              ISSI_REG_CONFIG_SSD_OFF | ISSI_REG_CONFIG_OSD)) {
    //post_state_get()->led_driver_ack = false;
    return false;
  }

  // Set global current control
  drv_is31fl_set_page(ISSI_PAGE_FUNCTION);
  hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_GCC, 0xFF);
  chThdSleepMilliseconds(10);

  // Turn off all LEDs
  drv_is31fl_set_page(ISSI_PAGE_LED);
  for (uint8_t i = 0; i <= 0x17; i++) {
    hal_i2c_write_reg_byte(LED_I2C_ADDR, i, 0x00);
  }

  chThdSleepMilliseconds(100);

  drv_is31fl_set_page(ISSI_PAGE_FUNCTION);
  if (!hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_CONFIG,
                              ISSI_REG_CONFIG_OSD | ISSI_REG_CONFIG_SSD_OFF)) {
    //post_state_get()->led_driver_ack = false;
    return false;
  }

  chThdSleepMilliseconds(100);

  // PWM the LEDs back to 0
  drv_is31fl_set_page(ISSI_PAGE_PWM);
  for (uint8_t i = 0; i <= 0xBE; i += 1) {
    hal_i2c_write_reg_byte(LED_I2C_ADDR, i, 0x00);
  }

  chThdSleepMilliseconds(100);

  // Turn on all LEDs
  drv_is31fl_set_page(ISSI_PAGE_LED);
  for (uint8_t i = 0; i <= 0x17; i++) {
    hal_i2c_write_reg_byte(LED_I2C_ADDR, i, 0xFF);
  }

  // get the open/short status
  drv_is31fl_set_page(ISSI_PAGE_LED);
  for (uint8_t i = 0x18 ; i <= 0x2f ; i++) {
    uint8_t state;
    state = hal_i2c_read_reg_byte(LED_I2C_ADDR, i);
    if (state) printf("LED OPEN! address %x\r\n", i);
  }

  drv_is31fl_set_page(ISSI_PAGE_LED);
  for (uint8_t i = 0x30 ; i <= 0x47 ; i++) {
    uint8_t state;
    state = hal_i2c_read_reg_byte(LED_I2C_ADDR, i);
    if (state) printf("LED SHORT! address %x\r\n", i);
  }
  drv_is31fl_set_page(ISSI_PAGE_PWM);

  printf("IS31FL3736 driver started.\r\n");

  m_initialized = true;
  //post_state_get()->led_driver_ack = true;
  return true;
}

void drv_is31fl_gcc_set(uint8_t gcc) {
#ifndef CONFIG_BADGE_TYPE_STANDALONE
  // Lazy init
  drv_is31fl_init();

  drv_is31fl_set_page(ISSI_PAGE_FUNCTION);
  hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_GCC, gcc);
  drv_is31fl_set_page(ISSI_PAGE_PWM);
#endif
}

void drv_is31fl_send_value(uint8_t address, uint8_t value) {
#ifndef CONFIG_BADGE_TYPE_STANDALONE
  // Lazy init
  drv_is31fl_init();

  if (m_initialized) {
    if (!hal_i2c_write_reg_byte(LED_I2C_ADDR, address, value)) {
      m_initialized = false;
    }
  }
#endif
}

/**
 * Select page to write to
 */
void drv_is31fl_set_page(uint8_t page) {
#ifndef CONFIG_BADGE_TYPE_STANDALONE
  hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_WRITE_LOCK,
                         ISSI_REG_WRITE_UNLOCK);
  hal_i2c_write_reg_byte(LED_I2C_ADDR, ISSI_REG_COMMAND,
                         page);
#endif
}
