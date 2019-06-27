/*
 * app-battle.c
 *
 * Multiplayer, peer-to-peer rewrite similar to the old "Sea Battle"
 * game.
 *
 * J. Adams 4/27/2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "nrf52i2s_lld.h"
#include "ides_gfx.h"
#include "images.h"
#include "battle.h"
#include "gll.h"
#include "math.h"
#include "led.h"
#include "userconfig.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

// debugs the discovery process
#define DEBUG_ENEMY_DISCOVERY
#define DEBUG_BATTLE_STATE 1
#define DEBUG_ENEMY_TTL 1

#include "gamestate.h"
#include "ships.h"

#undef ENABLE_MAP_PING     // if you want the sonar sounds

#define FRAME_DELAY 0.033f   // timer will be set to this * 1,000,000 (33mS)
#define FPS         30       // ... which represents about 30 FPS.
#define NETWORK_TIMEOUT 450  // number of ticks to timeout (15 seconds)

#define COLOR_ENEMY  Red
#define COLOR_PLAYER HTML2COLOR(0xeeeeee) // light grey

/* single player on this badge */
static ENTITY me;
static ENTITY bullet[MAX_BULLETS];
static VECTOR bullet_pending = {0,0};

static ENEMY *last_near = NULL;
static ENEMY *current_enemy = NULL;

// if true, we initiated combat.
static bool started_it = false;

// time left in combat
static uint8_t combat_time_left = 0;

OrchardAppContext *mycontext;

extern mutex_t peer_mutex;

/* private memory */
typedef struct {
  GListener	gl;
  font_t	fontLG;
  font_t	fontXS;
  font_t	fontSM;
  GHandle	ghTitleL;
  GHandle	ghTitleR;
  GHandle	ghACCEPT;
  GHandle	ghDECLINE;

  // these are the top row, player stats frame buffers.
  pixel_t       *l_pxbuf;
  pixel_t       *c_pxbuf;
  pixel_t       *r_pxbuf;
} battle_ui_t;

// enemies -- linked list
gll_t *enemies = NULL;

/* state tracking */
static battle_state current_battle_state = NONE;
static int16_t animtick = 0;

/* prototypes */
static void changeState(battle_state nextstate);
static ENEMY *enemy_find_by_peer(uint8_t *addr);

static
int getMapTile(ENTITY *e) {
  // we have to cast to int or math will be very wrong
  int x = (int)e->vecPosition.x;
  int y = (int)e->vecPosition.y;

  return ( (( y / TILE_H ) * 4) + ( x / TILE_W ));
}

static void zoomEntity(ENTITY *e) {
  /* if you are at some x,y on the world map and we zoom in to a sub-map
   * translate that coordinate to the new map
   *
   * Your point on the world map could represent any of NxM pixels in the
   * submap.
   */

  float scalechange = 0.25;
  float offsetX = -(e->vecPosition.x * scalechange);
  float offsetY = -(e->vecPosition.y * scalechange);

  e->vecPosition.x += offsetX;
  e->vecPosition.y += offsetY;
}

static void
entity_init(ENTITY *p) {
  p->visible = false;
  p->blinking = false;
  p->ttl = -1;

  p->vecVelocity.x = 0 ;
  p->vecVelocity.y = 0 ;

  p->vecVelocityGoal.x = 0;
  p->vecVelocityGoal.y = 0;

  // we'll set this to 0,0 for now. The caller should immediately set this.
  p->vecPosition.x = 0;
  p->vecPosition.y = 0;

  // this is "ocean drag"
  p->vecGravity.x = VDRAG;
  p->vecGravity.y = VDRAG;
  p->pix_inited = false;
}

/* approach lets us interpolate smoothly between positions */
static float
approach(float flGoal, float flCurrent, float dt) {
  float flDifference = flGoal - flCurrent;

  if (flDifference > dt)
    return flCurrent + dt;

  if (flDifference < -dt)
    return flCurrent - dt;

  return flGoal;
}

static bool
check_land_collision(ENTITY *p) {
  /* linear search for a collision with land.
   *
   * this is somewhat inefficient, we should be checking edges of pixmap
   * not interior. Edges are 40 pixels, interior is 100 px. maybe we don't
   * care at such a small size.
   */

  // 0x1f28 or 0x3f20 is blue water in rgb565 land. There's some random
  // variation here maybe due to coloring?
  for (int i = 0; i < FB_X * FB_Y; i++) {
    if (p->pix_old[i] != 0x1f28 &&
        p->pix_old[i] != 0x3f20 &&
        p->pix_old[i] != 0xffff) {
      // collide
      p->vecVelocity.x = 0;
      p->vecVelocity.y = 0;
      p->vecVelocityGoal.x = 0;
      p->vecVelocityGoal.y = 0;
      p->vecPosition.x = p->prevPos.x;
      p->vecPosition.y = p->prevPos.y;

      return true;
    }
  }

  return false;
}

