/*
 * app-unlock.c
 *
 * the unlock screen that allows people to unlock features in
 * the badge
 *
 */

#include <string.h>
#include "orchard-app.h"
#include "orchard-ui.h"
#include "userconfig.h"
#include "ides_gfx.h"
#include "badge.h"
#include "images.h"
#include "led.h"
#include "unlocks.h"
#include "i2s_lld.h"

#define ALERT_DELAY 3000

extern systime_t char_reset_at;

// handles
typedef struct _UnlockHandles {
  // GListener
  GListener glUnlockListener;

  // GHandles
  GHandle ghNum1;
  GHandle ghNum2;
  GHandle ghNum3;
  GHandle ghNum4;
  GHandle ghNum5;

  GHandle ghBack;
  GHandle ghUnlock;

  // Fonts
  font_t font_jupiterpro_36;
  font_t font_manteka_20;

} UnlockHandles;

/* codes, packed as bits. Note only last five nibbles (4 bits) are used, MSB of
   1st byte is always zero */
#define MAX_ULCODES 11
static uint8_t unlock_codes[MAX_ULCODES][3] = { { 0x01, 0xde, 0xf1 }, // 0 +10% DEF
                                                { 0x0d, 0xef, 0xad }, // 1 +10% HP
                                                { 0x0a, 0x7a, 0xa7 }, // 2 +1 MIGHT
                                                { 0x07, 0x70, 0x07 }, // 3 +20% LUCK
                                                { 0x0a, 0xed, 0x17 }, // 4 FASTER HEAL
                                                { 0x00, 0x1a, 0xc0 }, // 5 MOAR LEDs
                                                { 0x0b, 0xae, 0xac }, // 6 CAESAR
                                                { 0x0d, 0xe0, 0x1a }, // 7 SENATOR
                                                { 0x0b, 0xbb, 0xbb }, // 8 BENDER
                                                { 0x06, 0x07, 0x42 }, // 9 GOD MODE (can issue grants)
                                                { 0x0a, 0x1a, 0x1a }  // 10 PINGDUMP
};

static uint8_t unlock_challenge[] = { 0x16, 0xe1, 0x6a };

static char *unlock_desc[] = { "+10% DEF",
                               "+10% HP",
                               "+1 MIGHT",
                               "+20% LUCK",
                               "FASTER HEAL",
                               "MOAR LEDs",
                               "U R CAESAR",
                               "U R SENATOR",
                               "BENDER",
                               "U R A GOD",
                               "PINGDUMP"};


static uint32_t last_ui_time = 0;
static uint8_t code[5];
static int8_t focused = 0;

/* prototypes */
static void updatecode(OrchardAppContext *, uint8_t, int8_t);
static void do_unlock(OrchardAppContext *context);
static void unlock_result(UnlockHandles *, char *);
static void destroy_unlock_ui(OrchardAppContext *);

static void unlock_result(UnlockHandles *p, char *msg) {
  gdispClear(Black);
  gdispDrawStringBox (0,
		      (gdispGetHeight() / 2) - (gdispGetFontMetric(p->font_manteka_20, fontHeight) / 2),
		      gdispGetWidth(),
		      gdispGetFontMetric(p->font_manteka_20, fontHeight),
		      msg,
		      p->font_manteka_20, Yellow, justifyCenter);

  if (code[0] == 0x0f) {
    // Almus / Caezer display
    gdispDrawStringBox (0,
                        60,
                        320,
                        gdispGetFontMetric(p->font_manteka_20, fontHeight),
                        "User: badgeinvite",
                        p->font_manteka_20, Cyan, justifyCenter);

    gdispDrawStringBox (0,
                        160,
                        320,
                        gdispGetFontMetric(p->font_manteka_20, fontHeight),
                        "http://famwqkizjdga.xyz",
                        p->font_manteka_20, Cyan, justifyCenter);
  }
}

