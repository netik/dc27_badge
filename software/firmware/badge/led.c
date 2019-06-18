#include <stdint.h>
#include "hal.h"
#include "led.h"
#include "badge.h"
#include "is31fl_lld.h"
#include "math.h"
#include "userconfig.h"
#include "rand.h"

#include <string.h>

#define MILLIS() chVTGetSystemTime()
#define util_random(x,y) randRange(x,y)
#define M_PI 3.14159265358979323846264338327950288

/* if use_gamma is on, we'll do lookups againt the gamma table
 * which may make animations look smoother. This comes at a cost,
 * which is that over all the entire system looks darker. It's off
 * for now.
 */
#undef USE_GAMMA

/*
 * We use led_memory[] as a frame buffer. In reality it's
 * an overlay for the PWM control registers in the LED control
 * chip, which span from 0x00 to 0xBE on register page 2.
 * We actually need 0xBF bytes to hold all the values. We
 * also add one byte because we want to send the whole
 * buffer in a single I2C transfer, and for that we need
 * one leading byte to hold the register offset of the
 * first register. Once we write that, the chip will
 * autoincrement as it copies in new bytes.
 */
unsigned char led_memory[ISSI_ADDR_MAX + 2];

/* prototypes */
void led_set_all(uint8_t r, uint8_t g, uint8_t b);
void led_brightness_set(uint8_t brightness);
uint8_t led_brightness_get(void);
bool led_init(void);
void led_clear(void);
void led_show(void);
void led_reset(void);
void led_reinit(void);
void ledSetPattern(uint8_t);

void led_pattern_balls_init(led_pattern_balls_t *);
void led_pattern_balls(led_pattern_balls_t*);
void led_pattern_flame(void);
void led_pattern_kitt(int8_t*, int8_t*);
void led_pattern_triangle(int8_t*, int8_t*);
void led_pattern_double_sweep(uint8_t* p_index, float* p_hue, float* p_value);
void led_pattern_triple_sweep(led_triple_sweep_state_t* p_triple_sweep);
void led_pattern_rainbow(float* p_hue, uint8_t repeat);
void led_pattern_roller_coaster(uint8_t positions[], color_rgb_t color);
void led_pattern_running_lights(uint8_t red,
                                uint8_t green,
                                uint8_t blue,
                                uint8_t * p_position);
void led_pattern_kraftwerk(uint8_t* p_index, int8_t* p_position);
void led_pattern_police(uint8_t* p_index, int8_t* p_direction);
void led_pattern_dualspin(uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          uint8_t glitter, int16_t* p_index);
void led_pattern_bgsparkle(uint8_t r1, uint8_t g1, uint8_t b1,
                           int16_t *degree, bool bgpulse);
void led_pattern_meteor(uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t meteorSize, uint8_t meteorTrailDecay, bool meteorRandomDecay,
                        uint8_t *pos);
void led_pattern_unlock(uint8_t* p_index, int8_t* p_direction);
void led_pattern_unlock_success(uint8_t* p_index);
void led_pattern_unlock_failed(uint8_t *p_position);

/* control vars */
static thread_t * pThread;
static uint8_t ledExitRequest = 0;
uint8_t ledsOff = 1;
static bool leds_init_ok = false;
// the current function that updates the LEDs. Override with ledSetFunction();
static uint8_t led_current_func = 1;

// if we're random, our last function was this, which always starts at 2.
static uint8_t led_current_random = 2;
static uint32_t last_random_time = 0;

// for tracking "glitter"
static LED_PARTICLE particles[LED_MAX_PARTICLES];
static int8_t   led_used_particles = -1;
static bool     led_particle_fwd = true;

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

    /* D217-D224 */ /* D224 skipped, it's the eye */
    {0x30, 0x40, 0x50}, {0x32, 0x42, 0x52}, {0x34, 0x44, 0x54},
    {0x36, 0x46, 0x56}, {0x38, 0x48, 0x58}, {0x3A, 0x4A, 0x5A},
    {0x3C, 0x4C, 0x5C},

    /* D225-D232 with D224 as the last led */
    {0x00, 0x10, 0x20}, {0x02, 0x12, 0x22}, {0x04, 0x14, 0x24},
    {0x06, 0x16, 0x26}, {0x08, 0x18, 0x28}, {0x0A, 0x1A, 0x2A},
    {0x0C, 0x1C, 0x2C}, {0x0E, 0x1E, 0x2E}, {0x3E, 0x4E, 0x5E},

};

