#include <stdint.h>
#include "hal.h"
#include "led.h"
#include "badge.h"
#include "is31fl_lld.h"

unsigned char led_memory[ISSI_ADDR_MAX];

/* prototypes */
void led_set_all(uint8_t r, uint8_t g, uint8_t b);
void led_brightness_set(uint8_t brightness);
uint8_t led_brightness_get(void);
void led_init(void);
void led_clear(void);
void led_show(void);

const unsigned char led_address[LED_COUNT_INTERNAL][3] = {
    /* Here LEDs are in Green, Red, Blue order*/
    /* D201-D208 */
    {0x90, 0xA0, 0xB0}, {0x92, 0xA2, 0xB2}, {0x94, 0xA4, 0xB4},
    {0x96, 0xA6, 0xB6}, {0x98, 0xA8, 0xB8}, {0x9A, 0xAA, 0xBA},
    {0x9C, 0xAC, 0xBC}, {0x9E, 0xAE, 0xBE},

    /* D209-D216 */
    {0x60, 0x70, 0x80}, {0x62, 0x72, 0x82}, {0x64, 0x74, 0x84},
    {0x66, 0x76, 0x86}, {0x68, 0x78, 0x88}, {0x6A, 0x7A, 0x8A},
    {0x6C, 0x7C, 0x8C}, {0x6E, 0x7E, 0x8E},

    /* D217-D224 */
    {0x00, 0x10, 0x20}, {0x02, 0x12, 0x22}, {0x04, 0x14, 0x24},
    {0x06, 0x16, 0x26}, {0x08, 0x18, 0x28}, {0x0A, 0x1A, 0x2A},
    {0x0C, 0x1C, 0x2C}, {0x0E, 0x1E, 0x2E},

    /* D225-D232 */
    {0x30, 0x40, 0x50}, {0x32, 0x42, 0x52}, {0x34, 0x44, 0x54},
    {0x36, 0x46, 0x56}, {0x38, 0x48, 0x58}, {0x3A, 0x4A, 0x5A},
    {0x3C, 0x4C, 0x5C}, {0x3E, 0x4E, 0x5E},
};

const uint8_t gamma_values[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
    14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,
    31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,
    58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,
    75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
    96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119,
    120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146,
    148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177,
    180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252,
    255
};

/* Brightness control */
static uint8_t m_brightness = 10;

uint8_t led_brightness_get() {
  return m_brightness;
}

void led_brightness_set(uint8_t brightness) {
  m_brightness = brightness;
  drv_is31fl_gcc_set(m_brightness);
}

void led_init() {
  drv_is31fl_init();
  // on exit, the chip is now in the PWM page
  for (uint8_t i = 0; i < ISSI_ADDR_MAX; i++) {
    led_memory[i] = 0;
  }
}

void led_clear() {
  led_set_all(0, 0, 0);
  led_show();
}

void led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index < LED_COUNT) {
    led_memory[led_address[index][0]] = gamma_values[g / 2];
    led_memory[led_address[index][1]] = gamma_values[r / 2];
    led_memory[led_address[index][2]] = gamma_values[b / 2];
  }
}


void led_set_rgb(uint32_t index, color_rgb_t rgb) {
  led_set(index, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

void led_set_all(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    led_set(i, r, g, b);
  }
}

void led_set_all_rgb(color_rgb_t rgb) {
  for (uint32_t i = 0; i < LED_COUNT; i++) {
    led_set_rgb(i, rgb);
  }
}

void led_show() {
#ifndef CONFIG_BADGE_TYPE_STANDALONE
  drv_is31fl_set_page(ISSI_PAGE_PWM);

  for (uint8_t i = 0; i < LED_COUNT_INTERNAL; i++) {
    for (uint8_t ii = 0; ii < 3; ii++) {
      uint8_t address = led_address[i][ii];
      hal_i2c_write_reg_byte(LED_I2C_ADDR, address, led_memory[address]);
    }
  }
#endif
}

void led_test() {
    printf("Testing Red\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 255, 0, 0);
    }
    led_show();
    chThdSleepMilliseconds(2000);
    led_clear();

    printf("Testing Green\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 255, 0);
    }
    led_show();
    chThdSleepMilliseconds(2000);
    led_clear();

    printf("Testing Blue\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 0, 255);
    }
    led_show();
    chThdSleepMilliseconds(2000);
    led_clear();

#ifdef HSVTEST
    // HSV cycle
    for (uint8_t i = 0; i < 3; i++) {
      for (float h = 0; h < 1; h += 0.01) {
        color_rgb_t rgb = util_hsv_to_rgb(h, 1.0, 1.0);
        led_set_all_rgb(rgb);
        led_show();
        chThdSleepMilliseconds(500);
      }
    }
#endif
    led_clear();
    chThdSleepMilliseconds(1000);
}