static void
entity_update(ENTITY *p, float dt) {

  if (! p->pix_inited) {
    getPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, p->pix_old);
    p->pix_inited = true;
  }

  if ((p->ttl > -1) && (p->visible)) {
    p->ttl--;
    if (p->ttl == 0) {
      p->visible = false;
      i2sPlay("game/splash.snd");
    }
  }

  // store our position
  p->prevPos.x = p->vecPosition.x;
  p->prevPos.y = p->vecPosition.y;

  p->vecVelocity.x = approach(p->vecVelocityGoal.x,
                              p->vecVelocity.x,
                              dt * VAPPROACH);

  p->vecVelocity.y = approach(p->vecVelocityGoal.y,
                              p->vecVelocity.y,
                              dt * VAPPROACH);

  p->vecPosition.x = p->vecPosition.x + p->vecVelocity.x * dt * VMULT;
  p->vecPosition.y = p->vecPosition.y + p->vecVelocity.y * dt * VMULT;

  p->vecVelocity.x = p->vecVelocity.x + p->vecGravity.x * dt;
  p->vecVelocity.y = p->vecVelocity.y + p->vecGravity.y * dt;

  if (p->prevPos.x != p->vecPosition.x ||
      p->prevPos.y != p->vecPosition.y) {
      // erase the old position
      putPixelBlock (p->prevPos.x, p->prevPos.y, FB_X, FB_Y, p->pix_old);

      // check for collision with land
      getPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, p->pix_old);
      if (check_land_collision(p)) {
            getPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, p->pix_old);
      };
  }
}

static void
player_render(ENTITY *p) {
  userconfig *config = getConfig();

  /* Draw ship */
  gdispFillArea (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, COLOR_PLAYER);

  /*
   * Broadcast our location.
   * Ideally we should only do this when we know we've changed location,
   * however this seems as good a place as any to capture location changes
   * for now.
   */

  if (current_battle_state != COMBAT) {
    bleGapUpdateState ((uint16_t)p->vecPosition.x,
                       (uint16_t)p->vecPosition.y,
                       config->xp,
                       config->level,
                       config->in_combat);
  }

  return;
}

static void
bullets_render(void) {
  for (int i=0; i < MAX_BULLETS; i++) {
    if (bullet[i].visible == true)
      gdispFillCircle (bullet[i].vecPosition.x+(FB_X/2), bullet[i].vecPosition.y+(FB_X/2), FB_X/2-1, Black);
  }
}

static void
enemy_clear_blink(void *e) {
  ENEMY *en = e;
  en->e.blinking = false;
}

static void
enemy_clearall_blink(void) {
  // @brief clear enemy blink state
  gll_each(enemies, enemy_clear_blink);
}

static ENEMY *enemy_find_by_peer(uint8_t *addr) {
  gll_node_t *currNode = enemies->first;

  while(currNode != NULL) {
    ENEMY *e = (ENEMY *)currNode->data;

    if (memcmp(&(e->ble_peer_addr.addr), addr, 6) == 0) {
      return e;
    }

    currNode = currNode->next;
  }

  return NULL;
}

static int enemy_find_ll_pos(ble_gap_addr_t *ble_peer_addr) {
  gll_node_t *currNode = enemies->first;
  int i = 0;

  while(currNode != NULL) {
    ENEMY *e = (ENEMY *)currNode->data;

    if (memcmp(&(e->ble_peer_addr), ble_peer_addr, 6) == 0) {
      return i;
    }

    i++;
    currNode = currNode->next;
  }

  return -1;
}