#ifdef USE_GAMMA
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
#endif

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
  "Random",
  "Flame",
  "Balls",
  "Larson Scanner",
  "Spot da Fed",
  "Double Sweep",
  "Triangle",
  "Triple Sweep",
  "Rainbow",
  "Red Blip",
  "Green Blip",
  "Purple Blip",
  "Yellow Blip",
  "Red Spin",
  "Green Spin",
  "Blue Spin",
  "Yellow Spin",
  "Kraftwerk",
  "Dual BG",
  "Dual Sky",
  "Dual PurOrg",
  "Meteor Red",
  "Meteor Blu",
  "Meteor Pur",
  "Red Sparkle",
  "Purple Pulse",
};

/* Brightness control */
static uint8_t led_brightness_level = 0xff;

uint8_t led_brightness_get() {
  return led_brightness_level;
}

void led_brightness_set(uint8_t brightness) {
  led_brightness_level = brightness;
}

// FIND INDEX OF ANTIPODAL (OPPOSITE) LED
#define LEDS_TOP_INDEX 15
static int antipodal_index(int i) {
  int iN = i + LEDS_TOP_INDEX;
  if (i >= LEDS_TOP_INDEX) {iN = ( i + LEDS_TOP_INDEX ) % LED_COUNT; }
  return iN;
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
  for (uint8_t i = 0; i < ISSI_ADDR_MAX + 2; i++) {
    led_memory[i] = 0;
  }

  led_reset ();

  leds_init_ok = drv_is31fl_init();

  return (leds_init_ok);
}

void led_clear() {
  led_set_all(0, 0, 0);
  led_show();
}

void ledSetPattern(uint8_t patt) {
  led_current_func = patt;

  if (leds_init_ok == false)
    return;

  if (ledExitRequest == 1) {
    // our thread is stopped.
    ledStart();
  }
}

void led_set(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (index < LED_COUNT_INTERNAL) {
#ifdef USE_GAMMA
    led_memory[led_address[index][0] + 1] = gamma_values[g / 2];
    led_memory[led_address[index][1] + 1] = gamma_values[r / 2];
    led_memory[led_address[index][2] + 1] = gamma_values[b / 2];
#else
    led_memory[led_address[index][0] + 1] = g;
    led_memory[led_address[index][1] + 1] = r;
    led_memory[led_address[index][2] + 1] = b;
#endif
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

void led_reset() {
  volatile uint32_t * p = (uint32_t *)(NRF_TWI1_BASE + 0xFFC);

  osalSysLock ();

  /* Disable the I2C port */

  i2c_lld_stop (&I2CD2);
  I2CD2.state = I2C_STOP;

  /*
   * Disabling the I2C controller doesn't always fully reset it.
   * To be really sure, you need to power cycle it. Apparently
   * the nRF52 peripherals have a power control register at offset
   * 0xFFC. I'm not sure if this is common to all devices, but
   * according to info on the forum, it's at least true for the
   * I2C block. We're using TWI1, which is at base address
   * 0x40004000. We cycle power by writing a 0 to the register
   * to turn it off and then writing a 1 to turn it back on again.
   */

  *p = 0;
  (void)p;
  *p = 1;

  /* Re-enable the I2C port */

  i2c_lld_start (&I2CD2);
  I2CD2.state = I2C_READY;

  osalSysUnlock ();

  return;
}

void
led_reinit (void)
{
  while (1) {

    led_reset ();

    /* Reload configuration */

    if (drv_is31fl_init () == true)
      break;
  }

  drv_is31fl_gcc_set (led_brightness_level);

  return;
}

void led_show() {
  msg_t r;

  if (leds_init_ok == false)
    return;

  i2cAcquireBus(&I2CD2);

  drv_is31fl_set_page(ISSI_PAGE_PWM);

  r = i2cMasterTransmitTimeout (&I2CD2, LED_I2C_ADDR,
    led_memory, sizeof(led_memory),
    NULL, 0, ISSI_TIMEOUT);

  if (r != MSG_OK)
    led_reinit();

  i2cReleaseBus(&I2CD2);
}

void led_test() {
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 255, 0, 0);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();

    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 255, 0);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();

    for (uint8_t i = 0; i < LED_COUNT; i++) {
      led_set(i, 0, 0, 255);
    }
    led_show();
    chThdSleepMilliseconds(1000);
    led_clear();
}

