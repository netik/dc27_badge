#include <stdint.h>
#include "hal.h"
#include "led.h"
#include "badge.h"
#include "is31fl_lld.h"
#include "math.h"
#include "userconfig.h"
#include "rand.h"

#define MILLIS() chVTGetSystemTime()

unsigned char led_memory[ISSI_ADDR_MAX];

/* prototypes */
void led_set_all(uint8_t r, uint8_t g, uint8_t b);
void led_brightness_set(uint8_t brightness);
uint8_t led_brightness_get(void);
bool led_init(void);
void led_clear(void);
void led_show(void);
void ledSetPattern(uint8_t);

void led_pattern_balls_init(led_pattern_balls_t *);
void led_pattern_balls(led_pattern_balls_t*);
void led_pattern_flame(void);
void led_pattern_kitt(int8_t*, int8_t*);
void led_pattern_triangle(int8_t*, int8_t*);
void led_pattern_sparkle(uint8_t*);
void led_pattern_double_sweep(uint8_t* p_index, float* p_hue, float* p_value);

/* control vars */
static thread_t * pThread;
static uint8_t ledExitRequest = 0;
uint8_t ledsOff = 1;
// the current function that updates the LEDs. Override with ledSetFunction();
static uint8_t led_current_func = 1;
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
    {0x30, 0x40, 0x50}, {0x32, 0x42, 0x52}, {0x34, 0x44, 0x54},
    {0x36, 0x46, 0x56}, {0x38, 0x48, 0x58}, {0x3A, 0x4A, 0x5A},
    {0x3C, 0x4C, 0x5C}, {0x3E, 0x4E, 0x5E},

    /* D225-D232 */
    {0x00, 0x10, 0x20}, {0x02, 0x12, 0x22}, {0x04, 0x14, 0x24},
    {0x06, 0x16, 0x26}, {0x08, 0x18, 0x28}, {0x0A, 0x1A, 0x2A},
    {0x0C, 0x1C, 0x2C}, {0x0E, 0x1E, 0x2E},

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

// common colors
const color_rgb_t roygbiv[7] = { 0xff0000,
                                 0xff3200,
                                 0xffff00,
                                 0x00ff00,
                                 0x0000ff,
                                 0x4B0082,
                                 0xee82ee
                               };


const char *fxlist[] = {
  "Off",
  "Flame",
  "Balls",
  "Larson Scanner",
  "Sparkle",
  "Double Sweep",
  "Triangle",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx",
  "xx"
};

/* Brightness control */
static uint8_t led_brightness_level = 0xff;

uint8_t led_brightness_get() {
  return led_brightness_level;
}

void led_brightness_set(uint8_t brightness) {
  led_brightness_level = brightness;
}

/* color utils */
color_rgb_t util_hsv_to_rgb(float H, float S, float V) {
  H = fmodf(H, 1.0);

  float h = H * 6;
  uint8_t i = floor(h);
  float a = V * (1 - S);
  float b = V * (1 - S * (h - i));
  float c = V * (1 - (S * (1 - (h - i))));
  float rf, gf, bf;

  switch (i) {
    case 0:
      rf = V * 255;
      gf = c * 255;
      bf = a * 255;
      break;
    case 1:
      rf = b * 255;
      gf = V * 255;
      bf = a * 255;
      break;
    case 2:
      rf = a * 255;
      gf = V * 255;
      bf = c * 255;
      break;
    case 3:
      rf = a * 255;
      gf = b * 255;
      bf = V * 255;
      break;
    case 4:
      rf = c * 255;
      gf = a * 255;
      bf = V * 255;
      break;
    case 5:
    default:
      rf = V * 255;
      gf = a * 255;
      bf = b * 255;
      break;
  }

  uint8_t R = rf;
  uint8_t G = gf;
  uint8_t B = bf;

  color_rgb_t RGB = (R << 16) + (G << 8) + B;
  return RGB;
}

bool led_init() {
  // on exit, the chip is now in the PWM page
  for (uint8_t i = 0; i < ISSI_ADDR_MAX; i++) {
    led_memory[i] = 0;
  }

  return drv_is31fl_init();
}

void led_clear() {
  led_set_all(0, 0, 0);
  led_show();
}

void ledSetPattern(uint8_t patt) {
  led_current_func = patt;
}

void led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index < LED_COUNT) {
    led_memory[led_address[index][0]] = g;
    led_memory[led_address[index][1]] = r;
    led_memory[led_address[index][2]] = b;
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
  drv_is31fl_set_page(ISSI_PAGE_PWM);

  for (uint8_t i = 0; i < LED_COUNT_INTERNAL; i++) {
    for (uint8_t ii = 0; ii < 3; ii++) {
      uint8_t address = led_address[i][ii];
      hal_i2c_write_reg_byte(LED_I2C_ADDR, address, led_memory[address]);
    }
  }
}

