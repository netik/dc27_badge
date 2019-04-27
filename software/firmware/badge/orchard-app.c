#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard-app.h"
#include "orchard-events.h"
#include "orchard-ui.h"

#include "i2s_lld.h"
#include "joypad_lld.h"

extern OrchardAppEvent joyEvent;

orchard_app_start();
orchard_app_end();

bool (*app_radio_notify)(void *);

const OrchardApp *orchard_app_list;

/* the one and in fact only instance of any orchard app */
orchard_app_instance instance;  

/* graphics event handle */
static OrchardAppEvent ugfx_evt;

/* orchard event sources */
event_source_t unlocks_updated;
event_source_t orchard_app_terminated;
event_source_t orchard_app_terminate;
event_source_t timer_expired;
event_source_t ui_completed;
event_source_t orchard_app_gfx;
event_source_t orchard_app_radio;
event_source_t orchard_app_key;

static uint8_t ui_override = 0;

#define RADIO_QUEUE_LEN	16

static uint8_t prod_idx;
static uint8_t cons_idx;
static uint8_t queue_cnt;

static OrchardAppRadioEvent * radio_evt[RADIO_QUEUE_LEN];
static uint8_t * radio_pkt[RADIO_QUEUE_LEN];

void orchardAppUgfxCallback (void * arg, GEvent * pe)
{
  GListener * gl;

  gl = (GListener *)arg;

  ugfx_evt.type = ugfxEvent;
  ugfx_evt.ugfx.pListener = gl;
  ugfx_evt.ugfx.pEvent = pe;

  chEvtBroadcast (&orchard_app_gfx);

  return;
}

static void ugfx_event(eventid_t id) {

  (void) id;

  if (ugfx_evt.ugfx.pEvent->type != GEVENT_GWIN_BUTTON_UP) {
    if (strcmp (instance.app->name, "Launcher") == 0)
      i2sPlay ("sound/ping.snd");
    else
      i2sPlay ("sound/click.snd");
  }
  instance.app->event (instance.context, &ugfx_evt);
  geventEventComplete (ugfx_evt.ugfx.pListener);

  return;
}

static void flush_radio_queue (void) {
  int i;

  for (i = 0; i < RADIO_QUEUE_LEN; i++) {
    if (radio_evt[i] != NULL)
        free (radio_evt[i]);
    radio_evt[i] = NULL;
    if (radio_pkt[i] != NULL)
        free (radio_pkt[i]);
    radio_pkt[i] = NULL;
  }

  prod_idx = 0;
  cons_idx = 0;
  queue_cnt = 0;

  return;
}
void orchardAppRadioCallback (OrchardAppRadioEventType type,
  ble_evt_t * evt, void * pkt, uint8_t len) {
 
  OrchardAppRadioEvent * r_evt;

  if (instance.context == NULL)
    return;

  /*
   * We buffer the current frame. If another frame arrives before
   * the app has processed it, then we drop the new frame. We could
   * expand this to permit buffering of more frames, but for now
   * this seems sufficient.
   */

  if (queue_cnt < RADIO_QUEUE_LEN) {
    r_evt = malloc (sizeof(OrchardAppRadioEvent));
    memset (r_evt, 0, sizeof(OrchardAppRadioEvent));
    radio_evt[prod_idx] = r_evt;

    r_evt->type = type;

    if (pkt != NULL && len != 0)
      {
      radio_pkt[prod_idx] = malloc (len);
      r_evt->pkt = radio_pkt[prod_idx];
      memcpy (r_evt->pkt, pkt, len);
      r_evt->pktlen = len;
      }

    if (evt != NULL)
      memcpy (&r_evt->evt, evt, sizeof(ble_evt_t));
    queue_cnt++;
    prod_idx++;
    if (prod_idx == RADIO_QUEUE_LEN)
        prod_idx = 0;
  } else
    return;

  chEvtBroadcast (&orchard_app_radio);

  return;
}