static void update_eye(void) {
  userconfig *config = getConfig();

  uint8_t r = (config->eye_rgb_color & 0xff0000) >> 16;
  uint8_t g = (config->eye_rgb_color & 0x00ff00) >> 8;
  uint8_t b = (config->eye_rgb_color & 0x0000ff);

  // this approximates the 'breathing' pattern as seen on macbooks
  led_set(31,(exp(sin(MILLIS()/4000.0 * M_PI)) - 0.36787944) * r/2,
	  (exp(sin(MILLIS()/4000.0 * M_PI)) - 0.36787944) * g/2,
	  (exp(sin(MILLIS()/4000.0 * M_PI)) - 0.36787944) * b/2);
}

/* Threads ------------------------------------------------------ */
static THD_WORKING_AREA(waBlingThread, 768);
static THD_FUNCTION(bling_thread, arg) {
  userconfig *config = getConfig();
  /* animation state vars */
  led_pattern_balls_t anim_balls;
  int8_t anim_index = 0;
  uint8_t anim_uindex = 0;
  int16_t anim_pos16 = 0;
  int8_t anim_position = 0;
  float anim_hue = 100;
  float anim_value = 0;
  uint8_t last_brightness;

  // Positions storage for roller coaster
  uint8_t positions[LED_PATTERN_ROLLER_COASTER_COUNT];
  for (uint8_t i = 0; i < LED_PATTERN_ROLLER_COASTER_COUNT; i++) {
    positions[i] = LED_COUNT - 1 - i;
  }

  //Triple sweep data
  led_triple_sweep_state_t anim_triple = {
    .direction_red = 1,
    .direction_green = 1,
    .direction_blue = -1,
    .direction_yellow = -1,
    .index_red = 7,
    .index_green = 23,
    .index_blue = 7,
    .index_yellow = 23,
    .red = 255,
    .green = 255,
    .blue = 255,
    .yellow = 255,
  };

  (void)arg;
  chRegSetThreadName("LED Bling");

  led_brightness_level = config->led_brightness;
  last_brightness = led_brightness_level;
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
      uint8_t my_current_func;

      if (led_current_func > 1 ) {
        my_current_func = led_current_func;
      } else {
        // we're in random mode. We will switch every 15 seconds.
        if( (chVTGetSystemTime() - last_random_time) > (15 * 1000)) {
          led_current_random = randRange(2, LED_PATTERNS_LIMITED);
          last_random_time = chVTGetSystemTime();
        }
        my_current_func = led_current_random;
      }

      switch (my_current_func) {
      case 2:
        led_pattern_flame();
        break;
      case 3:
        led_pattern_balls(&anim_balls);
        break;
      case 4:
        led_pattern_kitt(&anim_index, &anim_position);
        break;
      case 5:
        led_pattern_police(&anim_uindex, &anim_position);
        break;
      case 6:
        led_pattern_double_sweep(&anim_uindex, &anim_hue, &anim_value);
        break;
      case 7:
        led_pattern_triangle(&anim_index, &anim_position);
        break;
      case 8:
        led_pattern_triple_sweep(&anim_triple);
        break;
      case 9:
        led_pattern_rainbow(&anim_hue, 2);
        break;
      case 10:
        led_pattern_roller_coaster(positions, util_hsv_to_rgb(0, 1, 1));
        break;
      case 11:
        led_pattern_roller_coaster(positions, util_hsv_to_rgb(0.3, 1, 1));
        break;
      case 12:
        led_pattern_roller_coaster(positions, util_hsv_to_rgb(0.7, 1, 1));
        break;
      case 13:
        led_pattern_roller_coaster(positions, util_hsv_to_rgb(0.16, 1, 1));
        break;
      case 14:
        led_pattern_running_lights(255, 0, 0, &anim_uindex);
        break;
      case 15:
        led_pattern_running_lights(0, 255, 0, &anim_uindex);
        break;
      case 16:
        led_pattern_running_lights(0, 0, 255, &anim_uindex);
        break;
      case 17:
        led_pattern_running_lights(255, 255, 0, &anim_uindex);
        break;
      case 18:
        led_pattern_kraftwerk(&anim_uindex, &anim_position);
        break;
      case 19:
        led_pattern_dualspin(0, 0, 255,
                             0, 255, 0,
                             0, &anim_pos16);
        break;
      case 20:
        led_pattern_dualspin(0x14, 0x80, 0x33,
                             0xdd, 0xdd, 0xdd,
                             0, &anim_pos16);
        break;
      case 21:
        led_pattern_dualspin(0x81, 0x0D, 0x70,
                             0xF5, 0x82, 0x25,
                             0, &anim_pos16);
        break;
      case 22:
        led_pattern_meteor(0xff,0x00,0x00, 2, 100, true, &anim_uindex);
        break;
      case 23:
        led_pattern_meteor(0x01,0x0f,0xc2, 2, 100, true, &anim_uindex);
        break;
      case 24:
        led_pattern_meteor(0xff,0x00,0xff, 4, 80, true, &anim_uindex);
        break;
      case 25:
        led_pattern_bgsparkle(255,0,0,&anim_pos16,false);
        break;
      case 26:
        led_pattern_bgsparkle(255,0,255,&anim_pos16,true);
        break;
      case LED_PATTERN_WORLDMAP:
        led_clear();
        break;
      case LED_PATTERN_UNLOCK:
        led_pattern_unlock(&anim_uindex, &anim_index);
        break;
      case LED_PATTERN_UNLOCK_FAILED:
        led_pattern_unlock_failed(&anim_uindex);
        break;
      case LED_PATTERN_LEVELUP:
        led_pattern_unlock_success(&anim_uindex);
        break;
      case LED_TEST:
        led_test();
        break;
      }

      update_eye();
    }

    if ( ledExitRequest ) {
      // force one full cycle through an update on request to force LEDs off
      led_set_all(0,0,0);

      // make the eyeball very dim.
      led_set(31, 64, 0, 0);
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

  if (pThread == NULL)
    return (1);

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

void led_pattern_police(uint8_t* p_index, int8_t* p_direction) {

  //-POLICE LIGHTS (TWO COLOR SOLID)
  ++(*p_index);
  if (*p_index >= LED_COUNT) {*p_index = 0;}
  int fx_positionR = *p_index;
  int fx_positionB = antipodal_index(fx_positionR);
  led_set(fx_positionR, 255, 0, 0);
  led_set(fx_positionB, 0, 0, 255);

  led_show();
}

void led_pattern_triple_sweep(led_triple_sweep_state_t* p_triple_sweep) {
  // Ensure indices are within range
  while (p_triple_sweep->index_red < 0) {
    p_triple_sweep->index_red += LED_COUNT;
  }
  while (p_triple_sweep->index_green < 0) {
    p_triple_sweep->index_green += LED_COUNT;
  }
  while (p_triple_sweep->index_blue < 0) {
    p_triple_sweep->index_blue += LED_COUNT;
  }
  while (p_triple_sweep->index_yellow < 0) {
    p_triple_sweep->index_yellow += LED_COUNT;
  }
  while (p_triple_sweep->index_red >= LED_COUNT) {
    p_triple_sweep->index_red -= LED_COUNT;
  }
  while (p_triple_sweep->index_green >= LED_COUNT) {
    p_triple_sweep->index_green -= LED_COUNT;
  }
  while (p_triple_sweep->index_blue >= LED_COUNT) {
    p_triple_sweep->index_blue -= LED_COUNT;
  }
  while (p_triple_sweep->index_yellow >= LED_COUNT) {
    p_triple_sweep->index_yellow -= LED_COUNT;
  }

  // Apply sweep colors
  p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_red] = p_triple_sweep->red << 16;
  p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_green] =
    p_triple_sweep->green << 8;
  p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_blue] =
    p_triple_sweep->blue;
  p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_yellow] =
    (p_triple_sweep->yellow << 16) | (p_triple_sweep->yellow << 8);

  led_set_rgb((uint8_t)p_triple_sweep->index_red,
              p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_red]);
  led_set_rgb((uint8_t)p_triple_sweep->index_green,
              p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_green]);
  led_set_rgb((uint8_t)p_triple_sweep->index_blue,
              p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_blue]);
  led_set_rgb((uint8_t)p_triple_sweep->index_yellow,
              p_triple_sweep->rgb[(uint8_t)p_triple_sweep->index_yellow]);
  led_show();

  // Move indices, assume next run around range will be checked
  p_triple_sweep->index_red += p_triple_sweep->direction_red;
  p_triple_sweep->index_green += p_triple_sweep->direction_green;
  p_triple_sweep->index_blue += p_triple_sweep->direction_blue;
  p_triple_sweep->index_yellow += p_triple_sweep->direction_yellow;

  // Randomly change directions (sometimes) of one sweep color
  uint8_t r = util_random(0, 100);
  if (r == 0) {
    p_triple_sweep->direction_red *= -1;
    p_triple_sweep->red = util_random(180, 255);
  } else if (r == 1) {
   p_triple_sweep->direction_green *= -1;
    p_triple_sweep->green = util_random(180, 255);
  } else if (r == 2) {
    p_triple_sweep->direction_blue *= -1;
    p_triple_sweep->blue = util_random(180, 255);
  } else if (r == 3) {
    p_triple_sweep->direction_yellow *= -1;
    p_triple_sweep->yellow = util_random(180, 255);
  }
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


