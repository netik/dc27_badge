#ifndef _LED_H_
#define _LED_H_

// address is board address without the read write bit, so A0 becomes 50
#define LED_I2C_ADDR 0x50
#define LED_COUNT 32
#define LED_COUNT_INTERNAL 32

// our refresh rate is about 25Hz
#define EFFECTS_REDRAW_MS 20
#define LED_PATTERNS_FULL 16
#define LED_PATTERNS_LIMITED 16

/* colors */
#define COL_RED    0
#define COL_ORANGE 1
#define COL_YELLOW 2
#define COL_GREEN  3
#define COL_BLUE   4
#define COL_INDIGO 5
#define COL_VIOLET 6

typedef uint32_t color_rgb_t;

/* Prototypes */
extern bool led_init(void);
extern void led_test(void);
extern void ledStart(void);
extern uint8_t ledStop(void);
extern void ledSetPattern(uint8_t pattern);
extern uint8_t led_brightness_get(void);
extern void led_brightness_set(uint8_t brightness);

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

#endif /* _LED_H_ */
