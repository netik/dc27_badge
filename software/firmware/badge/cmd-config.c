/* cmd-config - config dump tool, mainly for debugging */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ch.h"
#include "hal.h"

#include "led.h"
#include "shell.h"
#include "datetime.h"

#include "orchard-app.h"
#include "userconfig.h"

#include "badge.h"
#include "unlocks.h"

/* This is defined in led.c, we need access to it for this config */
extern uint8_t ledsOff;
extern char *fxlist[];

/* prototypes */
static void cmd_config_show(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config_set(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config_save(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config_led_stop(BaseSequentialStream *chp);
static void cmd_config_led_run(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config_led_dim(BaseSequentialStream *chp, int argc, char *argv[]);
static void cmd_config_led_list(BaseSequentialStream *chp);
static void cmd_config_led_eye(BaseSequentialStream *chp, int argc, char *argv[]);

/* end prototypes */

static void cmd_config_show(BaseSequentialStream *chp, int argc, char *argv[])
{
#ifdef BLACK_BADGE
  tmElements_t dt;
  unsigned long curtime;
  unsigned long delta;
#endif
  userconfig *config = getConfig();
  (void)argv;
  (void)argc;

#ifdef BLACK_BADGE
  if (rtc != 0) {
    delta = (chVTGetSystemTime() - rtc_set_at);
    curtime = rtc + delta;
    breakTime(curtime, &dt);

    chprintf(chp, "%02d/%02d/%02d %02d:%02d:%02d\r\n", 1970+dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second);
  }
#endif

  chprintf(chp, "name       %s\r\n", config->name);
  chprintf(chp, "config ver %d\r\n", config->version);
  chprintf(chp, "signature  0x%08x\r\n", config->signature);
  chprintf(chp, "unlocks    0x%04x\r\n", config->unlocks);
  chprintf(chp, "sound      %d\r\n", config->sound_enabled);
  chprintf(chp, "LEDs       Pattern %d:%s power %d/255 eye (%x)\r\n",
    config->led_pattern,
    fxlist[config->led_pattern],
    config->led_brightness,
    config->eye_rgb_color
  );

  chprintf(chp, "Game\r\n");
  chprintf(chp, "----\r\n");
  chprintf(chp, "level      %s (%d)\r\n",
           rankname[config->level-1],
           config->level);
  chprintf(chp, "currship   %d (enabled: %x)\r\n",
           config->current_ship,
           config->ships_enabled);
  chprintf(chp, "hp         %d\r\n",
           config->hp);
  chprintf(chp, "xp         %d\r\n",
           config->xp);
  chprintf(chp, "energy     %d\r\n",
          config->hp);
  chprintf(chp, "bp         %d\r\n",
          config->xp);
  chprintf(chp, "lastdeath  %d\r\n", config->lastdeath);
  chprintf(chp, "incombat   %d\r\n", config->in_combat);
  chprintf(chp, "lastpos    (%d, %d)\r\n", config->last_x, config->last_y);
  chprintf(chp, "won/lost   %d/%d\r\n\r\n",
           config->won,
           config->lost);
}

static void cmd_config_set(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;
#ifdef BLACK_BADGE
  uint32_t val;
#endif

  userconfig *config = getConfig();

  if (argc != 3) {
    chprintf(chp, "Invalid set command.\r\nUsage: config set var value\r\n");
    return;
  }

  if (!strcasecmp(argv[1], "pingdump")) {
    if (!strcmp(argv[2], "1")) {
      config->unlocks |= UL_PINGDUMP;
    } else {
      config->unlocks &= ~(UL_PINGDUMP);
    }
    chprintf(chp, "Pingdump set.\r\n");
    return;
  }

  if (!strcasecmp(argv[1], "name")) {
    strncpy(config->name, argv[2], CONFIG_NAME_MAXLEN);
    chprintf(chp, "Name set.\r\n");
    return;
  }

  if (!strcasecmp(argv[1], "sound")) {
    if (!strcmp(argv[2], "1")) {
      config->sound_enabled = 1;
    } else {
      config->sound_enabled = 0;
    }

    strncpy(config->name, argv[2], CONFIG_NAME_MAXLEN);
    chprintf(chp, "Sound set to %d.\r\n", config->sound_enabled);
    return;
  }

#ifdef BLACK_BADGE
  val = strtoul (argv[2], NULL, 0);;

  if (!strcasecmp(argv[1], "level")) {
    config->level = val;
    chprintf(chp, "level set to %d.\r\n", config->level);
    return;
  }

  if (!strcasecmp(argv[1], "ships")) {
    config->ships_enabled = val;
    chprintf(chp, "Enabled ships set to %d.\r\n", config->ships_enabled);
    return;
  }

  if (!strcasecmp(argv[1], "unlocks")) {
    config->unlocks = val;
    chprintf(chp, "Unlocks set to %d.\r\n", config->unlocks);
    return;
  }

  if (!strcasecmp(argv[1], "hp")) {
    config->hp = val;
    chprintf(chp, "HP set to %d.\r\n", config->hp);
    return;
  }
  if (!strcasecmp(argv[1], "xp")) {
    config->xp = val;
    chprintf(chp, "XP set to %d.\r\n", config->xp);
    return;
  }

  if (!strcasecmp(argv[1], "rtc")) {
    rtc = val;
    rtc_set_at = chVTGetSystemTime();
    chprintf(chp, "rtc set to %d.\r\n", rtc);
    return;
  }
#endif
  chprintf(chp, "Invalid set command.\r\n");
}

static void cmd_config_save(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  userconfig *config = getConfig();
  configSave(config);

  chprintf(chp, "Config saved.\r\n");
}


static void cmd_config(BaseSequentialStream *chp, int argc, char *argv[])
{

  if (argc == 0) {
    chprintf(chp, "config commands:\r\n");
    chprintf(chp, "   show           show config\r\n");
    chprintf(chp, "   set nnn yyy    set variable nnn to yyy (vars: name, sound)\r\n");
    chprintf(chp, "   led list       list animations available\r\n");
    chprintf(chp, "   led dim n      LED Global Current Control (0-255) 255=brightest\r\n");
    chprintf(chp, "   led run n      run pattern #n\r\n");
    chprintf(chp, "   led stop       stop and blank LEDs\r\n");
    chprintf(chp, "   led eye r g b  set eye color (rgb 0-127)\r\n");
    chprintf(chp, "   save           save config to flash\r\n\r\n");

    chprintf(chp, "warning: there is no mutex on config changes. save quickly or get conflicts.\r\n");
    return;
  }

  if (!strcasecmp(argv[0], "show")) {
    cmd_config_show(chp, argc, argv);
    return;
  }

  if (!strcasecmp(argv[0], "set")) {
    cmd_config_set(chp, argc, argv);
    return;
  }

  if (!strcasecmp(argv[0], "save")) {
    cmd_config_save(chp, argc, argv);
    return;
  }

  if (!strcasecmp(argv[0], "led")) {
    if (argc < 2) {
      chprintf(chp, "config led ...what?\r\n");
      return;
    }

    if (!strcasecmp(argv[1], "dim")) {
      cmd_config_led_dim(chp, argc, argv);
      return;
    }

    if (!strcasecmp(argv[1], "eye")) {
      cmd_config_led_eye(chp, argc, argv);
      return;
    }

    if (!strcasecmp(argv[1], "list")) {
      cmd_config_led_list(chp);
      return;
    }

    if (!strcasecmp(argv[1], "run")) {
      cmd_config_led_run(chp, argc, argv);
      return;
    }

    if (!strcasecmp(argv[1], "stop")) {
      cmd_config_led_stop(chp);
      return;
    }
  }

  chprintf(chp, "Unrecognized config command.\r\n");

}

static void cmd_config_led_eye(BaseSequentialStream *chp, int argc, char *argv[]) {

  int16_t r,g,b;
  userconfig *config = getConfig();

  if (argc != 5) {
    chprintf(chp, "Not enough arguments.\r\n");
    return;
  }

  r = strtoul(argv[2], NULL, 0);
  g = strtoul(argv[3], NULL, 0);
  b = strtoul(argv[4], NULL, 0);

  if ((r < 0) || (r > 127) ||
      (g < 0) || (g > 127) ||
      (b < 0) || (b > 127) ) {
    chprintf(chp, "Invaild value. Must be 0 to 127.\r\n");
    return;

  }

  chprintf(chp, "Eye set.\r\n");

  config->eye_rgb_color = ( r << 16 ) + ( g << 8 ) + b;

}

/* LED configuration */
static void cmd_config_led_stop(BaseSequentialStream *chp) {
  ledStop();
  chprintf(chp, "Off.\r\n");
}

static void cmd_config_led_run(BaseSequentialStream *chp, int argc, char *argv[]) {

  uint8_t pattern;
  userconfig *config = getConfig();
  uint8_t max_led_patterns;

  if ( config->unlocks & UL_LEDS ) {
    max_led_patterns = LED_PATTERNS_FULL;
  } else {
    max_led_patterns = LED_PATTERNS_LIMITED;
  }

  if (argc != 3) {
    chprintf(chp, "No pattern specified\r\n");
    return;
  }

  pattern = strtoul(argv[2], NULL, 0);
  if ((pattern < 1) || (pattern > max_led_patterns)) {
    chprintf(chp, "Invaild pattern #!\r\n");
    return;
  }

  config->led_pattern = pattern-1;
  ledSetPattern(config->led_pattern);

  chprintf(chp, "Pattern changed to %s.\r\n", fxlist[config->led_pattern]);

  if (config->led_pattern == 0) {
    ledStop();
  } else {
    if (ledsOff) {
      ledStart();
    }
  }
}

static void cmd_config_led_dim(BaseSequentialStream *chp, int argc, char *argv[]) {

  int16_t level;
  userconfig *config = getConfig();

  if (argc != 3) {
    chprintf(chp, "level?\r\n");
    return;
  }

  level = strtoul(argv[2], NULL, 0);

  if ((level < 0) || (level > 255)) {
    chprintf(chp, "Invaild level. Must be 0 to 255.\r\n");
    return;
  }

  //  ledSetBrightness(level);
  chprintf(chp, "Level now %d.\r\n", level);

  config->led_brightness = level;
  led_brightness_set(config->led_brightness);
}

static void cmd_config_led_list(BaseSequentialStream *chp) {
  uint8_t max_led_patterns;
  userconfig *config = getConfig();

  chprintf(chp, "\r\nAvailable LED Patterns\r\n\r\n");

  if ( config->unlocks & UL_LEDS ) {
    max_led_patterns = LED_PATTERNS_FULL;
  } else {
    max_led_patterns = LED_PATTERNS_LIMITED;
  }


  for (int i=0; i < max_led_patterns; i++) {
    chprintf(chp, "%2d) %-20s  ", i+1, fxlist[i]);
    if ((i+1) % 3 == 0) {
      chprintf(chp, "\r\n");
    }
  }

  chprintf(chp, "\r\n");

  if ( (config->unlocks & UL_LEDS) == 0) {
    chprintf(chp, "\r\nMore can be unlocked!\r\n\r\n");
  }
}

orchard_command("config", cmd_config);