static void enemy_list_refresh(void) {
  ENEMY *e;
  ble_peer_entry * p;

  // copy enemies from BLE
	osalMutexLock (&peer_mutex);

  for (int i = 0; i < BLE_PEER_LIST_SIZE; i++) {
    p = &ble_peer_list[i];

    if (p->ble_used == 0)
      continue;

    if (p->ble_isbadge == TRUE) {
      /* this is one of us, copy data over. */

      /* have we seen this address ? */
      /* addresses are 48 bit / 6 bytes */
      if ((e = enemy_find_by_peer(&p->ble_peer_addr[0])) != NULL) {
#ifdef DEBUG_ENEMY_DISCOVERY
        printf("enemy: found old enemy %x:%x:%x:%x:%x:%x (TTL %d)\n",
          p->ble_peer_addr[5],
          p->ble_peer_addr[4],
          p->ble_peer_addr[3],
          p->ble_peer_addr[2],
          p->ble_peer_addr[1],
          p->ble_peer_addr[0],
          p->ble_ttl);
#endif
        // is this dude in combat? Get rid of him.
        if (p->ble_game_state.ble_ides_incombat) {
          e->ttl = 0;
        }
        /* yes, update state */
        e->ttl = p->ble_ttl;

        // we expire 3 seconds earlier than the ble peer checker...
        if (e->ttl < 3) {
          // erase the old position
#ifdef DEBUG_ENEMY_TTL
          printf("remove enemy due to ttl was: %d \n",e->ttl);
#endif
          putPixelBlock (e->e.prevPos.x, e->e.prevPos.y, FB_X, FB_Y, e->e.pix_old);
          printf("%d\n", enemy_find_ll_pos((ble_gap_addr_t *)&p->ble_peer_addr));
          gll_remove(enemies, enemy_find_ll_pos((ble_gap_addr_t *)&p->ble_peer_addr));
          continue;
        }

        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        e->xp = p->ble_game_state.ble_ides_xp;

        if (e->e.prevPos.x != e->e.vecPosition.x ||
            e->e.prevPos.y != e->e.vecPosition.y) {
            // erase the old position
            putPixelBlock (e->e.prevPos.x, e->e.prevPos.y, FB_X, FB_Y, e->e.pix_old);
            // grab the new data
            getPixelBlock (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, e->e.pix_old);
            /* Draw ship */
            gdispFillArea (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, COLOR_ENEMY);
            // update
            e->e.prevPos.x = e->e.vecPosition.x;
            e->e.prevPos.y = e->e.vecPosition.y;
        }

      } else {
        /* no, add him */
        if (p->ble_game_state.ble_ides_incombat) {
#ifdef DEBUG_ENEMY_DISCOVERY
          printf("enemy: ignoring in-combat %x:%x:%x:%x:%x:%x\n",
            e->ble_peer_addr.addr[5],
            e->ble_peer_addr.addr[4],
            e->ble_peer_addr.addr[3],
            e->ble_peer_addr.addr[2],
            e->ble_peer_addr.addr[1],
            e->ble_peer_addr.addr[0]);
#endif
          continue;
        }
        e = malloc(sizeof(ENEMY));
        entity_init(&(e->e));
        e->e.visible = true;
        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;
        e->e.prevPos.x = e->e.vecPosition.x;
        e->e.prevPos.y = e->e.vecPosition.y;

        /* load the frame buffer */
        getPixelBlock (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, e->e.pix_old);
        /* Draw ship */
        gdispFillArea (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, COLOR_ENEMY);

        e->xp = p->ble_game_state.ble_ides_xp;
        memcpy(&e->ble_peer_addr.addr, p->ble_peer_addr, 6);

        e->ble_peer_addr.addr_id_peer = TRUE;
        e->ble_peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

        strcpy(e->name, (char *)p->ble_peer_name);
#ifdef DEBUG_ENEMY_DISCOVERY
        printf("enemy: found new enemy (ic=%d) %x:%x:%x:%x:%x:%x\n",
          p->ble_game_state.ble_ides_incombat,
          e->ble_peer_addr.addr[5],
          e->ble_peer_addr.addr[4],
          e->ble_peer_addr.addr[3],
          e->ble_peer_addr.addr[2],
          e->ble_peer_addr.addr[1],
          e->ble_peer_addr.addr[0]);
#endif
        gll_push(enemies, e);
      }
    }
  }

	osalMutexUnlock (&peer_mutex);
}

static uint32_t
battle_init(OrchardAppContext *context)
{

  // don't allocate any stack space
  return (0);
}

static void draw_world_map(void) {
  const userconfig *config = getConfig();
  GWidgetInit wi;
  char tmp[40];
  battle_ui_t *bh;

  // get private memory
  bh = (battle_ui_t *)mycontext->priv;

  // draw map
  putImageFile("game/world.rgb", 0,0);

  // Draw UI
  gwinWidgetClearInit (&wi);

  /* Create label widget: ghTitleL */
  wi.g.show = TRUE;
  wi.g.x = 0;
  wi.g.y = 0;
  wi.g.width = 160;
  wi.g.height = gdispGetFontMetric(bh->fontSM, fontHeight) + 2;
  wi.text = config->name;
  wi.customDraw = gwinLabelDrawJustifiedLeft;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleStyle;
  bh->ghTitleL = gwinLabelCreate(0, &wi);
  gwinLabelSetBorder (bh->ghTitleL, FALSE);
  gwinSetFont (bh->ghTitleL, bh->fontSM);
  gwinRedraw (bh->ghTitleL);

  /* Create label widget: ghTitleR */
  strcpy(tmp, "Find an enemy!");
  wi.g.show = TRUE;
  wi.g.x = 150;
  wi.g.width = 170;
  wi.text = tmp;
  wi.customDraw = gwinLabelDrawJustifiedRight;
  bh->ghTitleR = gwinLabelCreate (0, &wi);
  gwinSetFont (bh->ghTitleR, bh->fontSM);
  gwinLabelSetBorder (bh->ghTitleR, FALSE);
  gwinRedraw (bh->ghTitleR);

}

