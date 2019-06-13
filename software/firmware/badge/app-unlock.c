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
#include "fontlist.h"

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
  font_t font_futara_12;
  font_t font_futara_20;
  font_t font_futara_36;

  // code frame buffers
  pixel_t *codefb[MAX_CODE_LEN]; /* pointers to frame buffers */

} UnlockHandles;

/* codes, packed as bits. Note only last five nibbles (4 bits) are used, MSB of
   1st byte is always zero */

static volatile long unsigned int *unlock_codes[MAX_ULCODES] = {
  UL_CODE_0 ,
  UL_CODE_1 ,
  UL_CODE_2 ,
  UL_CODE_3 ,
  UL_CODE_4 ,
  UL_CODE_5 ,
  UL_CODE_6 ,
  UL_CODE_7 ,
  UL_CODE_8 ,
  UL_CODE_9 ,
  UL_CODE_10
};

static char *unlock_desc[] = { "+10% DEF",
                               "+10% HP",
                               "+10% DMG",
                               "+20% SPEED",
                               "FAST REPAIR",
                               "MOAR LEDs",
                               "MOAR VIDEO",
                               "CARRIER",
                               "SUBMARINE",
                               "GOD MODE",
                               "DEBUG MODE"
};

static uint32_t last_ui_time = 0;
static uint8_t code[MAX_CODE_LEN];
static int8_t focused = 0;

/* prototypes */
static void updatecode(OrchardAppContext *, uint8_t, int8_t);
static void do_unlock(OrchardAppContext *context);
static void unlock_result(UnlockHandles *, char *);
static void destroy_unlock_ui(OrchardAppContext *);

static void unlock_result(UnlockHandles *p, char *msg) {
  gdispClear(Black);
  // lines around text
  gdispDrawThickLine(0,
    (gdispGetHeight() / 2) - (gdispGetFontMetric(p->font_futara_36, fontHeight) / 2) - 10,
    gdispGetWidth(),
    (gdispGetHeight() / 2) - (gdispGetFontMetric(p->font_futara_36, fontHeight) / 2) - 10,
    Blue,
    4,
    false
  );

  gdispDrawStringBox (0,
          (gdispGetHeight() / 2) - (gdispGetFontMetric(p->font_futara_36, fontHeight) / 2),
          gdispGetWidth(),
          gdispGetFontMetric(p->font_futara_36, fontHeight),
          msg,
          p->font_futara_36, Yellow, justifyCenter);

  gdispDrawThickLine(0,
    (gdispGetHeight() / 2) + (gdispGetFontMetric(p->font_futara_36, fontHeight) / 2) + 10,
    gdispGetWidth(),
    (gdispGetHeight() / 2) + (gdispGetFontMetric(p->font_futara_36, fontHeight) / 2) + 10,
    Blue,
    4,
    false
  );

}

static void redraw_code(UnlockHandles *p) {
  char msg[4];

  for (int i=0; i<5; i++) {
    sprintf(msg,"%x",code[i]);
    drawBufferedStringBox(&p->codefb[i],
      15 + (i*60),
      10,
      50,
      gdispGetFontMetric(p->font_futara_20, fontHeight),
      msg,
      p->font_futara_20, Yellow, justifyCenter);
  }
}