static void init_unlock_ui(UnlockHandles *p) {

  GWidgetInit wi;

  p->font_jupiterpro_36 = gdispOpenFont("Jupiter_Pro_2561336");
  p->font_manteka_20 = gdispOpenFont("manteka20");

  gwinWidgetClearInit(&wi);

  // create button widget: ghNum1
  wi.g.show = TRUE;
  wi.g.x = 40;
  wi.g.y = 40;
  wi.g.width = 40;
  wi.g.height = 40;
  wi.text = "0";
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleStyle;
  p->ghNum1 = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghNum1, p->font_jupiterpro_36);
  gwinRedraw(p->ghNum1);

  // create button widget: ghNum2
  wi.g.show = TRUE;
  wi.g.x = 90;
  p->ghNum2 = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghNum2, p->font_jupiterpro_36);
  gwinRedraw(p->ghNum2);

  // create button widget: ghNum3
  wi.g.x = 140;
  p->ghNum3 = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghNum3, p->font_jupiterpro_36);
  gwinRedraw(p->ghNum3);

  // create button widget: ghNum4
  wi.g.x = 190;
  p->ghNum4 = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghNum4, p->font_jupiterpro_36);
  gwinRedraw(p->ghNum4);

  // create button widget: ghNum5
  wi.g.x = 240;
  p->ghNum5 = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghNum5,  p->font_jupiterpro_36);
  gwinRedraw(p->ghNum5);

  // create button widget: ghBack
  wi.g.x = 10;
  wi.g.y = 100;
  wi.g.width = 110;
  wi.g.height = 30;
  wi.text = "<-";
  p->ghBack = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghBack,  p->font_manteka_20);
  gwinRedraw(p->ghBack);

  // create button widget: ghUnlock
  wi.g.x = 200;
  wi.text = "UNLOCK";
  wi.customDraw = 0;
  p->ghUnlock = gwinButtonCreate(0, &wi);
  gwinSetFont(p->ghUnlock, p->font_manteka_20);
  gwinRedraw(p->ghUnlock);

  // reset focus
  focused = 0;
  gwinSetFocus(p->ghNum1);
}


static uint32_t unlock_init(OrchardAppContext *context)
{
  (void)context;

  /*
   * We don't want any extra stack space allocated for us.
   * We'll use the heap.
   */

  return (0);
}

static void unlock_start(OrchardAppContext *context)
{
  UnlockHandles *p;

  // clear the screen
  gdispClear (Black);

  // aha, you found us!
  i2sPlay("fight/leveiup.raw");

  /* background */
  putImageFile(IMG_UNLOCKBG, 0, 0);

  // idle ui timer (10s)
  orchardAppTimer(context, 10000000, true);
  last_ui_time = chVTGetSystemTime();

  p = malloc (sizeof(UnlockHandles));
  init_unlock_ui(p);
  context->priv = p;

  geventListenerInit(&p->glUnlockListener);
  gwinAttachListener(&p->glUnlockListener);
  geventRegisterCallback (&p->glUnlockListener,
                          orchardAppUgfxCallback,
                          &p->glUnlockListener);

  /* starting code */
  memset(&code, 0, 5);
  //  ledSetFunction(leds_show_unlocks);
  orchardAppTimer(context, 1000, true);
}

static void updatecode(OrchardAppContext *context, uint8_t position, int8_t direction) {
  GHandle wi;
  UnlockHandles * p;
  char tmp[2];

  p = context->priv;

  code[position] += direction;

  // check boundaries
  if (code[position] & 0x80) {
    code[position] = 0x0f; // underflow of unsigned int
  }

  if (code[position] > 0x0f) {
    code[position] = 0;
  }

  // update labels
  switch(position) {
  case 1:
    wi = p->ghNum2;
    break;
  case 2:
    wi = p->ghNum3;
    break;
  case 3:
    wi = p->ghNum4;
    break;
  case 4:
    wi = p->ghNum5;
    break;
  default: /* 0 */
    wi = p->ghNum1;
    break;
  }

  if (code[position] < 0x0a) {
    tmp[0] = (char) 0x30 + code[position];
  } else {
    tmp[0] = (char) 0x37 + code[position];
  }

  tmp[1] = '\0';

  gwinSetText(wi, tmp, TRUE);
}

static uint8_t validate_code(OrchardAppContext *context, userconfig *config) {
  /* check for valid code */
  uint8_t i;
  char tmp[40];
  UnlockHandles *p = context->priv;

  for (i=0; i < MAX_ULCODES; i++) {
    if ((unlock_codes[i][0] == code[0]) &&
        (unlock_codes[i][1] == ((code[1] << 4) + code[2])) &&
        (unlock_codes[i][2] == ((code[3] << 4) + code[4]))) {
      // set bit
      config->unlocks |= (1 << i);

      // if it's the luck upgrade, we set luck directly.
      if (i == 3) {
        config->luck = 40;
      }

      strcpy(tmp, unlock_desc[i]);
      strcat(tmp, " unlocked!");
      unlock_result(p, tmp);

      //      ledSetFunction(leds_all_strobe);

      if ((1 << i) != UL_BENDER)      // default sound
        i2sPlay("fight/leveiup.raw");

      // save to config
      configSave(config);

      chThdSleepMilliseconds(ALERT_DELAY);

      orchardAppRun(orchardAppByName("Badge"));
      return true;
    }
  }
  return false;
}