static void
battle_start (OrchardAppContext *context)
{
  userconfig *config = getConfig();
  battle_ui_t *bh;
  bh = malloc(sizeof(battle_ui_t));
  memset(bh, 0, sizeof(battle_ui_t));

  mycontext = context;

  entity_init(&me);
  me.vecPosition.x = config->last_x;
  me.vecPosition.y = config->last_y;
  me.visible = true;

  for (int i=0; i < MAX_BULLETS; i++) {
    entity_init(&bullet[i]);
  }

  // load the enemy list
  enemies = gll_init();

  // turn off the LEDs
  ledSetPattern(LED_PATTERN_WORLDMAP);

  context->priv = bh;
  bh->fontXS = gdispOpenFont (FONT_XS);
  bh->fontLG = gdispOpenFont (FONT_LG);
  bh->fontSM = gdispOpenFont (FONT_SM);

  // gtfo if in airplane mode.
  if (config->airplane_mode) {
    screen_alert_draw(true, "TURN OFF AIRPLANE MODE!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppRun(orchardAppByName("Badge"));
    return;
  }

  /* start the timer - 30 fps */
  orchardAppTimer(context, FRAME_DELAY * 1000000, true);

  /* stand up ugfx */
  geventListenerInit (&bh->gl);
  gwinAttachListener (&bh->gl);
  geventRegisterCallback (&bh->gl, orchardAppUgfxCallback, &bh->gl);

  changeState(WORLD_MAP);
  return;
}

ENEMY *getNearestEnemy(void) {
  // given my current position? are there any enemies near me?
  gll_node_t *currNode = enemies->first;

  while(currNode != NULL) {
    ENEMY *e = (ENEMY *)currNode->data;

    // does not account for screen edges -- but might be ok?
    // also does not attempt to sort the enemy list. if two people in same place
    // we'll have an issue.
    if ((e->e.vecPosition.x >= (me.vecPosition.x - (ENGAGE_BB/2))) &&
        (e->e.vecPosition.x <= (me.vecPosition.x + (ENGAGE_BB/2))) &&
        (e->e.vecPosition.y >= (me.vecPosition.y - (ENGAGE_BB/2))) &&
        (e->e.vecPosition.y <= (me.vecPosition.y + (ENGAGE_BB/2)))) {
      return e;
    }
    currNode = currNode->next;
  }
  return NULL;
}

static void
draw_hud(OrchardAppContext *context) {
  userconfig *config = getConfig();
  battle_ui_t *bh;

  // get private memory
  bh = (battle_ui_t *)context->priv;

  // top bar
  drawBufferedStringBox (&bh->l_pxbuf,
    0,
    0,
    129,
    gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
    config->name,
    bh->fontSM,
    White,
    justifyLeft);

  drawBufferedStringBox (&bh->r_pxbuf,
    190,
    0,
    130,
    gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
    current_enemy->name,
    bh->fontSM,
    White,
    justifyRight);
}

static ENEMY *
enemy_engage(OrchardAppContext *context) {

  ENEMY * e;

  e = getNearestEnemy();

  if (e == NULL) {
    // play an error sound because no one is near by
    i2sPlay("game/error.snd");
    return NULL;
  }

  return e;
}

static void
fire_bullet(ENTITY *from, VECTOR *bullet_pending) {
  for (int i=0; i < MAX_BULLETS; i++) {
    // find the first available bullet structure
    if (bullet[i].visible == false) {
      entity_init(&bullet[i]);
      bullet[i].visible = true;
      bullet[i].ttl = 30; // about a second
      // start the bullet from player position
      bullet[i].vecPosition.x = from->vecPosition.x;
      bullet[i].vecPosition.y = from->vecPosition.y;
      // copy the frame buffer from the parent.
      memcpy(&bullet[i].pix_old, from->pix_old, sizeof(from->pix_old));
      bullet[i].vecVelocityGoal.x = bullet_pending->x * 50;
      bullet[i].vecVelocityGoal.y = bullet_pending->y * 50;
      bullet_pending->x = bullet_pending->y = 0;
      i2sPlay("game/shot.snd");
      return;
    }
  }
}

bool
entity_OOB(ENTITY *e) {
  // returns true if entity is out of bounds
  if ( (e->vecPosition.x >= SCREEN_W - FB_X) ||
       (e->vecPosition.x < 0) ||
       (e->vecPosition.y < 0) ||
       (e->vecPosition.y >= SCREEN_H - FB_Y))
    return true;

  return false;
}

static void
battle_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
  (void) context;
  uint8_t i;
  ENEMY *nearest;
  battle_ui_t *bh;
  GEvent * pe;
  GEventGWinButton * be;

  OrchardAppRadioEvent * radio;
  ble_evt_t * evt;
  ble_gatts_evt_rw_authorize_request_t * rw;
  ble_gatts_evt_write_t * req;

  char msg[32];
  char tmp[40];
  bh = (battle_ui_t *) context->priv;

  /* MAIN GAME EVENT LOOP */
  if (event->type == timerEvent) {

    // handle tick for current state
    animtick++;
    if (battle_funcs[current_battle_state].tick != NULL) // some states lack a tick call
      battle_funcs[current_battle_state].tick();

    // player and enemy update
    if (current_battle_state == COMBAT || current_battle_state == WORLD_MAP) {
      if (animtick % (FPS/2) == 0) {
        // refresh world map enemy positions every 500mS
        enemy_list_refresh();
      }

      // move player
      entity_update(&me, FRAME_DELAY);
      // check for collison with enemies
      player_render(&me);

      // bullets
      for (i=0; i < MAX_BULLETS; i++) {
        if (bullet[i].visible == true) {
          entity_update(&bullet[i], FRAME_DELAY);
          if (entity_OOB(&bullet[i])) {
            // out of bounds, remove.
            bullet[i].visible = false;
          }

          // do we have an impact?
          // ...

          getPixelBlock(bullet[i].vecPosition.x, bullet[i].vecPosition.y, FB_X, FB_Y, bullet[i].pix_old);
        }
      }

      bullets_render();
      // render enemies
      if (current_battle_state == WORLD_MAP) {
        enemy_clearall_blink();
        nearest = getNearestEnemy();

        if (nearest != NULL)
          nearest->e.blinking = true;

        if (nearest != last_near) {
          last_near = nearest;

          // if our state has changed, update the UI
          if (nearest == NULL) {
            gwinSetText(bh->ghTitleR, "Find an enemy!", TRUE);
            gwinRedraw(bh->ghTitleR);
          } else {
            // update UI
            strcpy(tmp, "Engage: ");
            strcat(tmp, nearest->name);
            gwinSetText(bh->ghTitleR, tmp, TRUE);
            gwinRedraw(bh->ghTitleR);
          }
        }
      }
    }
  }

  // handle uGFX events
  if (event->type == ugfxEvent) {
    if (current_battle_state == APPROVAL_DEMAND) {
  		pe = event->ugfx.pEvent;
  		if (pe->type == GEVENT_GWIN_BUTTON) {
  			be = (GEventGWinButton *)pe;
  			if (be->gwin == bh->ghACCEPT) {
          // we will send an accept packet
          msg[0] = BLE_IDES_GAMEATTACK_ACCEPT;
  				bleGattcWrite (gm_handle.value_handle,
  				    (uint8_t *)msg, 1, FALSE);

          // on return of that packet, we'll have a l2CapConnection
          // and we'll switch to combat state.
  			}

  			if (be->gwin == bh->ghDECLINE) {
          // send a decline and go back to the map.
          msg[0] = BLE_IDES_GAMEATTACK_DECLINE;
  				bleGattcWrite (gm_handle.value_handle,
  				     (uint8_t *)msg, 1, FALSE);
  				bleGapDisconnect ();
          changeState(WORLD_MAP);
  			}
      }
		}
  }

  // deal with the radio
  if (event->type == radioEvent) {
    radio = (OrchardAppRadioEvent *)&event->radio;
    evt = &radio->evt;

    // ble notes
    // ble_gap_role (ble_lld.h) indicates where the connection came from.
    // It can be BLE_GAP_ROLE_CENTRAL or BLE_GAP_ROLE_PERIPH.
    // CENTRAL is the one that initiates the connection.
    // PERIPH is the one that accepts.
  	switch (radio->type) {
      case connectEvent:
        if (current_battle_state == APPROVAL_WAIT) {
          // fires when we succeed on a connect i think.
          screen_alert_draw (FALSE, "Sending Challenge...");
          msg[0] = BLE_IDES_GAMEATTACK_CHALLENGE;
          bleGattcWrite (gm_handle.value_handle,
              (uint8_t *)msg, 1, FALSE);
        }
        break;
      case gattsReadWriteAuthEvent:
        rw =
         &evt->evt.gatts_evt.params.authorize_request;
        req = &rw->request.write;
        if (rw->request.write.handle ==
            gm_handle.value_handle &&
            req->data[0] == BLE_IDES_GAMEATTACK_ACCEPT) {
          screen_alert_draw (FALSE,
              "Battle Accepted!");
          /* Offer accepted - create L2CAP link */
          /* the attacker makes the connect first */
          bleL2CapConnect (BLE_IDES_BATTLE_PSM);
        }

        if (rw->request.write.handle ==
            gm_handle.value_handle &&
            req->data[0] == BLE_IDES_GAMEATTACK_DECLINE) {
          screen_alert_draw (FALSE,
              "Battle Declined.");
          chThdSleepMilliseconds (2000);
          bleGapDisconnect ();
          if (current_battle_state == WORLD_MAP) {
            draw_world_map();
          }
          changeState(WORLD_MAP);
        }

        // we under attack, yo.
        if (rw->request.write.handle ==
            gm_handle.value_handle &&
            req->data[0] == BLE_IDES_GAMEATTACK_CHALLENGE) {
            changeState(APPROVAL_DEMAND);
        }
        break;
      case l2capRxEvent:
        // we then get a radio event
        // MTU is 1024 bytes.
        // radio->pkt and radio->pktlen
        break;
      case l2capTxEvent:
        // after transmit... they got the data.
        break;
      case l2capConnectEvent:
        // now we have a peer connection.
        // we send with bleL2CapSend(data, size) == NRF_SUCCESS ...
        printf("BATTLE: l2CapConnection Success\n");
        if (current_enemy == NULL) {
          // copy his data over and start the battle.
          current_enemy = enemy_find_by_peer(&ble_peer_addr.addr[0]);
        }
        changeState(COMBAT);
        break;
      default:
        break;
      }
  }

  // we cheat a bit here and read pins directly to see if
  // the joystick is in a corner.
  if (current_battle_state == COMBAT) {
    if (event->type == keyEvent) {
      if (event->key.flags == keyPress) {
        // we need to figure out if the key is still held
        // and if so, we will fire a bullet.
        if (bullet_pending.x == 0 && bullet_pending.y == 0) {
          if (palReadPad (BUTTON_B_UP_PORT, BUTTON_B_UP_PIN) == 0)
            bullet_pending.y = -1;

          if (palReadPad (BUTTON_B_DOWN_PORT, BUTTON_B_DOWN_PIN) == 0)
            bullet_pending.y = 1;

          if (palReadPad (BUTTON_B_LEFT_PORT, BUTTON_B_LEFT_PIN) == 0)
            bullet_pending.x = -1;

          if (palReadPad (BUTTON_B_RIGHT_PORT, BUTTON_B_RIGHT_PIN) == 0)
            bullet_pending.x = 1;
        }
      }
    }
  }

  // movement works in both modes
  if (current_battle_state == WORLD_MAP || current_battle_state == COMBAT)
    if (event->type == keyEvent) {
      if (event->key.flags == keyPress)  {
        switch (event->key.code) {
        case keyALeft:
          me.vecVelocityGoal.x = -VGOAL;
          break;
        case keyARight:
          me.vecVelocityGoal.x = VGOAL;
          break;
        case keyAUp:
          me.vecVelocityGoal.y = -VGOAL;
          break;
        case keyADown:
          me.vecVelocityGoal.y = VGOAL;
          break;
        case keyASelect:
          if (current_battle_state == WORLD_MAP) {
            i2sPlay ("sound/click.snd");
            orchardAppExit ();
          }
          break;
        case keyBSelect:
          if (current_battle_state != COMBAT) {
            if ((current_enemy = enemy_engage(context)) != NULL) {
              // outgoing challenge
              started_it = true;
              changeState(APPROVAL_WAIT);
            }
          }
          break;
        default:
          break;
        }
      }

      if (event->key.flags == keyRelease) {
        switch (event->key.code) {
        case keyALeft:
          me.vecVelocityGoal.x = 0;
          break;
        case keyARight:
          me.vecVelocityGoal.x = 0;
          break;
        case keyAUp:
          me.vecVelocityGoal.y = 0;
          break;
        case keyADown:
          me.vecVelocityGoal.y = 0;
          break;
        default:
          break;
        }
      }
    }

  return;
}


