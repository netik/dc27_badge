#ifndef _LED_H_
#define _LED_H_

// address is board address without the read write bit, so A0 becomes 50
#define LED_I2C_ADDR 0x50
#define LED_COUNT 32
#define LED_COUNT_INTERNAL 32

typedef uint32_t color_rgb_t;

/* Prototypes */
extern void led_init(void);
extern void led_test(void);

#endif /* _LED_H_ */