static void radio_event(eventid_t id) {
  OrchardAppEvent evt;
  OrchardAppRadioEvent * r_evt;
  bool r;

  (void) id;

  if (instance.context != NULL) {
    evt.type = radioEvent;
    while (queue_cnt) {

        r_evt = radio_evt[cons_idx];

        /*
         * When someone writes to one of our characteristics, it could
         * be a notification of some kind (e.g. chat request). If no
	 * other app is running besides the launcher, run the notify
	 * app to announce what happened.
	 */

        r = FALSE;

        if (strcmp (instance.app->name, "Launcher") == 0 &&
            (r_evt->type == gattsReadWriteAuthEvent ||
            r_evt->type == gattsWriteEvent))
          r = app_radio_notify (r_evt);

        if (r == FALSE) {
          memcpy (&evt.radio, r_evt, sizeof(OrchardAppRadioEvent));
          instance.app->event (instance.context, &evt);
        }

        free (radio_evt[cons_idx]);
        if (radio_pkt[cons_idx] != NULL)
            free (radio_pkt[cons_idx]);
        radio_evt[cons_idx] = NULL;
        radio_pkt[cons_idx] = NULL;

        queue_cnt--;

        cons_idx++;
        if (cons_idx == RADIO_QUEUE_LEN)
            cons_idx = 0;
    }
  }

  return;
}

static void key_event(eventid_t id) {

  (void) id;

  if (instance.context != NULL) {
      if (strcmp (instance.app->name, "Launcher") == 0 &&
          joyEvent.key.flags == keyPress)
        i2sPlay ("sound/ping.snd");
      instance.app->event(instance.context, &joyEvent);
  }
  return;
}

static void ui_complete_cleanup(eventid_t id) {
  (void)id;
  OrchardAppEvent evt;
 
  evt.type = uiEvent;
  evt.ui.code = uiComplete;
  evt.ui.flags = uiOK;

  instance.app->event(instance.context, &evt);  

  return;
}

static void terminate(eventid_t id) {
  OrchardAppEvent evt;

  (void)id;

  if (!instance.app->event)
    return;

  evt.type = appEvent;
  evt.app.event = appTerminate;
  instance.app->event(instance.context, &evt);

  flush_radio_queue ();

  chThdTerminate(instance.thr);
}

static void timer_event(eventid_t id) {

  (void)id;
  OrchardAppEvent evt;

  if (!instance.app->event)
    return;

  evt.type = timerEvent;
  evt.timer.usecs = instance.timer_usecs;
  if( !ui_override )
    {
    instance.app->event(instance.context, &evt);
    }

  if (instance.timer_repeating)
    orchardAppTimer(instance.context, instance.timer_usecs, true);
}

static void timer_do_send_message(void *arg) {

  (void)arg;
  chSysLockFromISR();
  chEvtBroadcastI(&timer_expired);
  chSysUnlockFromISR();
}

const OrchardApp *orchardAppByName(const char *name) {
  const OrchardApp *current;

  current = orchard_apps();
  while(current->name) {
    if( !strncmp(name, current->name, 16) ) {
      return current;
    }
    current++;
  }
  return NULL;
}

void orchardAppRun(const OrchardApp *app) {
  instance.next_app = app;
  chThdTerminate(instance.thr);
  chEvtBroadcast(&orchard_app_terminate);
}

void orchardAppExit(void) {
  instance.next_app = orchard_app_list;  // the first app is the launcher

  /*
   * The call to terminate the app thread below seems to be wrong. The
   * terminate() handler will be invoked by broadcasting the app
   * terminate event, and it already calls chThdTerminate() once it sends
   * the app terminate event to the app's event handler, and that can't
   * happen if the thread has already exited from here.
   */
  /*chThdTerminate(instance.thr);*/

  chEvtBroadcast(&orchard_app_terminate);
}

void orchardAppTimer(const OrchardAppContext *context,
                     uint32_t usecs,
                     bool repeating) {

  if (!usecs) {
    chVTReset(&context->instance->timer);
    context->instance->timer_usecs = 0;
    return;
  }

  context->instance->timer_usecs = usecs;
  context->instance->timer_repeating = repeating;

  /*
   * The TIME_US2I() macro used by ChibiOS does not work with
   * large microsecond counts when the system tick resolution is
   * fairly high. With a resolutio of 10000, the TIME_US2I() macro
   * macro wraps at 429496. So for values above this, we need to
   * base the calculation on milliseconds instead.
   */

  if (usecs > 429496)
    chVTSet(&context->instance->timer, TIME_MS2I(usecs / 1000),
      timer_do_send_message, NULL);
  else
    chVTSet(&context->instance->timer, TIME_US2I(usecs),
      timer_do_send_message, NULL);

  return;
}

// jna: second argument here is stack space allocated to the application thread.
// 0x300 = 768  = Breaks app-fight.c badly
// 0x400 = 1024 = Works fine with app-fight.c.
//
// Please don't modify this unless absolutely necessary. I will figure
// out how to reduce stack usage in app-fight.c 