static void changeState(battle_state nextstate) {
  // if we need to switch states, do so.
  if (nextstate == current_battle_state) {
    // do nothing.
#ifdef DEBUG_BATTLE_STATE
    printf("BATTLE: ignoring state change request, as we are already in state %d: %s...\r\n", nextstate, battle_state_name[nextstate]);
#endif
    return;
  }

#ifdef DEBUG_BATTLE_STATE
  printf("BATTLE: moving to state %d: %s...\r\n", nextstate, battle_state_name[nextstate]);
#endif

  // exit the old state and reset counters
  if (battle_funcs[current_battle_state].exit != NULL) {
    battle_funcs[current_battle_state].exit();
  }

  animtick = 0;
  current_battle_state = nextstate;

  // enter the new state
  if (battle_funcs[current_battle_state].enter != NULL) {
    battle_funcs[current_battle_state].enter();
  }
}

static void free_enemy(void *e) {
  ENEMY *en = e;
  free(en);
}

static void battle_exit(OrchardAppContext *context)
{
  userconfig *config = getConfig();
  battle_ui_t *bh;

  // free the UI
  bh = (battle_ui_t *)context->priv;
  // store the last x/y of this guy
  config->last_x = me.vecPosition.x;
  config->last_y = me.vecPosition.y;
  configSave(config);

  // reset this state or we won't init properly next time.
  changeState(NONE);

  // free the enemy list
  if (enemies) {
    gll_each(enemies, free_enemy);
    gll_destroy(enemies);
    free(enemies);
  }

  if (bh->fontXS)
    gdispCloseFont (bh->fontXS);

  if (bh->fontLG)
    gdispCloseFont (bh->fontLG);

  if (bh->fontSM)
    gdispCloseFont (bh->fontSM);

  if (bh->ghTitleL)
    gwinDestroy (bh->ghTitleL);

  if (bh->ghTitleR)
    gwinDestroy (bh->ghTitleR);

  geventRegisterCallback (&bh->gl, NULL, NULL);
  geventDetachSource (&bh->gl, NULL);

  free (context->priv);
  context->priv = NULL;

  // restore the LED pattern from config
  ledSetPattern(config->led_pattern);
  return;
}


