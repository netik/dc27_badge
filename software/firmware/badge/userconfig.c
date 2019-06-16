#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "joypad_lld.h"
#include "nrf52flash_lld.h"
#include "i2s_lld.h"

#include "unlocks.h"
#include "userconfig.h"
#include "badge.h"

#include "rand.h"
#include "vector.h"

#include <string.h>

const char *rankname[] = {
  "Ensign",      // 0
  "Lt. Junior",  // 1
  "Lieutenant",  // 2
  "Lt. Commdr",  // 3
  "Commander",   // 4
  "Captain",     // 5
  "Rear Adm.",   // 6
  "Rear Adm.2",  // 7
  "Vice Adm.",   // 8
  "Admiral",     // 9
  "Flt Admiral", // 10
  "WOPR"         // 11
};

// so that we don't have to search for starting places, here's a list
// of 20 safe starting places in the world map.
VECTOR safe_start[MAX_SAFE_START] = {
  { 20, 225 },
  { 14, 158 },
  { 14, 127 },
  { 14,105 },
  { 26, 92 },
  { 56, 109 },
  { 78, 92 },
  { 64, 49 },
  { 88, 50 },
  { 144, 128 },
  { 173, 162 },
  { 199, 199 },
  { 150, 228 },
  { 208, 223 },
  { 231, 199 },
  { 259, 140 },
  { 166, 70 },
  { 173, 26 },
  { 220, 60 },
  { 252, 62 },
  { 286, 107 }
};

/* We implement a naive real-time clock protocol like NTP but
 * much worse. The global value rtc contains the current real time
 * clock as a time_t value measured by # seconds since Jan 1, 1970.
 *
 * If our RTC value is set to 0, then we will set our RTC from any
 * arriving user packet that is non zero. we will also spread the
 * "virus" of the RTC to other badges. Thanks to Queercon 11's badge
 * badge for this idea.
 *
 */
unsigned long rtc = 0;
unsigned long rtc_set_at = 0;

static userconfig config_cache;

mutex_t config_mutex;

int16_t maxhp(uint16_t unlocks, uint8_t level) {
  // return maxHP given some unlock data and level
  uint16_t hp;

  hp = (70+(20*(level-1)));

  if (unlocks & UL_PLUSHP) {
    hp = hp * 1.10;
  }

  return hp;
}

void configSave(userconfig *newConfig) {
  int8_t ret;
  osalMutexLock(&config_mutex);
  /*
   * Note: we're compiled to use the SoftDevice for flash access,
   * which means we can only actually perform erase and program
   * operations on the internal flash after the SoftDevice has
   * been enabled.
   */

  if (flashStartEraseSector (&FLASHD2, 255) != FLASH_NO_ERROR) {
    printf ("configSave: Flash Erase Failed!\r\n");
    osalMutexUnlock(&config_mutex);
    return;
  }

  flashWaitErase((void *)&FLASHD2);
  /*                  base      offset   size                pointer */
  ret = flashProgram (&FLASHD2, 0xFF000, sizeof(userconfig), (uint8_t *)newConfig);

  if (ret != FLASH_NO_ERROR) {
    printf("ERROR (%d): Unable to save config to flash.\n", ret);
  }

  osalMutexUnlock(&config_mutex);
}

static void init_config(userconfig *config) {
  /* please keep these in the same order as userconfig.h
   * so it's easier to maintain.  */

  int position = randRange(0, MAX_SAFE_START);

  config->signature = CONFIG_SIGNATURE;
  config->version = CONFIG_VERSION;

  config->led_pattern = 1;
  config->led_brightness = 0xf0;
  config->sound_enabled = 1;
  config->airplane_mode = 0;
  config->rotate = 0;

  config->touch_data_present = 0;
  // touch_data will be ignored for now.
  memset(config->led_string, 0, CONFIG_LEDSIGN_MAXLEN);

  /* game */
  memset(config->name, 0, CONFIG_NAME_MAXLEN);
  config->level = 1;
  config->lastdeath = 0; // last death time to calc healing
  config->in_combat = 1; // we are incombat until name is set.
#ifdef BLACK_BADGE
  config->unlocks = UL_BLACKBADGE;
#else
  config->unlocks = 0;
#endif

  /* ship config */
  config->current_ship = 1;
  config->ships_enabled = 1; // just the first ship enabled
  config->energy = 100;
  config->build_points = 40; // you can buy a few things
  config->last_x = safe_start[position].x;
  config->last_y = safe_start[position].y;

  config->xp = 0;
  config->hp = 0;
  config->won = 0;
  config->lost= 0;
}


#define ENABLE_JOYPAD

void configStart(void) {
  userconfig *config = (userconfig *)CONFIG_FLASH_ADDR;
  uint8_t wipeconfig = false;
  osalMutexObjectInit(&config_mutex);

  /* if the user is holding down BOTH SELECTS, then we will wipe the configuration */
#ifdef ENABLE_JOYPAD
  if ((palReadPad (BUTTON_A_ENTER_PORT, BUTTON_A_ENTER_PIN) == 0) &&
      (palReadPad (BUTTON_B_ENTER_PORT, BUTTON_B_ENTER_PIN) == 0)) {
#else
  /*
   * On the Freescale/NXP reference boards, we don't have a joypad,
   * and the UP and DOWN signals are stuck at ground, which makes
   * us think the user always wants to reset the config. If the joypad
   * isn't enabled, just use the enter button to request factory reset.
   */
  if (palReadPad (BUTTON_A_ENTER_PORT, BUTTON_A_ENTER_PIN) == 0) {
#endif
    printf("FACTORY RESET requested\n");

    /* play a tone to tell them we're resetting */
    i2sPlay("sound/ping.snd");
    i2sWait ();

    wipeconfig = true;
  }

  if ( (config->signature != CONFIG_SIGNATURE) || (wipeconfig)) {
    printf("Config not found, Initializing!\n");
    init_config(&config_cache);
    memcpy(config, &config_cache, sizeof(userconfig));
    configSave(&config_cache);
  } else if ( config->version != CONFIG_VERSION ) {
    printf("Config found, but wrong version.\n");
    init_config(&config_cache);
    memcpy(config, &config_cache, sizeof(userconfig));
    configSave(&config_cache);
  } else {
    printf("Config OK!\n");
    memcpy(&config_cache, config, sizeof(userconfig));

    if (config_cache.in_combat != 0) {
        printf("You were stuck in combat. Fixed.\n");
        config_cache.in_combat = 0;
        configSave(&config_cache);
    }
  }

  return;
}

struct userconfig *getConfig(void) {
  userconfig *config = (userconfig *)CONFIG_FLASH_ADDR;

  /*
   * returns volatile config, unless we're called very early
   * during startup, in which case we try to return a pointer
   * to the actual flash memory. This is to avoid a "chicken-and-the-egg"
   * problem where the BLE startup code needs to read the board config.
   * (We need the SoftDevice to do flash accesses so we have to initialize
   * BLE first and userconfig second, but that means config_cache will
   * never be valid until after we initialize the radio.)
   */

  if (config_cache.signature != CONFIG_SIGNATURE &&
      config->signature == CONFIG_SIGNATURE)
    return (config);

  return &config_cache;
}
