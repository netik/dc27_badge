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

#include "i2s_lld.h"

/* remember these many last key-pushes (app-default) */
#define KEY_HISTORY 9

static int8_t lastimg = -1;

typedef struct _DefaultHandles {
  GHandle ghFightButton;
  GHandle ghExitButton;
  GListener glBadge;
  font_t fontLG, fontSM, fontXS;

  /* ^ ^ v v < > < > ent will fire the unlocks screen */
  OrchardAppEventKeyCode last_pushed[KEY_HISTORY];
} DefaultHandles;


const OrchardAppEventKeyCode konami[] = { keyBUp, keyBUp, keyBDown,
                                          keyBDown, keyBLeft, keyBRight,
                                          keyBLeft, keyBRight, keyBSelect };
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

static void redraw_player(DefaultHandles *p) {
  // our image draws just a bit underneath the stats data. If we
  // draw the character during the HP update, it will blink and we
  // don't want that.
  const userconfig *config = getConfig();
  char tmp[20];
  char tmp2[40];

  // TODO: put player ship image based on fleet damage on screen

  // Put player rank based on us navy insignia
  sprintf(tmp2, "game/rank-%d.rgb", config->level);
  putImageFile(tmp2, 0, 100);

  //  putImageFile(getAvatarImage(config->current_type, img, class, false),
  //               POS_PLAYER1_X, totalheight - 42 - PLAYER_SIZE_Y);

  /* hit point bar */
  gdispDrawThickLine(0, 76, 240, 75, Blue, 2, FALSE);
  drawProgressBar(30,80,160,12,maxhp(config->current_type, config->unlocks,config->level), config->hp, 0, false);
  gdispDrawThickLine(0, 96, 240, 95, Blue, 2, FALSE);

  chsnprintf(tmp, sizeof(tmp), "HP");
  chsnprintf(tmp2, sizeof(tmp2), "%d", config->hp);

  gdispDrawStringBox (0 ,
		      78,
          30,
		      gdispGetFontMetric(p->fontXS, fontHeight),
		      tmp,
		      p->fontXS, White, justifyLeft);

  gdispFillArea( 0,78,
                 30,gdispGetFontMetric(p->fontXS, fontHeight),
                 Black );

  gdispDrawStringBox (200,
		      78,
          30,
		      gdispGetFontMetric(p->fontXS, fontHeight),
		      tmp2,
		      p->fontXS, White, justifyLeft);
}

static void draw_stat (DefaultHandles * p,
                       uint16_t x, uint16_t y,
                       char * str1, char * str2) {
  uint16_t lmargin = 120;

  gdispDrawStringBox (lmargin + x,
		      y,
		      gdispGetFontMetric(p->fontSM, fontMaxWidth)*strlen(str1),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str1,
		      p->fontSM, Cyan, justifyLeft);

  gdispDrawStringBox (lmargin + x + 15,
		      y,
                      72,
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      str2,
		      p->fontSM, White, justifyRight);
  return;
}

static void redraw_badge(DefaultHandles *p) {
  // draw the entire background badge image. Shown when the screen is idle.
  const userconfig *config = getConfig();

  char tmp[20];
  char tmp2[40];
  uint16_t ypos = 0;

  putImageFile("images/badge.rgb",0,0);

  redraw_player(p);

  gdispDrawStringBox (0,
		      ypos,
		      gdispGetWidth(),
		      gdispGetFontMetric(p->fontLG, fontHeight),
		      config->name,
		      p->fontLG, Cyan, justifyLeft);

  chsnprintf(tmp, sizeof(tmp), "LEVEL %d", config->level);

  /* Level */

  ypos = ypos + gdispGetFontMetric(p->fontLG, fontHeight);
  gdispDrawStringBox (0,
		      ypos,
		      gdispGetWidth(),
		      gdispGetFontMetric(p->fontSM, fontHeight),
		      tmp,
		      p->fontSM, Cyan, justifyRight);

  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 4;

  /* end hp bar */
  ypos = ypos + 30;

  /* XP/WON */
  chsnprintf(tmp2, sizeof(tmp2), "%3d", config->xp);
  draw_stat (p, 0, ypos, "XP", tmp2);


  /* AGL / LOST */
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;
  chsnprintf(tmp2, sizeof(tmp2), "%3d", config->agl);
  draw_stat (p, 0, ypos, "AGL", tmp2);

  /* MIGHT */
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;
  if (config->unlocks & UL_PLUSMIGHT)
    chsnprintf(tmp2, sizeof(tmp2), "%3d", config->might + 1);
  else
    chsnprintf(tmp2, sizeof(tmp2), "%3d", config->might);
  draw_stat (p, 0, ypos, "MIGHT", tmp2);

  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;
  chsnprintf(tmp2, sizeof(tmp2), "%3d", config->won);
  draw_stat (p, 0, ypos, "WON", tmp2);
  ypos = ypos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;
  chsnprintf(tmp2, sizeof(tmp2), "%3d", config->lost);
  draw_stat (p, 0, ypos, "LOST", tmp2);

  //  if (config->airplane_mode)
  //    putImageFile(IMG_PLANE, 0, 0);

#ifdef LEADERBOARD_AGENT
  screen_alert_draw(false, "LB AGENT MODE");
#endif

}

static uint32_t default_init(OrchardAppContext *context) {
  (void)context;

  //  orchardAppTimer(context, HEAL_INTERVAL_US, true);

  return 0;
}

static void default_start(OrchardAppContext *context) {
  DefaultHandles * p;
  const userconfig *config = getConfig();

  p = malloc(sizeof(DefaultHandles));
  context->priv = p;

  p->fontXS = gdispOpenFont (FONT_XS);
  p->fontLG = gdispOpenFont (FONT_LG);
  p->fontSM = gdispOpenFont (FONT_FIXED);

  gdispClear(Black);

  gdispSetOrientation (0);

  lastimg = -1;
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
  userconfig *config = getConfig();
  tmElements_t dt;
  char tmp[40];
  uint16_t lmargin;
  coord_t totalheight = gdispGetHeight();
  coord_t totalwidth = gdispGetWidth();

  lmargin = 61;

  p = context->priv;

  if (event->type == keyEvent) {
    if (event->key.flags == keyPress)
      storeKey(context, event->key.code);

    /* hitting enter goes into fight, unless konami has been entered,
       then we send you to the unlock screen! */
    if ( (event->key.code == keyBSelect) &&
         (event->key.flags == keyPress) )  {

      if (testKonami(context)) {
        orchardAppRun(orchardAppByName("Unlocks"));
      }
      return;
    }

  }

  if (event->type == ugfxEvent) {
    pe = event->ugfx.pEvent;

    switch(pe->type) {
    case GEVENT_GWIN_BUTTON:
      if (((GEventGWinButton*)pe)->gwin == p->ghFightButton) {
        orchardAppRun(orchardAppByName("Fight"));
        return;
      }

      if (((GEventGWinButton*)pe)->gwin == p->ghExitButton) {
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