static THD_WORKING_AREA(waOrchardAppThread, 1280);
static THD_FUNCTION(orchard_app_thread, arg) {

  struct orchard_app_instance *instance = arg;
  struct evt_table orchard_app_events;
  OrchardAppEvent evt;
  OrchardAppContext app_context;

  (void)arg;

  ui_override = 0;
  memset(&app_context, 0, sizeof(app_context));
  instance->context = &app_context;
  app_context.instance = instance;
  
  // set UI elements to null
  instance->ui = NULL;
  instance->uicontext = NULL;
  instance->ui_result = 0;

  evtTableInit(orchard_app_events, 6);
  evtTableHook(orchard_app_events, ui_completed, ui_complete_cleanup);
  evtTableHook(orchard_app_events, orchard_app_terminate, terminate);
  evtTableHook(orchard_app_events, orchard_app_gfx, ugfx_event);
  evtTableHook(orchard_app_events, orchard_app_radio, radio_event);
  evtTableHook(orchard_app_events, orchard_app_key, key_event);
  evtTableHook(orchard_app_events, timer_expired, timer_event);

  // if APP is null, the system will crash here. 
  if (instance->app->init)
    app_context.priv_size = instance->app->init(&app_context);
  else
    app_context.priv_size = 0;

  chRegSetThreadName(instance->app->name);

  /* Allocate private data on the stack (word-aligned) */
  uint32_t priv_data[app_context.priv_size / 4];

  if (app_context.priv_size) {
    memset(priv_data, 0, sizeof(priv_data));
    app_context.priv = priv_data;
  } else {
    app_context.priv = NULL;
  }

  /*
   * Initialize the radio event queue.
   * Note: Any events left unserviced by previous app
   * are discarded.
   */

  flush_radio_queue ();

  if (instance->app->start)
    instance->app->start(&app_context);
 
  if (instance->app->event) {
    {
      evt.type = appEvent;
      evt.app.event = appStart;
      instance->app->event(instance->context, &evt);
    }
    while (!chThdShouldTerminateX())
      chEvtDispatch(evtHandlers(orchard_app_events), chEvtWaitOne(ALL_EVENTS));
  }

  chVTReset(&instance->timer);

  if (instance->app->exit)
    instance->app->exit(&app_context);

  instance->context = NULL;

  /* Set up the next app to run when the orchard_app_terminated message is
     acted upon.*/
  if (instance->next_app)
    instance->app = instance->next_app;
  else
    instance->app = orchard_app_list;
  instance->next_app = NULL;

  evtTableUnhook(orchard_app_events, timer_expired, timer_event);
  evtTableUnhook(orchard_app_events, orchard_app_terminate, terminate);
  evtTableUnhook(orchard_app_events, ui_completed, ui_complete_cleanup);
  evtTableUnhook(orchard_app_events, orchard_app_gfx, ugfx_event);
  evtTableUnhook(orchard_app_events, orchard_app_radio, radio_event);
  evtTableUnhook(orchard_app_events, orchard_app_key, key_event);
 
  /* Atomically broadcasting the event source and terminating the thread,
     there is not a chSysUnlock() because the thread terminates upon return.*/

  chSysLock();
  chEvtBroadcastI(&orchard_app_terminated);
  chThdExitS(MSG_OK);
}

void orchardAppInit(void) {
  const OrchardApp * current;

  orchard_app_list = orchard_apps();
  instance.app = orchard_app_list;
  chEvtObjectInit(&orchard_app_terminate);
  chEvtObjectInit(&timer_expired);
  chEvtObjectInit(&ui_completed);
  chEvtObjectInit(&orchard_app_gfx);
  chEvtObjectInit(&orchard_app_radio);
  chEvtObjectInit(&orchard_app_key);
  chVTReset(&instance.timer);

  current = orchard_app_list;
  while (current->name) {
    if (current->flags & APP_FLAG_AUTOINIT)
      current->init(NULL);
    current++;
  }

}

void orchardAppRestart(void) {

  /* Recovers memory of the previous application. */
  if (instance.thr) {
    osalDbgAssert(chThdTerminatedX(instance.thr), "App thread still running");
    chThdRelease(instance.thr);
    instance.thr = NULL;
  }

  instance.thr = chThdCreateStatic(waOrchardAppThread,
                                   sizeof(waOrchardAppThread),
                                   ORCHARD_APP_PRIO,
                                   orchard_app_thread,
                                   (void *)&instance);
}
