#ifndef _LED_H_
#define _LED_H_

// address is board address without the read write bit, so A0 becomes 50
#define LED_I2C_ADDR 0x50
#define LED_COUNT 32
#define LED_COUNT_INTERNAL 32
#define EFFECTS_REDRAW_MS 35

typedef uint32_t color_rgb_t;

/* Prototypes */
extern bool led_init(void);
extern void led_test(void);
extern void ledStart(void);

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