static void
render_enemy(void *e) {
  ENEMY *en = e;
  gdispFillArea (en->e.vecPosition.x, en->e.vecPosition.y, FB_X, FB_Y, COLOR_ENEMY);
}

static void
render_all_enemies(void) {
  // @brief clear enemy blink state
  gll_each(enemies, render_enemy);
}

/* state change and tick functions go here ----------------------------------- */

// WORLDMAP ------------------------------------------------------------------
void state_worldmap_enter(void) {
  current_enemy = NULL;

  gdispClear(Black);
  draw_world_map();
  render_all_enemies();
}

void state_worldmap_tick(void) {
#ifdef ENABLE_MAP_PING
  /* every four seconds (120 frames) go ping */
  if (animtick % (FPS*4) == 0) {
    i2sPlay("game/map_ping.snd");
  }
#endif
}

void state_worldmap_exit(void) {
  battle_ui_t *bh;

  // get private memory
  bh = (battle_ui_t *)mycontext->priv;

  if (bh->ghTitleL) {
    gwinDestroy (bh->ghTitleL);
    bh->ghTitleL= NULL;
  }

  if (bh->ghTitleR) {
    gwinDestroy (bh->ghTitleR);
    bh->ghTitleR = NULL;
  }
}

// APPROVAL_WAIT -------------------------------------------------------------
// we are attacking someone, wait for consent.
void state_approval_wait_enter(void) {
  // we will now attempt to open a BLE gap connecto to the user.
  started_it = true;
  printf("engage: %x:%x:%x:%x:%x:%x\n",
    current_enemy->ble_peer_addr.addr[5],
    current_enemy->ble_peer_addr.addr[4],
    current_enemy->ble_peer_addr.addr[3],
    current_enemy->ble_peer_addr.addr[2],
    current_enemy->ble_peer_addr.addr[1],
    current_enemy->ble_peer_addr.addr[0]);
  screen_alert_draw (FALSE, "Connecting...");

  bleGapConnect (&current_enemy->ble_peer_addr);
  // when the gap connection is accepted, we will attempt a
  // l2cap connect

  animtick = 0;
}

