#ifndef _LED_H_
#define _LED_H_

// address is board address without the read write bit, so A0 becomes 50
#define LED_I2C_ADDR 0x50

// LEDs that animations can use
#define LED_COUNT 31

// actual leds on board. the eye is last.
#define LED_COUNT_INTERNAL 32

// our refresh rate is about 25Hz
#define EFFECTS_REDRAW_MS 20
#define LED_PATTERNS_FULL 27
#define LED_PATTERNS_LIMITED 27

// these are non-user selectable patterns
// which are used by some apps
#define LED_PATTERN_WORLDMAP 128
#define LED_PATTERN_BATTLE   129
#define LED_PATTERN_UNLOCK   130
#define LED_TEST             255

typedef uint32_t color_rgb_t;

/* Prototypes */
extern bool led_init(void);
extern void led_test(void);
extern void ledStart(void);
extern uint8_t ledStop(void);
extern void ledSetPattern(uint8_t pattern);
extern uint8_t led_brightness_get(void);
extern void led_brightness_set(uint8_t brightness);
extern void led_clear(void);

/* Animation structs */
#define LED_PATTERN_ROLLER_COASTER_COUNT 5
#define LED_PATTERN_BALLS_COUNT 5
typedef struct {
  float gravity;
  color_rgb_t colors[LED_PATTERN_BALLS_COUNT];
  int32_t start_height;
  float height[LED_PATTERN_BALLS_COUNT];
  float impact_velocity_start;
  float impact_velocity[LED_PATTERN_BALLS_COUNT];
  float time_since_last_bounce[LED_PATTERN_BALLS_COUNT];
  int32_t position[LED_PATTERN_BALLS_COUNT];
  uint32_t clock_since_last_bounce[LED_PATTERN_BALLS_COUNT];
  float dampening[LED_PATTERN_BALLS_COUNT];
} led_pattern_balls_t;

typedef struct {
  int8_t direction_red;
  int8_t direction_green;
  int8_t direction_blue;
  int8_t direction_yellow;
  int8_t index_red;
  int8_t index_green;
  int8_t index_blue;
  int8_t index_yellow;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t yellow;
  color_rgb_t rgb[LED_COUNT];
} led_triple_sweep_state_t;

#define LED_MAX_PARTICLES LED_COUNT

// used for add_glitter
typedef struct {
  bool visible;
  uint8_t led_num;
  bool growing;
  int16_t level;
  uint8_t accl;
} LED_PARTICLE;

#endif /* _LED_H_ */