void led_test() {
    printf("LED Test: Red\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 255, 0, 0);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();

    printf("LED Test: Green\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 255, 0);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();

    printf("LED Test: Blue\r\n");
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 0, 255);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();
#ifdef HSV_TEST
    // HSV cycle
    for (uint8_t i = 0; i < 3; i++) {
      for (float h = 0; h < 1; h += 0.01) {
        color_rgb_t rgb = util_hsv_to_rgb(h, 1.0, 1.0);
        led_set_all_rgb(rgb);
        led_show();
        chThdSleepMilliseconds(500);
      }
    }

    led_clear();

    chThdSleepMilliseconds(1000);
#endif
}

/* Threads ------------------------------------------------------ */
static THD_WORKING_AREA(waBlingThread, 512);
static THD_FUNCTION(bling_thread, arg) {
  userconfig *config = getConfig();
  /* animation state vars */
  led_pattern_balls_t anim_balls;
  int8_t anim_index = 0;
  uint8_t anim_uindex = 0;
  int8_t anim_position = 0;
  float anim_hue = 100;
  float anim_value = 0;
  uint8_t last_brightness = led_brightness_level;

  (void)arg;
  chRegSetThreadName("LED Bling");

  drv_is31fl_gcc_set(led_brightness_level);
	led_pattern_balls_init(&anim_balls);
  led_current_func = config->led_pattern;

  while (!ledsOff) {
    // wait until the next update cycle
    chThdYield();
    chThdSleepMilliseconds(EFFECTS_REDRAW_MS);

    // do we need to update brightness?
    if (last_brightness != led_brightness_level) {
      drv_is31fl_gcc_set(led_brightness_level);
      last_brightness = led_brightness_level;
    }
    // re-render the internal framebuffer animations
    if (led_current_func != 0) {
      switch (led_current_func) {
      case 1:
        led_pattern_flame();
        break;
      case 2:
        led_pattern_balls(&anim_balls);
        break;
      case 3:
        led_pattern_kitt(&anim_index, &anim_position);
        break;
      case 4:
        led_pattern_sparkle(&anim_uindex);
        break;
      case 5:
        led_pattern_double_sweep(&anim_uindex, &anim_hue, &anim_value);
        break;
      case 6:
        led_pattern_triangle(&anim_index, &anim_position);
        break;
        // ... add more animations here ...
      }
    }

    if( ledExitRequest ) {
      // force one full cycle through an update on request to force LEDs off
      led_set_all(0,0,0);
      led_show();
      ledsOff = 1;

      chThdExitS(MSG_OK);
    }
  }
  return;
}

void ledStart(void) {
  ledExitRequest = 0;
  ledsOff = 0;

  /* now that we're not using WS2812B's, we can make this a
   * normal priority thread */
  pThread = chThdCreateStatic(waBlingThread, sizeof(waBlingThread),
                    NORMALPRIO, bling_thread, NULL);
}

uint8_t ledStop(void) {
  ledExitRequest = 1;

  chThdWait(pThread);
  pThread = NULL;

  return ledsOff;
}

/* Animations ------------------------------------------------------------- */

/**
 * @brief Initilize balls
 * @param p_balls : Pointer to balls
 */
void led_pattern_balls_init(led_pattern_balls_t* p_balls) {
  p_balls->start_height = 1;
  p_balls->gravity = -9.81;
  p_balls->impact_velocity_start =
      sqrt(-2 * p_balls->gravity * p_balls->start_height);

  // Initialize the led balls state
  for (int i = 0; i < LED_PATTERN_BALLS_COUNT; i++) {
    p_balls->clock_since_last_bounce[i] = MILLIS();
    p_balls->height[i] = p_balls->start_height;
    p_balls->position[i] = 0;
    p_balls->impact_velocity[i] = p_balls->impact_velocity_start;
    p_balls->time_since_last_bounce[i] = 0;
    p_balls->dampening[i] = 0.90 - (float)i / pow(LED_PATTERN_BALLS_COUNT, 2);
    p_balls->colors[i] =
        util_hsv_to_rgb((float)randRange(0, 100) / 100.0, 1, 1);
  }
}

/**
 * @brief Bouncing ball LED pattern mode. Ported from tweaking4all
 * @param p_balls : Pointer to balls state
 */
void led_pattern_balls(led_pattern_balls_t* p_balls) {
  for (int i = 0; i < LED_PATTERN_BALLS_COUNT; i++) {
    p_balls->time_since_last_bounce[i] =
        MILLIS() - p_balls->clock_since_last_bounce[i];
    p_balls->height[i] =
        0.5 * p_balls->gravity *
            pow(p_balls->time_since_last_bounce[i] / 1000, 2.0) +
        p_balls->impact_velocity[i] * p_balls->time_since_last_bounce[i] / 1000;

    if (p_balls->height[i] < 0) {
      p_balls->height[i] = 0;
      p_balls->impact_velocity[i] =
          p_balls->dampening[i] * p_balls->impact_velocity[i];
      p_balls->clock_since_last_bounce[i] = MILLIS();

      // Bouncing has stopped, start over
      if (p_balls->impact_velocity[i] < 0.01) {
        p_balls->impact_velocity[i] = p_balls->impact_velocity_start;
        p_balls->colors[i] =
            util_hsv_to_rgb((float)randRange(0, 100) / 100.0, 1, 1);
      }
    }
    p_balls->position[i] =
        round(p_balls->height[i] * (LED_COUNT - 1) / p_balls->start_height);
  }

  for (int i = 0; i < LED_PATTERN_BALLS_COUNT; i++) {
    led_set_rgb(p_balls->position[i], p_balls->colors[i]);
  }
  led_show();
  led_set_all(0, 0, 0);
}

/**
 * @brief Flame LED mode
 */
void led_pattern_flame() {
  uint8_t red, green;
  for (int x = 0; x < LED_COUNT; x++) {
    // 1 in 4 chance of pixel changing
    if (randRange(0, 4) == 0) {
      // Pick a random red color and ensure green never exceeds it ensuring some
      // shade of red orange or yellow
      red = randRange(180, 255);
      green = randRange(0, red);
      led_set(x, red, green, 0);
    }
  }
  led_show();
}

/**
 * @brief Do a kitt (knight rider) like pattern because of course
 * @param p_index : Pointer to current index
 * @param p_direction : Pointer to current direction
 */
void led_pattern_kitt(int8_t* p_index, int8_t* p_direction) {
  int8_t index;
  led_set_all(0, 0, 0);

  for (int8_t i = 0; i < 4; i++) {
    if (*p_direction > 0) {
      index = *p_index - i;
    } else {
      index = *p_index + i;
    }

    if (index >= 0 && index < LED_COUNT) {
      led_set(index, 255 / (i + 1), 0, 0);
    }
  }

  led_show();

  if (*p_direction > 0) {
    (*p_index) += 2;
    if (*p_index >= LED_COUNT) {
      *p_index = LED_COUNT - 1;
      *p_direction = -1;
    }
  } else {
    (*p_index) -= 2;
    if (*p_index < 0) {
      *p_index = 0;
      *p_direction = 1;
    }
  }
}

void led_pattern_sparkle(uint8_t* p_index) {
  uint8_t mode = randRange(0, 2);

  switch (mode) {
    case 0:
      *p_index = randRange(0, LED_COUNT);
      led_set(*p_index, 255, 255, 255);
      break;
    case 1:;
      float hue = (float)randRange(0, 100) / 100.0;
      led_set_rgb(*p_index, util_hsv_to_rgb(hue, 1.0, 1.0));
      break;
  }
  led_show();
}

void led_pattern_double_sweep(uint8_t* p_index, float* p_hue, float* p_value) {
  color_rgb_t rgb = util_hsv_to_rgb(*p_hue, 1.0, *p_value);
  led_set_rgb(16, rgb);
  led_set_rgb(16 + *p_index, rgb);
  led_set_rgb(16 - *p_index, rgb);
  led_show();

  (*p_index)++;
  if (*p_index > 16) {
    *p_index = 0;

    // Ensure hue wraps around
    *p_hue += 0.1;
    if (*p_hue >= 1.0) {
      *p_hue -= 1.0;
    }

    // Alternate light / dark
    if (*p_value > 0) {
      *p_value = 0;
    } else {
      *p_value = 1.0;
    }
  }
}

void led_pattern_triangle(int8_t *fx_index, int8_t *fx_position) {
  // pulsating triangle, ramp up
  int16_t N3 = LED_COUNT/4;
  (*fx_index)++;
  if ( *fx_index % 3 != 0 ) { return; } // 1/3rd speed
  led_clear();

  for(int i = 0; i < LED_COUNT; i++ ) {
    if ((i == (0 + *fx_position)) || (i == (N3 + 1 + *fx_position)) ||
        (i == (N3 * 3 - 1) + *fx_position)) {
      led_set(i, *fx_index % 255, *fx_index % 255, 0);
    }
  }

  (*fx_position)++;
  if (*fx_position > N3) { *fx_position = 0; }

  led_show();
}
