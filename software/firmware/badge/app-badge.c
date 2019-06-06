#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "orchard-app.h"
#include "orchard-events.h"

#include "rand.h"
#include "led.h"
#include "orchard-ui.h"
#include "images.h"
#include "fontlist.h"

#include "ides_gfx.h"
#include "unlocks.h"

#include "userconfig.h"
#include "datetime.h"
#include "src/gdisp/gdisp_driver.h"

#include "i2s_lld.h"

/* remember these many last key-pushes (app-default) */
#define KEY_HISTORY 8

typedef struct _DefaultHandles {
  GHandle ghFightButton;
  GHandle ghExitButton;
  GListener glBadge;
  font_t fontLG, fontSM, fontXS;

  /* ^ ^ v v < > < > ent will fire the unlocks screen */
  OrchardAppEventKeyCode last_pushed[KEY_HISTORY];
} DefaultHandles;

// Konami code. The last can be either select button.
// in final release we will extend this to
// up, up, down, down, left, right, left, right, B, A.
// PROD
// #define KEY_HISTORY 10
//const OrchardAppEventKeyCode konami[] = { keyAUp, keyAUp, keyADown,
//                                          keyADown, keyALeft, keyARight,
//                                          keyALeft, keyARight, keyBLeft, keyBRight };

// DEV
const OrchardAppEventKeyCode konami[KEY_HISTORY] = { keyAUp, keyAUp, keyADown,
                                          keyADown, keyALeft, keyARight,
                                          keyALeft, keyARight };

static void destroy_buttons(DefaultHandles *p) {
  gwinDestroy (p->ghFightButton);
  gwinDestroy (p->ghExitButton);
}

static void draw_buttons(DefaultHandles * p) {
  GWidgetInit wi;
  coord_t totalheight = gdispGetHeight();

  // Apply some default values for GWIN
  gwinWidgetClearInit(&wi);
  wi.g.show = TRUE;
  wi.g.width = 60;
  wi.g.height = 60;
  wi.g.y = totalheight - 60;
  wi.g.x = 0;
  wi.text = "";
  wi.customDraw = noRender;

  p->ghFightButton = gwinButtonCreate(NULL, &wi);

  wi.g.x = gdispGetWidth() - 60;
  p->ghExitButton = gwinButtonCreate(NULL, &wi);
}

static void draw_stat (DefaultHandles * p,
                       uint16_t x, uint16_t y,
                       char * str1, char * str2) {

  gdispDrawStringBox (x+1,
		      y+1,
		      gdispGetFontMetric(p->fontSM, fontMaxWidth)*strlen(str1),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str1,
		      p->fontSM, Black, justifyLeft);

  gdispDrawStringBox (x,
		      y,
		      gdispGetFontMetric(p->fontSM, fontMaxWidth)*strlen(str1),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str1,
		      p->fontSM, Red, justifyLeft);

  gdispDrawStringBox (x+101 ,
		      y+1,
		      gdispGetFontMetric(p->fontSM, fontMaxWidth)*strlen(str2),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str2,
		      p->fontSM, Black, justifyLeft);

  gdispDrawStringBox (x+100,
		      y,
		      gdispGetFontMetric(p->fontSM, fontMaxWidth)*strlen(str2),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str2,
		      p->fontSM, White, justifyLeft);
  return;
}

static void redraw_badge(DefaultHandles *p) {
  // draw the entire background badge image. Shown when the screen is idle.
  const userconfig *config = getConfig();

  char tmp[20];
  coord_t ypos = 5;

  // Background
  putImageFile("images/badge.rgb",0,0);

  // Rank
  sprintf(tmp, "%s", rankname[config->level-1]);
  gdispDrawStringBox (5,
		      ypos,
		      gdispGetWidth(),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      tmp,
		      p->fontSM, White, justifyLeft);

  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight);

  // user's name
  gdispDrawStringBox (0,
		      ypos,
		      gdispGetWidth()-5,
		      gdispGetFontMetric(p->fontLG, fontHeight),
		      config->name,
		      p->fontLG, White, justifyRight);

  ypos = 77;

  // level
  sprintf(tmp, "LEVEL %d", config->level);
  gdispDrawStringBox (0,
          ypos,
          gdispGetWidth(),
          gdispGetFontMetric(p->fontSM, fontHeight),
          tmp,
          p->fontSM, Red, justifyCenter);

  // insignia
  sprintf(tmp, "game/rank-%d.rgb", config->level);
  putImageFile(tmp, 92, 106);
  gdispDrawBox(91,105,53,95,Grey);

  /* XP/WON */
  ypos=210;
  chsnprintf(tmp, sizeof(tmp), "%3d", config->xp);
  draw_stat (p, 50, ypos, "XP", tmp);
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;

  // WON
  chsnprintf(tmp, sizeof(tmp), "%3d", config->won);
  draw_stat (p, 50, ypos, "WON", tmp);
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;

  // LOST
  chsnprintf(tmp, sizeof(tmp), "%3d", config->lost);
  draw_stat (p, 50, ypos, "LOST", tmp);
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;

  if (config->airplane_mode) // right under the user name.
    putImageFile(IMG_PLANE, 190, 77);
}