static void init_unlock_ui(UnlockHandles *p) {

  GWidgetInit wi;

  gwinWidgetClearInit(&wi);
  gwinSetDefaultFont(p->font_futara_12);

  // create button widget: ghNum1
  wi.g.show = TRUE;
  wi.g.x = 15;
  wi.g.y = 40;
  wi.g.width = 50;
  wi.g.height = 50;
  wi.text = "0";
  wi.customDraw = noRender;
  wi.customParam = 0;

  // the buttons are not exactly buttons, they are just areas which we can
  // receieve clicks in
  p->ghNum1 = gwinButtonCreate(0, &wi);

  // create button widget: ghNum2
  wi.g.show = TRUE;
  wi.g.x = 75;
  p->ghNum2 = gwinButtonCreate(0, &wi);

  // create button widget: ghNum3
  wi.g.x = 135;
  p->ghNum3 = gwinButtonCreate(0, &wi);

  // create button widget: ghNum4
  wi.g.x = 195;
  p->ghNum4 = gwinButtonCreate(0, &wi);

  // create button widget: ghNum5
  wi.g.x = 255;
  p->ghNum5 = gwinButtonCreate(0, &wi);

  // create button widget: ghBack
  wi.g.x = 10;
  wi.g.y = 120;
  wi.g.width = 110;
  wi.g.height = 30;
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleFilledStyle;
  wi.text = "CANCEL";
  p->ghBack = gwinButtonCreate(0, &wi);
  gwinRedraw(p->ghBack);

  // create button widget: ghUnlock
  wi.g.x = 200;
  wi.text = "UNLOCK";
  wi.customDraw = gwinButtonDraw_Normal;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleFilledStyle;
  p->ghUnlock = gwinButtonCreate(0, &wi);
  gwinRedraw(p->ghUnlock);

  // reset focus
  focused = 0;

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

static void update_focus(void) {
  color_t c;

  for (int i = 0; i < 5; i++)
  {
    if (i == focused) {
      c = Yellow;
    } else {
      c = Black;
    }

    // draw a slightly bigger box around the flags
    gdispDrawBox(14 + (i*60), 39, 52, 52, c);
    gdispDrawBox(13 + (i*60), 38, 53, 53, c);
  }
}

static void redraw_flags(void) {
  for (int i = 0; i < 5; i++)
  {
    char tmp[30];
    sprintf(tmp, "flags/flag-%x.rgb", code[i]);
    putImageFile(tmp, 15 + (i*60), 40);
  }
  update_focus();
}

static void unlock_start(OrchardAppContext *context)
{
  UnlockHandles *p;

  gdispClear(Black);

  // aha, you found us! make some noise.
  i2sPlay("sound/levelup.snd");

  // fill background
  putImageFile(IMG_UNLOCKBG, 0, 0);

  // draw UI
  /* starting code */
  memset(&code, 0, 5);

  p = (UnlockHandles *)malloc (sizeof(UnlockHandles));
  p->font_futara_12 = gdispOpenFont (FONT_XS);
  p->font_futara_20 = gdispOpenFont (FONT_SM);
  p->font_futara_36 = gdispOpenFont (FONT_LG);
  init_unlock_ui(p);
  context->priv = p;

  // clear buffers
  memset(p->codefb, 0, sizeof(pixel_t *) * MAX_CODE_LEN);
  // draw flags for first time
  redraw_flags();
  redraw_code(p);

  // idle ui timer (10s)
  last_ui_time = chVTGetSystemTime();
  orchardAppTimer(context, 10000000, true);

  geventListenerInit(&p->glUnlockListener);
  gwinAttachListener(&p->glUnlockListener);
  geventRegisterCallback (&p->glUnlockListener,
                          orchardAppUgfxCallback,
                          &p->glUnlockListener);



  // display the current unlocks on the LEDs
  ledSetPattern(LED_PATTERN_UNLOCK);
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

  // redraw the flags
  redraw_flags();
  redraw_code(p);

}

static uint8_t validate_code(OrchardAppContext *context, userconfig *config) {
  /* check for valid code */
  uint8_t i;
  char tmp[40];
  UnlockHandles *p = context->priv;
  long unsigned int mycode;

  // each code is a byte. shift it accordingly
  mycode = (code[0] << 0) +
    (code[1] << 8) +
    (code[2] << 16) +
    (code[3] << 24);

  for (i=0; i < MAX_ULCODES; i++) {
    printf("%d\n", i);
    if (*unlock_codes[i] == mycode) {
      // set bit
      config->unlocks |= (1 << i);

      strcpy(tmp, unlock_desc[i]);
      strcat(tmp, " unlocked!");
      unlock_result(p, tmp);

      //      ledSetFunction(leds_all_strobe);
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

  // remove the UI and repaint the background
  destroy_unlock_ui(context);
  putImageFile(IMG_UNLOCKBG, 0, 0);

  if (validate_code(context, config)) {
    return;
  }

  // no match
  //      ledSetFunction(leds_all_strobered);
  unlock_result(p, "Unlock Failed.");
  i2sPlay("sound/wilhelm.snd");
  chThdSleepMilliseconds(ALERT_DELAY);
  orchardAppRun(orchardAppByName("Badge"));
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

    update_focus();

    if ( (event->key.code == keyBSelect) &&
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
  userconfig *config = getConfig();

  p = context->priv;
  if (p->ghUnlock != NULL) {
    destroy_unlock_ui(context);
  }

  gdispCloseFont (p->font_futara_12);
  gdispCloseFont (p->font_futara_20);
  gdispCloseFont (p->font_futara_36);
  geventDetachSource (&p->glUnlockListener, NULL);
  geventRegisterCallback (&p->glUnlockListener, NULL, NULL);

  for (int i=0; i < MAX_CODE_LEN; i++) {
    if (p->codefb[i] != NULL) {
      free(p->codefb[i]);
    }
  }
  free(context->priv);
  context->priv = NULL;

  // put the LEDs back
  ledSetPattern(config->led_pattern);
}

/* We are a hidden app, only accessible through the konami code on the
   badge screen. */
orchard_app("Unlocks", "icons/bell.gif", APP_FLAG_HIDDEN, unlock_init,
    unlock_start, unlock_event, unlock_exit, 9999);
