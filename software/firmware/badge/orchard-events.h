#ifndef __ORCHARD_EVENTS__
#define __ORCHARD_EVENTS__

#include "gfx.h"
#include "joypad_lld.h"

#include "nrf_sdm.h"
#include "ble.h"
#include "ble_l2cap.h"
#include "ble_l2cap_lld.h"

/* Orchard event wrappers.
   These simplify the ChibiOS eventing system.  To use, initialize the event
   table, hook an event, and then dispatch events.

  static void shell_termination_handler(eventid_t id) {
    chprintf(stream, "Shell exited.  Received id %d\r\n", id);
  }

  void main(int argc, char **argv) {
    struct evt_table events;

    // Support up to 8 events
    evtTableInit(events, 8);

    // Call shell_termination_handler() when 'shell_terminated' is emitted
    evtTableHook(events, shell_terminated, shell_termination_handler);

    // Dispatch all events
    while (TRUE)
      chEvtDispatch(evtHandlers(events), chEvtWaitOne(ALL_EVENTS));
   }
 */

struct evt_table {
  int size;
  int next;
  evhandler_t *handlers;
  event_listener_t *listeners;
};

#define evtTableInit(table, capacity)                                       \
  do {                                                                      \
    static evhandler_t handlers[capacity];                                  \
    static event_listener_t listeners[capacity];                            \
    table.size = capacity;                                                  \
    table.next = 0;                                                         \
    table.handlers = handlers;                                              \
    table.listeners = listeners;                                            \
  } while(0)

#define evtTableHook(table, event, callback)                                \
  do {                                                                      \
    if (CH_DBG_ENABLE_ASSERTS != FALSE)                                     \
      if (table.next >= table.size)                                         \
        chSysHalt("event table overflow");                                  \
    chEvtRegister(&event, &table.listeners[table.next], table.next);        \
    table.handlers[table.next] = callback;                                  \
    table.next++;                                                           \
  } while(0)

#define evtTableUnhook(table, event, callback)                              \
  do {                                                                      \
    int i;                                                                  \
    for (i = 0; i < table.next; i++) {                                      \
      if (table.handlers[i] == callback)                                    \
        chEvtUnregister(&event, &table.listeners[i]);                       \
    }                                                                       \
  } while(0)

#define evtHandlers(table)                                                  \
    table.handlers

#define evtListeners(table)                                                 \
    table.listeners


/* Orchard App events */
typedef enum _OrchardAppEventType {
  keyEvent,
  appEvent,
  timerEvent,
  uiEvent,
  radioEvent,
  touchEvent,
  ugfxEvent,
} OrchardAppEventType;

/* ------- */

typedef struct _OrchardUiEvent {
  uint8_t   code;
  uint8_t   flags;
} OrchardUiEvent;

typedef enum _OrchardUiEventCode {
  uiComplete = 0x01,
} OrchardUiEventCode;

typedef enum _OrchardUiEventFlags {
  uiOK = 0x01,
  uiCancel,
  uiError,
} OrchardUiEventFlags;

typedef struct _OrchardAppKeyEvent {
  OrchardAppEventKeyCode code;
  OrchardAppEventKeyFlag flags;
} OrchardAppKeyEvent;

typedef struct _OrchardAppUgfxEvent {
  GListener * pListener;
  GEvent * pEvent;
} OrchardAppUgfxEvent;

typedef enum _OrchardAppRadioEventType {
   connectEvent,		/* GAP Connected to peer */
   disconnectEvent,		/* GAP Disconnected from peer */
   connectTimeoutEvent,		/* GAP Connection timed out */
   advertisementEvent,		/* GAP Received advertisement */
   scanResponseEvent,		/* GAP Received scan response */
   advAndScanEvent,		/* GAP Received adv + scan response */
   l2capConnectEvent,		/* L2CAP connection successful */
   l2capDisconnectEvent,	/* L2CAP connection closed */
   l2capConnectRefusedEvent,	/* L2CAP connection failed */
   l2capRxEvent,		/* L2CAP data received */
   l2capTxEvent,		/* L2CAP data sent */
   l2capTxDoneEvent,		/* L2CAP data acknowledged */
   gattsWriteEvent,		/* GATTS attribute write notification */
   gattsReadWriteAuthEvent,	/* GATTS attribute read/write auth request */
   gattsTimeout,		/* GATTS timeout */
   gattcServiceDiscoverEvent,	/* GATTC service discovery complete */
   gattcCharDiscoverEvent,	/* GATTC characteristic discovery complete */
   gattcAttrDiscoverEvent,	/* GATTC attribute discovery complete */
   gattcCharReadByUuidEvent,	/* GATTC characteristic read b/UUID complete */
   gattcCharReadEvent,		/* GATTC characteristic read complete */
   gattcCharWriteEvent,		/* GATTC characteristic write complete */
   gattcTimeout			/* GATTC timeout */
} OrchardAppRadioEventType;

typedef struct _OrchardAppRadioEvent {
   OrchardAppRadioEventType type;
   ble_evt_t evt;
   uint8_t pktlen;
   uint8_t * pkt;
} OrchardAppRadioEvent;

/* ------- */

typedef enum _OrchardAppLifeEventFlag {
  appStart,
  appTerminate,
} OrchardAppLifeEventFlag;

typedef struct _OrchardAppLifeEvent {
  uint8_t   event;
} OrchardAppLifeEvent;

/* ------- */

typedef struct _OrchardAppTimerEvent {
  uint32_t  usecs;
} OrchardAppTimerEvent;

/* ------- */

typedef struct _OrchardTouchEvent {
  uint16_t  x;
  uint16_t  y;
  uint16_t  z;
  uint16_t  temp;
  uint16_t  batt;
} OrchardTouchEvent;

typedef struct _OrchardAppEvent {
  OrchardAppEventType     type;
  union {
    OrchardAppKeyEvent    key;
    OrchardAppLifeEvent   app;
    OrchardAppTimerEvent  timer;
    OrchardUiEvent        ui;
    OrchardTouchEvent     touch;
    OrchardAppUgfxEvent   ugfx;
    OrchardAppRadioEvent  radio;
  } ;
} OrchardAppEvent;

#endif /* __ORCHARD_EVENTS__ */
