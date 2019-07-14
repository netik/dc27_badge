#ifndef __USERCONFIG_H__
#define __USERCONFIG_H__

#include "vector.h"

/* userconfig.h
 *
 * anything that has to do with long term storage of users
 * goes in here
 */

#define CONFIG_FLASH_ADDR 0xFF000
#define CONFIG_FLASH_SECTOR 255
#define CONFIG_SIGNATURE  0xdeadbeef  // duh
#define CONFIG_END_SIGNATURE  0xdeadfa11
#define CONFIG_VERSION    6
#define CONFIG_NAME_MAXLEN 20

#define CONFIG_LEDSIGN_MAXLEN	124

/* WARNING: if you change the userconfig struct, update CONFIG_VERSION
 * above so that the config is automatically init'd with the new
 * else, the config struct will be misaligned and full of garbage.
 */

typedef struct userconfig {
  uint32_t signature;
  uint32_t version;

  /* hw config */
  uint8_t led_pattern;
  uint8_t led_brightness;
  uint8_t sound_enabled;
  uint8_t airplane_mode;
  uint8_t rotate;

  uint32_t eye_rgb_color;

  /* touchpad calibration data */
  uint8_t touch_data_present;
  float touch_data[6];

  /* saved LED sign string */
  char led_string[CONFIG_LEDSIGN_MAXLEN];

  /* NCS puzzles */
  uint8_t puz_enabled;

  /* game */
  char name[CONFIG_NAME_MAXLEN+1];
  uint8_t level;
  uint16_t lastdeath; // last time you died
  uint8_t in_combat;
  uint32_t unlocks;

  /* ship configuration -- which ships you possess */
  uint8_t current_ship;  // which of the ships you are currently using
  uint8_t ships_enabled; // boolean flags - ship type
  uint16_t energy;       // how much energy this ship has.
  uint16_t build_points;
  uint16_t last_x;
  uint16_t last_y;

  // long-term stats
  uint16_t xp;
  int16_t hp;
  uint16_t won;
  uint16_t lost;

  /* end of config */
  uint32_t end_signature;
} userconfig;

/* prototypes */
extern void configStart(void);
extern void configSave(userconfig *);
extern userconfig *getConfig(void);
extern int16_t maxhp(uint16_t, uint8_t);
extern uint16_t xp_for_level(uint8_t level);

extern const char *rankname[];
extern unsigned long rtc;
extern unsigned long rtc_set_at;

extern VECTOR safe_start[];
#endif