static void do_unlock(OrchardAppContext *context) {
  userconfig *config = getConfig();
  UnlockHandles * p;
  p = context->priv;

  destroy_unlock_ui(context);

  if (validate_code(context, config)) {
    return;
  } else {
    if (code[0] == 0x0f) {
      char tmp[20];
      // note that we used code[1] here twice to add in some entropy
      // else first unit of password would always be 0x0f & code[0]
      chsnprintf(tmp, sizeof(tmp),
                 "PW: %02x%02x%02x\n",
                 (( code[0] << 4 ) + code[1]) ^ unlock_challenge[0],
                 (( code[1] << 4 ) + code[2]) ^ unlock_challenge[1],
                 (( code[3] << 4 ) + code[4]) ^ unlock_challenge[2]);

      unlock_result(p, tmp);
      i2sPlay("alert1.raw");
      // give ane extra 10ms to write down the url
      chThdSleepMilliseconds(10000);
    } else {
      // no match
      //      ledSetFunction(leds_all_strobered);
      unlock_result(p, "unlock failed.");
      i2sPlay("fight/pathtic.raw");
    }

    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppRun(orchardAppByName("Badge"));
    return;
  }
}
static void unlock_event(OrchardAppContext *context,
                       const OrchardAppEvent *event)
{
  GEvent * pe;
  UnlockHandles * p;

  p = context->priv;

  // idle timeout
  if (event->type == timerEvent) {
    if( (chVTGetSystemTime() - last_ui_time) > (UI_IDLE_TIME * 1000)) {
      orchardAppRun(orchardAppByName("Badge"));
    }
    return;
  }

  if (event->type == ugfxEvent) {
    pe = event->ugfx.pEvent;
    last_ui_time = chVTGetSystemTime();

    if (pe->type == GEVENT_GWIN_BUTTON) {
      if ( ((GEventGWinButton*)pe)->gwin == p->ghBack) {
        orchardAppRun(orchardAppByName("Badge"));
        return;
      }

      if ( ((GEventGWinButton*)pe)->gwin == p->ghUnlock) {
        do_unlock(context);
      }

      /* tapping the number advances the value */
      if ( ((GEventGWinButton*)pe)->gwin == p->ghNum1)
        updatecode(context,0,1);

      if ( ((GEventGWinButton*)pe)->gwin == p->ghNum2)
        updatecode(context,1,1);

      if ( ((GEventGWinButton*)pe)->gwin == p->ghNum3)
        updatecode(context,2,1);

      if ( ((GEventGWinButton*)pe)->gwin == p->ghNum4)
        updatecode(context,3,1);

      if ( ((GEventGWinButton*)pe)->gwin == p->ghNum5)
        updatecode(context,4,1);
    }

  }

  // keyboard support
  if (event->type == keyEvent) {
    last_ui_time = chVTGetSystemTime();
    if ( (event->key.code == keyAUp) &&
         (event->key.flags == keyPress) ) {
      updatecode(context,focused,1);
    }
    if ( (event->key.code == keyADown) &&
         (event->key.flags == keyPress) ) {
      updatecode(context,focused,-1);
    }
    if ( (event->key.code == keyALeft) &&
         (event->key.flags == keyPress) ) {
      focused--;
      if (focused < 0) focused = 4;
    }
    if ( (event->key.code == keyARight) &&
         (event->key.flags == keyPress) ) {
      focused++;
      if (focused > 4) focused = 0;
    }

    // set focus
    gwinSetFocus(NULL);
    switch(focused) {
    case 0:
      gwinSetFocus(p->ghNum1);
      break;
    case 1:
      gwinSetFocus(p->ghNum2);
      break;
    case 2:
      gwinSetFocus(p->ghNum3);
      break;
    case 3:
      gwinSetFocus(p->ghNum4);
      break;
    case 4:
      gwinSetFocus(p->ghNum5);
      break;
    default:
      break;
    }

    if ( ((event->key.code == keyBSelect) ||
          (event->key.code == keyASelect)) &&
         (event->key.flags == keyPress) ) {
        do_unlock(context);
    }
  }
}

static void destroy_unlock_ui(OrchardAppContext *context) {
  UnlockHandles * p;
  p = context->priv;

  gwinDestroy(p->ghUnlock);
  gwinDestroy(p->ghNum1);
  gwinDestroy(p->ghNum2);
  gwinDestroy(p->ghNum3);
  gwinDestroy(p->ghNum4);
  gwinDestroy(p->ghNum5);
  gwinDestroy(p->ghBack);

  // if this is non-null we will destroy the ui again.
  p->ghUnlock = NULL;
}

static void unlock_exit(OrchardAppContext *context) {
  UnlockHandles * p;

  p = context->priv;

  if (p->ghUnlock != NULL) {
    destroy_unlock_ui(context);
  }

  gdispCloseFont (p->font_manteka_20);
  gdispCloseFont (p->font_jupiterpro_36);

  geventDetachSource (&p->glUnlockListener, NULL);
  geventRegisterCallback (&p->glUnlockListener, NULL, NULL);

  free(context->priv);
  context->priv = NULL;

  //  ledSetFunction(NULL);

}

/* We are a hidden app, only accessible through the konami code on the
   badge screen. */
orchard_app("Unlocks", NULL, APP_FLAG_HIDDEN, unlock_init,
    unlock_start, unlock_event, unlock_exit, 9999);