void led_pattern_double_sweep(uint8_t* p_index, float* p_hue, float* p_value) {
  color_rgb_t rgb = util_hsv_to_rgb(*p_hue, 1.0, *p_value);
  led_set_rgb(LED_COUNT / 2, rgb);
  led_set_rgb(LED_COUNT / 2 + *p_index, rgb);
  led_set_rgb(LED_COUNT / 2 - *p_index, rgb);
  led_show();

  (*p_index)++;
  if (*p_index > (LED_COUNT / 2)) {
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



/**
 * @brief Rainbow pattern because we can
 * @param p_hue : Pointer to hue for first LED
 * @param repeat : How many times to repeat the rainbow
 */
void led_pattern_rainbow(float* p_hue, uint8_t repeat) {
  float hue_step = 1.0 / ((float)LED_COUNT / (float)repeat);
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    color_rgb_t rgb = util_hsv_to_rgb(*p_hue + (hue_step * i), 1, 1);
    led_set_rgb(i, rgb);
  }
  led_show();

  *p_hue += 0.05;
  if (*p_hue > 1) {
    *p_hue -= 1;
  }
}

/**
 * @brief Run LEDs in a roller coaster pattern (ala Win10)
 * @param indices : Positions of each LED ball in the pattern
 */
void led_pattern_roller_coaster(uint8_t positions[], color_rgb_t color) {
  led_set_all(0, 0, 0);
  for (uint8_t i = 0; i < LED_PATTERN_ROLLER_COASTER_COUNT; i++) {
    float pos = (float)positions[i];
    float rad = (pos / (float)(LED_COUNT - 1)) * 2 * M_PI;
    float velocity =
        (-1.75) * cosf(rad) - 2.25;  // Ensure velocity is between -0.5 and -4.0
    pos += velocity;
    if (pos < 0) {
      pos += LED_COUNT;
    }
    positions[i] = (uint8_t)pos;

    led_set_rgb(positions[i], color);
  }

  led_show();
}

/**
 * @brief Do a running lights animation of a specific color, ported from
 * tweaking4all
 * @param red : Max Red value
 * @param green : Max Green value
 * @param blue : Max blue value
 */
void led_pattern_running_lights(uint8_t red,
                                uint8_t green,
                                uint8_t blue,
                                uint8_t* p_position) {
  for (int i = 0; i < LED_COUNT; i++) {
    led_set(i, ((sin(i + *p_position) * 127 + 128) / 255) * red,
            ((sin(i + *p_position) * 127 + 128) / 255) * green,
            ((sin(i + *p_position) * 127 + 128) / 255) * blue);
  }

  led_show();

  (*p_position)++;
  if (*p_position >= LED_COUNT * 2) {
    *p_position = 0;
  }
}

void led_pattern_kraftwerk(uint8_t *fx_index, int8_t *fx_position) {
  (*fx_index)++;
  if (*fx_index % 8 == 0) { (*fx_position)++; }

  if (*fx_position > 8) { *fx_position = 0; };

  led_clear();
  led_set(LED_COUNT-(*fx_position)-1, 255,0,0);
  led_set(*fx_position, 255,0,0);
  led_show();
}

void led_add_glitter(int n, int16_t *pos) {

  if (led_used_particles < 0) {
    led_particle_fwd = true;
  }

  // have we reached n or max yet?
  if ( (led_particle_fwd && led_used_particles < n) &&
       (*pos % 3 == 0) ) { // spawn every 200mS only
    // no, spawn a particle.
    led_used_particles++;
    particles[led_used_particles].led_num = util_random(0,LED_COUNT);
    particles[led_used_particles].visible = true;
  }

  // despawn
  if (led_particle_fwd == false &&
      led_used_particles > 0) {
    particles[led_used_particles].visible = false;
    led_used_particles--;

    if (led_used_particles <= 0) {
      led_particle_fwd = true;
    }
  }

  if (led_used_particles >= n) {
    led_particle_fwd = false;
  }

  // draw all particles in memory.
  for (int i = 0; i < led_used_particles; i++) {
    if (particles[i].visible)
      led_set(particles[i].led_num,
              particles[i].level,
              particles[i].level,
              particles[i].level);
  }

  // increment or decrement particle for next round.
  for (int i = 0; i < led_used_particles; i++) {
    if (particles[i].visible) {
      // sin here will make the bright lights pulse hard and randomly
      particles[i].level = 255;
    }
  }
};

void led_pattern_dualspin(uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          uint8_t glitter, int16_t *p_index) {
  // Ensure indices are within range
  if ((*p_index) > (LED_COUNT_INTERNAL - 1)) { *p_index = 0; }

  // we want to fill half of the display with wraparound
  led_set_all(r1,g1,b1);

  for (int i = (*p_index); i < ((LED_COUNT_INTERNAL / 2) - 1) + (*p_index); i++) {
    int pos = i;
    if (i > LED_COUNT-1) {
      pos = i - LED_COUNT;
    }
    led_set(pos, r2, g2, b2);
  }

  if (glitter > 0) {
    led_add_glitter(glitter, p_index);
  }

  led_show();
  (*p_index)++;
}

void led_pattern_bgsparkle(uint8_t r1, uint8_t g1, uint8_t b1,
                           int16_t *p_position, bool bgpulse) {

  if (bgpulse) {
    led_set_all(  (sin(*p_position * M_PI/180) * r1/2 + r1/2 ),
                  (sin(*p_position * M_PI/180) * g1/2 + g1/2 ),
                  (sin(*p_position * M_PI/180) * b1/2 + b1/2 )  );

  } else {
    led_set_all(r1, g1, b1);
  }

  led_add_glitter(6, p_position);

  (*p_position)++;

  led_show();

};

void fadeToBlack(int ledNo, uint8_t fadeValue) {
  uint8_t r, g, b;

  g = led_memory[led_address[ledNo][0] + 1];
  r = led_memory[led_address[ledNo][1] + 1];
  b = led_memory[led_address[ledNo][2] + 1];

  r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
  g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
  b=(b<=10)? 0 : (int) b-(b*fadeValue/256);

  led_set(ledNo, r, g, b);
}

void led_pattern_meteor(uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t meteorSize, uint8_t meteorTrailDecay, bool meteorRandomDecay,
                        uint8_t *pos) {

    if ((*pos) > LED_COUNT+LED_COUNT) {
      *pos = 0;
    }

    // fade brightness all LEDs one step
    for(int j=0; j < LED_COUNT; j++) {
      if( (!meteorRandomDecay) || (util_random(0,10) > 5) ) {
        fadeToBlack(j, meteorTrailDecay );
      }
    }

    // draw meteor
    for (int j = 0; j < meteorSize; j++) {
      if( ( (*pos)-j < LED_COUNT) && ((*pos)-j>=0) ) {
        led_set((*pos)-j, red, green, blue);
      }
    }

    led_show();
    (*pos)++;
}

void led_pattern_unlock(uint8_t *p_position, int8_t *direction) {
  userconfig *config = getConfig();

  led_set_all(0, 0, 0);
  // pulse the first 16 LEDs to show unlock state.
  for (int i = 0; i < 16; i++) {
    // pulse the ones that we have enabled.
    if (config->unlocks & ( 1 << i )) {
      led_set(i,0,(sin(.25 * *p_position) * 64) + 128,0);
    } else {
      // blue
      led_set(i, 0, 0, 30);
    }
  }

  *p_position = *p_position + 1;

  if (*p_position > 254) {
    *p_position = 0;
  }

  led_show();
}

void led_pattern_unlock_failed(uint8_t *p_position) {
  // abuse lights @ 40mS
  if (*p_position % 2 == 0) {
    led_set_all(220, 255, 255); // white-ish-blue
  } else {
    led_set_all(0, 0, 0);
  };

  ++(*p_position);

  if (*p_position > 1000 / EFFECTS_REDRAW_MS) {
    *p_position = 0;
  }

  led_show();

}

void led_pattern_unlock_success(uint8_t *p_position) {
  // abuse lights @ 40mS, green
  if (*p_position % 2 == 0) {
    led_set_all(0, 255, 0); // green
  } else {
    led_set_all(0, 0, 0);
  };

  ++(*p_position);

  if (*p_position > 1000 / EFFECTS_REDRAW_MS) {
    *p_position = 0;
  }

  led_show();

}