void state_approval_wait_tick(void) {
  // we shouldn't be in this state for more than five seconds or so.
  if (animtick > NETWORK_TIMEOUT) {
    // forget it.
    screen_alert_draw(TRUE, "TIMED OUT");
    chThdSleepMilliseconds(ALERT_DELAY);
    changeState(WORLD_MAP);
  }
}

void state_approval_wait_exit(void) {
  bleGapDisconnect ();
}

// APPOVAL_DEMAND ------------------------------------------------------------
// Someone is attacking us.
void state_approval_demand_enter(void) {
  battle_ui_t *bh;
	ble_peer_entry * peer;
  char buf[36];
	int fHeight;
	GWidgetInit wi;

  animtick = 0;
  bh = (battle_ui_t *)mycontext->priv;
  gdispClear(Black);
  printf("entered approval demand 3\n");
  peer = blePeerFind (ble_peer_addr.addr);
  if (peer == NULL)
		snprintf (buf, 36, "Badge %X:%X:%X:%X:%X:%X",
		    ble_peer_addr.addr[5],  ble_peer_addr.addr[4],
		    ble_peer_addr.addr[3],  ble_peer_addr.addr[2],
		    ble_peer_addr.addr[1],  ble_peer_addr.addr[0]);
	else
		snprintf (buf, 36, "%s", peer->ble_peer_name);
  printf("entered approvald demand 4\n");
  putImageFile ("images/undrattk.rgb", 0, 0);
  i2sPlay("sound/klaxon.snd");

  fHeight = gdispGetFontMetric (bh->fontSM, fontHeight);
  printf("approval demand5\n");

  gdispDrawStringBox (0, 50 -
    fHeight,
    gdispGetWidth(), fHeight,
    buf, bh->fontSM, White, justifyCenter);

  gdispDrawStringBox (0, 50, gdispGetWidth(), fHeight,
      "IS CHALLENGING YOU", bh->fontSM, White, justifyCenter);

  gwinSetDefaultStyle (&RedButtonStyle, FALSE);
  gwinSetDefaultFont (bh->fontSM);
  gwinWidgetClearInit (&wi);

  wi.g.show = TRUE;
  wi.g.x = 0;
  wi.g.y = 210;
  wi.g.width = 150;
  wi.g.height = 30;
  wi.text = "DECLINE";
  bh->ghDECLINE = gwinButtonCreate(0, &wi);

  wi.g.x = 170;
  wi.text = "ACCEPT";
  bh->ghACCEPT = gwinButtonCreate(0, &wi);

  gwinSetDefaultStyle (&BlackWidgetStyle, FALSE);

}

