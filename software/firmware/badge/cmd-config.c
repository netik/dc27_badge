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
  userconfig *config = getConfig();
  (void)argv;
  (void)argc;

  printf ("name       %s\n", config->name);
  printf ("config ver %ld\n", config->version);
  printf ("signature  0x%08lx\n", config->signature);
  printf ("unlocks    0x%04lx\n", config->unlocks);
  printf ("sound      %d\n", config->sound_enabled);
  printf ("LEDs       Pattern %d:%s power %d/255 eye (%lx)\n",
    config->led_pattern,
    fxlist[config->led_pattern],
    config->led_brightness,
    config->eye_rgb_color
  );

  printf ("Game\n");
  printf ("----\n");
  printf ("level      %s (%d)\n",
           rankname[config->level],
           config->level);
  printf ("currship   %d (enabled: %x)\n",
           config->current_ship,
           config->ships_enabled);
  printf ("hp         %d\n",
           config->hp);
  printf ("xp         %d\n",
           config->xp);
  printf ("energy     %d\n",
          config->hp);
  printf ("bp         %d\n",
          config->xp);
  printf ("lastdeath  %d\n", config->lastdeath);
  printf ("incombat   %d\n", config->in_combat);
  printf ("lastpos    (%d, %d)\n", config->last_x, config->last_y);
  printf ("won/lost   %d/%d\n\n",
           config->won,
           config->lost);
}

static void cmd_config_set(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

#ifdef BLACK_BADGE
  unsigned int val;
#endif
  
  userconfig *config = getConfig();

  if (argc != 3) {
    printf ("Invalid set command.\nUsage: config set var value\n");
    return;
  }

  if (!strcasecmp(argv[1], "name")) {
    strncpy(config->name, argv[2], CONFIG_NAME_MAXLEN);
    printf ("Name set.\n");
    return;
  }

  if (!strcasecmp(argv[1], "sound")) {
    if (!strcmp(argv[2], "1")) {
      config->sound_enabled = 1;
    } else {
      config->sound_enabled = 0;
    }

    strncpy(config->name, argv[2], CONFIG_NAME_MAXLEN);
    printf ("Sound set to %d.\n", config->sound_enabled);
    return;
  }

#ifdef BLACK_BADGE
  val = strtoul (argv[2], NULL, 0);;

  if (!strcasecmp(argv[1], "level")) {
    config->level = val;
    printf ("level set to %d.\n", config->level);
    return;
  }

  if (!strcasecmp(argv[1], "won")) {
    config->won = val;
    printf ("won count set to %d.\n", config->won);
    return;
  }

  if (!strcasecmp(argv[1], "lost")) {
    config->lost = val;
    printf ("lost count set to %d.\n", config->lost);
    return;
  }

  if (!strcasecmp(argv[1], "ship_type")) {
    config->current_ship = val;
    printf ("Current Ship set to %d.\n", config->current_ship);
    return;
  }

  if (!strcasecmp(argv[1], "unlocks")) {
    config->unlocks = val;
    printf ("Unlocks set to %ld.\n", config->unlocks);
    return;
  }

  if (!strcasecmp(argv[1], "hp")) {
    config->hp = val;
    printf ("HP set to %d.\n", config->hp);
    return;
  }
  if (!strcasecmp(argv[1], "xp")) {
    config->xp = val;
    printf ("XP set to %d.\n", config->xp);
    return;
  }

  if (!strcasecmp(argv[1], "rtc")) {
    rtc = val;
    rtc_set_at = chVTGetSystemTime();
    printf ("rtc set to %ld.\n", rtc);
    return;
  }
#endif
  printf ("Invalid set command.\n");
}

static void cmd_config_save(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  userconfig *config = getConfig();
  configSave(config);

  printf ("Config saved.\n");
}


static void cmd_config(BaseSequentialStream *chp, int argc, char *argv[])
{

  if (argc == 0) {
    printf ("config commands:\n");
    printf ("   show           show config\n");
    printf ("   set nnn yyy    set variable nnn to yyy (vars: name, sound)\n");
    printf ("   led list       list animations available\n");
    printf ("   led dim n      LED Global Current Control (0-255) 255=brightest\n");
    printf ("   led run n      run pattern #n\n");
    printf ("   led stop       stop and blank LEDs\n");
    printf ("   led eye r g b  set eye color (rgb 0-127)\n");
    printf ("   save           save config to flash\n\n");

    printf ("warning: there is no mutex on config changes. save quickly or get conflicts.\n");
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
      printf ("config led ...what?\n");
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

  printf ("Unrecognized config command.\n");

}

static void cmd_config_led_eye(BaseSequentialStream *chp, int argc, char *argv[]) {

  int16_t r,g,b;
  userconfig *config = getConfig();

  if (argc != 5) {
    printf ("Not enough arguments.\n");
    return;
  }

  r = strtoul(argv[2], NULL, 0);
  g = strtoul(argv[3], NULL, 0);
  b = strtoul(argv[4], NULL, 0);

  if ((r < 0) || (r > 127) ||
      (g < 0) || (g > 127) ||
      (b < 0) || (b > 127) ) {
    printf ("Invaild value. Must be 0 to 127.\n");
    return;

  }

  printf ("Eye set.\n");

  config->eye_rgb_color = ( r << 16 ) + ( g << 8 ) + b;

}

/* LED configuration */
static void cmd_config_led_stop(BaseSequentialStream *chp) {
  ledStop();
  printf ("Off.\n");
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
    printf ("No pattern specified\n");
    return;
  }

  pattern = strtoul(argv[2], NULL, 0);
  if ((pattern < 1) || (pattern > max_led_patterns)) {
    printf ("Invaild pattern #!\n");
    return;
  }

  config->led_pattern = pattern-1;
  ledSetPattern(config->led_pattern);

  printf ("Pattern changed to %s.\n", fxlist[config->led_pattern]);

  if (config->led_pattern == 0) {
    ledStop();
  } else {
    if (ledsOff) {
      led_init ();
      ledStart();
    }
  }
}

static void cmd_config_led_dim(BaseSequentialStream *chp, int argc, char *argv[]) {

  int16_t level;
  userconfig *config = getConfig();

  if (argc != 3) {
    printf ("level?\n");
    return;
  }

  level = strtoul(argv[2], NULL, 0);

  if ((level < 0) || (level > 255)) {
    printf ("Invaild level. Must be 0 to 255.\n");
    return;
  }

  //  ledSetBrightness(level);
  printf ("Level now %d.\n", level);

  config->led_brightness = level;
  led_brightness_set(config->led_brightness);
}

static void cmd_config_led_list(BaseSequentialStream *chp) {
  uint8_t max_led_patterns;
  userconfig *config = getConfig();

  printf ("\nAvailable LED Patterns\n\n");

  if ( config->unlocks & UL_LEDS ) {
    max_led_patterns = LED_PATTERNS_FULL;
  } else {
    max_led_patterns = LED_PATTERNS_LIMITED;
  }


  for (int i=0; i < max_led_patterns; i++) {
    printf ("%2d) %-20s  ", i+1, fxlist[i]);
    if ((i+1) % 3 == 0) {
      printf ("\n");
    }
  }

  printf ("\n");

  if ( (config->unlocks & UL_LEDS) == 0) {
    printf ("\nMore can be unlocked!\n\n");
  }
}

orchard_command("config", cmd_config);
