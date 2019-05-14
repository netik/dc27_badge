#ifndef __USERCONFIG_H__
#define __USERCONFIG_H__
/* userconfig.h
 *
 * anything that has to do with long term storage of users
 * goes in here
 */

#define CONFIG_FLASH_ADDR 0xFF000
#define CONFIG_FLASH_SECTOR 255
#define CONFIG_SIGNATURE  0xdeadbeef  // duh
#define CONFIG_VERSION    1
#define CONFIG_NAME_MAXLEN 20

#define CONFIG_LEDSIGN_MAXLEN	124

/* WARNING: if you change the userconfig struct, update CONFIG_VERSION
 * above so that the config is automatically init'd with the new
 * else, the config struct will be misaligned and full of garbage.
 */

typedef struct ship_t {
  char name[16];
  uint8_t max_bullets;
  uint8_t velocity;
} ship_t;

typedef struct userconfig {
  uint32_t signature;
  uint32_t version;

  /* hw config */
  uint8_t led_pattern;

  uint8_t led_brightness;
  uint8_t sound_enabled;
  uint8_t airplane_mode;
  uint8_t rotate;

  /* touchpad calibration data */
  uint8_t touch_data_present;
  float touch_data[6];

  /* saved LED sign string */
  char led_string[CONFIG_LEDSIGN_MAXLEN];

  /* game */
  char name[CONFIG_NAME_MAXLEN+1];
  uint8_t level;
  uint16_t lastdeath; // last time you died
  uint8_t in_combat;
  uint32_t unlocks;
  int16_t hp;

  /* ship configuration -- which ships you possess */
  uint8_t ships[8]; // integers that point to the ships

  uint16_t xp;
  uint16_t build_points;

  uint16_t energy; // max energy will be calc'd from level.

  /* long-term counters */
  uint16_t won;
  uint16_t lost;

} userconfig;

typedef struct _peer {
  /* this struct is used for holding users in memory */
  /* it is also the format of our ping packet */
  /* unique network ID determined from use of lower 64 bits of SIM-UID */
  uint32_t netid;         /* 4 */
  uint8_t opcode;         /* 1 - BATTLE_OPCODE */

  /* Player Payload */
  char name[CONFIG_NAME_MAXLEN + 1];  /* 16 */
  uint8_t in_combat;      /* 1 */
  uint16_t unlocks;       /* 2 */
  /* Player stats */
  int16_t hp;             /* 2 */
  uint16_t xp;            /* 2 */
  uint8_t level;          /* 1 */
  uint8_t agl;            /* 1 */
  uint8_t might;          /* 1 */
  uint8_t luck;           /* 1 */
  uint16_t won;           /* 2 */
  uint16_t lost;          /* 2 */
  uint32_t rtc;           /* 4 - their clock if they have it */
  uint8_t ttl;            /* 1 - how recent this peer data is. 10 = 10 pings */

  /* Battle Payload */
  /* A bitwise map indicating the attack type */
  uint8_t attack_bitmap;  /* 1 */
  int16_t damage;         /* 2 */

} peer;

typedef struct _fightpkt {
  /* this is a shortened form of userdata for transmission during the fight */
  /* MAX 52 bytes, max is 66 (AES limitiation) */

  /* unique network ID determined from use of lower 64 bits of SIM-UID */
  uint8_t opcode;         /* 1 - BATTLE_OPCODE */

  /* clock, if any */
  unsigned long rtc;      /* 4 */
  /* Player Payload */
  char name[CONFIG_NAME_MAXLEN + 1];  /* 16 */
  uint8_t in_combat;      /* 1 */
  uint16_t unlocks;       /* 2 */
  int16_t hp;             /* 2 */
  uint16_t xp;            /* 2 */
  uint8_t level;          /* 1 */
  uint8_t agl;            /* 1 */
  uint8_t might;          /* 1 */
  uint8_t luck;           /* 1 */

  /* Player stats, used in ping packet */
  uint16_t won;           /* 2 */
  uint16_t lost;          /* 2 */

  /* Battle Payload */
  /* A bitwise map indicating the attack type */
  uint8_t attack_bitmap;  /* 1 */
  int16_t damage;         /* 2 */

} fightpkt;

/* prototypes */
extern void configStart(void);
extern void configSave(userconfig *);
extern userconfig *getConfig(void);
extern int16_t maxhp(uint16_t, uint8_t);

extern const char *rankname[];
extern unsigned long rtc;
extern unsigned long rtc_set_at;

#endif