void state_approval_demand_tick(void) {
  if (animtick > NETWORK_TIMEOUT) {
    // user chose nothing.
    screen_alert_draw(TRUE, "TIMED OUT");
    chThdSleepMilliseconds(ALERT_DELAY);
    changeState(WORLD_MAP);
  }
}

void state_approval_demand_exit(void) {
  battle_ui_t *bh;
  bh = mycontext->priv;

  gwinDestroy (bh->ghACCEPT);
  gwinDestroy (bh->ghDECLINE);

  geventDetachSource (&bh->gl, NULL);
  geventRegisterCallback (&bh->gl, NULL, NULL);

  bleGapDisconnect ();
}

// COMBAT --------------------------------------------------------------------

void state_combat_enter(void) {
  char fnbuf[30];
  int newmap;
  userconfig *config = getConfig();

  // we're in combat now, send our last advertisement.
  combat_time_left = 60;

  config->in_combat = 1;
  configSave(config);

  bleGapUpdateState ((uint16_t)me.vecPosition.x,
                     (uint16_t)me.vecPosition.y,
                     config->xp,
                     config->level,
                     config->in_combat);

  newmap = getMapTile(&me);
  sprintf(fnbuf, "game/map-%02d.rgb", newmap);
  putImageFile(fnbuf, 0,0);
  zoomEntity(&me);

  draw_hud(mycontext);
  enemy_clearall_blink();
}


void state_combat_tick(void) {
  char tmp[10];
  battle_ui_t *bh;
  bh = mycontext->priv;

  if (animtick % FPS == 0) {
    combat_time_left--;
    sprintf(tmp, "00:%02d", combat_time_left);

    // update clock
    drawBufferedStringBox (&bh->c_pxbuf,
      130,
      0,
      60,
      gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
      tmp,
      bh->fontSM,
      Yellow,
      justifyCenter);

    if (combat_time_left == 0) {
      // time up.
      i2sPlay("sound/foghrn.snd");
      changeState(SHOW_RESULTS);
    }
  }
}

void state_combat_exit(void) {
  battle_ui_t *bh;
  bh = mycontext->priv;

  free(bh->l_pxbuf);
  bh->l_pxbuf = NULL;

  free(bh->c_pxbuf);
  bh->c_pxbuf = NULL;

  free(bh->r_pxbuf);
  bh->r_pxbuf = NULL;
}

// -------------------------------------------------------------------------

orchard_app("Sea Battle",
            "icons/ship.rgb",
            0,
            battle_init, battle_start,
            battle_event, battle_exit, 1);
