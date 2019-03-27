#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "joypad_lld.h"
#include "nrf52flash_lld.h"

#include "unlocks.h"
#include "userconfig.h"
#include "badge.h"

#include "rand.h"

#include <string.h>

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

int16_t maxhp(player_type ctype, uint16_t unlocks, uint8_t level) {
  // return maxHP given some unlock data and level
  uint16_t hp;

  hp = (70+(20*(level-1)));

  if (unlocks & UL_PLUSHP) {
    hp = hp * 1.10;
  }

  if (ctype == p_bender) {
    hp = hp * 1.10;
  }

  if (ctype == p_caesar) {
    hp = hp * 3;
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
    return;
  }

  flashWaitErase((void *)&FLASHD2);
  /*                  base      offset   size                pointer */
  ret = flashProgram (&FLASHD2, 0xFF000, sizeof(userconfig), (uint8_t *)newConfig);

  if (ret != FLASH_NO_ERROR) {
    printf("ERROR (%d): Unable to save config to flash.\r\n", ret);
  }

  osalMutexUnlock(&config_mutex);
}

static void init_config(userconfig *config) {
  /* please keep these in the same order as userconfig.h
   * so it's easier to maintain.  */
  config->signature = CONFIG_SIGNATURE;
  config->version = CONFIG_VERSION;
  config->tempcal = 99;
  config->unlocks = 0;

  /* this is a very, very naive approach to uniqueness, but it might work. */
  /* an alternate approach would be to store all 80 bits, but then our radio */
  /* packets would be huge. */
  config->netid = 0; // we will use the bluetooth address instead of this var.
  config->unlocks = 0;
  config->led_pattern = 1;
  config->led_shift = 4;
  config->sound_enabled = 1;
  config->airplane_mode = 0;
  config->rotate = 0;

  config->current_type = p_notset; // what your current class is (caesear, bender, etc, specials...)
  config->p_type = p_notset;       // your permanent class, cannot be changed.
  config->touch_data_present = 0;

  memset(config->led_string, 0, CONFIG_LEDSIGN_MAXLEN);
  memset(config->name, 0, CONFIG_NAME_MAXLEN);

  config->won = 0;
  config->lost = 0;

  config->lastdeath = 0; // last death time to calc healing
  config->in_combat = 1; // we are incombat until type is set.

  /* stats, dunno if we will use */
  config->xp = 0;
  config->level = 1;

  config->agl = 1;
  config->luck = 20;
  config->might = 1;

  config->won = 0;
  config->lost = 0;

  /* randomly pick a new color */
  config->led_r = randByte();
  config->led_g = randByte();
  config->led_b = randByte();

  config->hp = maxhp(p_notset, 0, config->level);

}

void configStart(void) {
  userconfig config;
  uint8_t wipeconfig = false;
  osalMutexObjectInit(&config_mutex);

  flashRead(&FLASHD2, CONFIG_FLASH_ADDR,
    sizeof(userconfig), (uint8_t *)&config);

  /* if the user is holding down UP and DOWN, then we will wipe the configuration */
#ifdef ENABLE_JOYPAD
  if ((palReadPad (BUTTON_UP_PORT, BUTTON_UP_PIN) == 0) &&
      (palReadPad (BUTTON_DOWN_PORT, BUTTON_DOWN_PIN) == 0)) {
#else
  /*
   * On the Freescale/NXP reference boards, we don't have a joypad,
   * and the UP and DOWN signals are stuck at ground, which makes
   * us think the user always wants to reset the config. If the joypad
   * isn't enabled, just use the enter button to request factory reset.
   */
  if (palReadPad (BUTTON_ENTER_PORT, BUTTON_ENTER_PIN) == 0) {
#endif
    printf("FACTORY RESET requested\r\n");

    /* play a tone to tell them we're resetting */
#ifdef notyet
    playConfigReset();
#endif
    chThdSleepMilliseconds (200);
    wipeconfig = true;
  }

  if ( (config.signature != CONFIG_SIGNATURE) || (wipeconfig)) {
    printf("Config not found, Initializing!\r\n");
    init_config(&config_cache);
    memcpy(&config, &config_cache, sizeof(userconfig));
    configSave(&config_cache);
  } else if ( config.version != CONFIG_VERSION ) {
    printf("Config found, but wrong version.\r\n");
    init_config(&config_cache);
    memcpy(&config, &config_cache, sizeof(userconfig));
    configSave(&config_cache);
  } else {
    printf("Config OK!\r\n");
    memcpy(&config_cache, &config, sizeof(userconfig));

    if (config_cache.in_combat != 0) {
      if (config_cache.p_type > 0) {
        // we will only release this when type is set
        printf("You were stuck in combat. Fixed.\r\n");
        config_cache.in_combat = 0;
        configSave(&config_cache);
      }
    }

    if (config.p_type != config.current_type) {
      // reset class on fight
      printf("Class reset to %d.\r\n", config_cache.p_type);
      config_cache.current_type = config_cache.p_type;
      config_cache.hp = maxhp(config_cache.p_type, config_cache.unlocks, config_cache.level);
      configSave(&config_cache);
    }
  }

  return;
}

struct userconfig *getConfig(void) {
  // returns volitale config
  return &config_cache;
}