static uint32_t default_init(OrchardAppContext *context) {
  (void)context;

  //  orchardAppTimer(context, HEAL_INTERVAL_US, true);

  return 0;
}

static void default_start(OrchardAppContext *context) {
  DefaultHandles * p;

  p = malloc(sizeof(DefaultHandles));
  context->priv = p;

  p->fontXS = gdispOpenFont (FONT_XS);
  p->fontLG = gdispOpenFont (FONT_LG);
  p->fontSM = gdispOpenFont (FONT_SM);

  for (int i=0; i < KEY_HISTORY; i++) {
    p->last_pushed[i] = 0;
  }

  gdispClear(Black);

  gdispSetOrientation (0);

  redraw_badge(p);
  draw_buttons(p);

  geventListenerInit(&p->glBadge);
  gwinAttachListener(&p->glBadge);
  geventRegisterCallback (&p->glBadge, orchardAppUgfxCallback, &p->glBadge);
}

static inline void storeKey(OrchardAppContext *context, OrchardAppEventKeyCode key) {
  /* remember the last pushed keys to enable the konami code */
  DefaultHandles * p;
  p = context->priv;

  for (int i=1; i < KEY_HISTORY; i++) {
    p->last_pushed[i-1] = p->last_pushed[i];
  }

  p->last_pushed[KEY_HISTORY-1] = key;
};

static inline uint8_t testKonami(OrchardAppContext *context) {
  DefaultHandles * p;
  p = context->priv;
  for (int i=0; i < KEY_HISTORY; i++) {
    if (p->last_pushed[i] != konami[i])
      return false;
  }
  return true;
}

static void default_event(OrchardAppContext *context,
	const OrchardAppEvent *event) {
  DefaultHandles * p;
  GEvent * pe;
  char tmp[40];
  coord_t totalheight = gdispGetHeight();
  coord_t totalwidth = gdispGetWidth();

  p = context->priv;

  if (event->type == keyEvent) {
    if (event->key.flags == keyPress) {
      i2sPlay("sound/click.snd");
      if (event->key.code == keyASelect ||
			    event->key.code == keyBSelect) {
          if (testKonami(context)) {
            orchardAppRun(orchardAppByName("Unlocks"));
          }
          return;
      }
      // else store it for later
      storeKey(context, event->key.code);
    }
  }

  if (event->type == ugfxEvent) {
    pe = event->ugfx.pEvent;

    switch(pe->type) {
    case GEVENT_GWIN_BUTTON:
      if (((GEventGWinButton*)pe)->gwin == p->ghFightButton) {
        orchardAppRun(orchardAppByName("Sea Battle"));
        return;
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghExitButton) {
        i2sPlay("sound/click.snd");
        orchardAppExit();
        return;
      }
    }
  }

  /* timed events (heal, caesar election, etc.) */
  if (event->type == timerEvent) {

    gdispFillArea( totalwidth - 100, totalheight - 50,
                   100, gdispGetFontMetric(p->fontXS, fontHeight),
                   Black );

    gdispDrawStringBox (0,
                        totalheight - 50,
                        totalwidth,
                        gdispGetFontMetric(p->fontXS, fontHeight),
                        tmp,
                        p->fontXS, White, justifyRight);

  }
}

static void default_exit(OrchardAppContext *context) {
  DefaultHandles * p;
  orchardAppTimer(context, 0, false); // shut down the timer

  p = context->priv;
  destroy_buttons(p);

  gdispCloseFont (p->fontXS);
  gdispCloseFont (p->fontLG);
  gdispCloseFont (p->fontSM);

  geventDetachSource (&p->glBadge, NULL);
  geventRegisterCallback (&p->glBadge, NULL, NULL);

  free(context->priv);
  context->priv = NULL;

  gdispSetOrientation (GDISP_DEFAULT_ORIENTATION);

  return;
}

orchard_app("Badge", "icons/anchor.rgb", 0, default_init, default_start,
	default_event, default_exit, 0);
