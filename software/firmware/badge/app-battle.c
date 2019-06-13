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
#include "i2s_lld.h"
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

#define DEBUG_BATTLE_STATE 1
#include "gamestate.h"

#undef ENABLE_MAP_PING     // if you want the sonar sounds

#define VGOAL       8        // ship accleration goal
#define VDRAG       -0.01f   // this is constant
#define VAPPROACH   12       // this is accel/decel rate
#define FRAME_DELAY 0.033f   // timer will be set to this * 1,000,000 (33mS)
#define FPS         30       // ... which represents about 30 FPS.
#define VMULT       8        // on each time step, take this many steps.
#define ENGAGE_BB   40       // If enemy is in this bounding box we can engage
#define MAX_BULLETS 3        // duh.

// size of sub-map tiles
#define TILE_W 80
#define TILE_H 60

/* single player on this badge */
static ENTITY me;
static ENTITY bullet[MAX_BULLETS];
static VECTOR bullet_pending = {0,0};

static ENEMY *last_near = NULL;
static ENEMY *current_enemy = NULL;

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

  // we're going to always start in the lower harbor for now.
  p->vecPosition.x = HARBOR_LWR_X;
  p->vecPosition.y = HARBOR_LWR_Y;

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
  for (int i = 0; i < FB_X * FB_Y; i++) {
    if ( (p->pix_old[i] <= 0x9e00 || p->pix_old[i] >= 0xbf09) && p->pix_old[i] != 0xffff) {
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

  p->vecPosition.x = p->vecPosition.x + p->vecVelocity.x * dt	* VMULT;
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
  gdispFillArea (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, Purple);

  /*
   * Broadcast our location.
   * Ideally we should only do this when we know we've changed location,
   * however this seems as good a place as any to capture location changes
   * for now. Note that XP and rank aren't supported yet so for the moment
   * they're hardcoded to 0.
   */

  bleGapUpdateState ((uint16_t)p->vecPosition.x,
    (uint16_t)p->vecPosition.y, config->xp, config->level);

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
        /*printf("enemy: found old enemy %x:%x:%x:%x:%x:%x\n",
          p->ble_peer_addr[5],
          p->ble_peer_addr[4],
          p->ble_peer_addr[3],
          p->ble_peer_addr[2],
          p->ble_peer_addr[1],
          p->ble_peer_addr[0]);
        */
        // we expire 3 seconds earlier than the ble peer checker...
        if (e->ttl < 3) {
          // erase the old position
          putPixelBlock (e->e.prevPos.x, e->e.prevPos.y, FB_X, FB_Y, e->e.pix_old);

          gll_remove(enemies, enemy_find_ll_pos((ble_gap_addr_t *)&p->ble_peer_addr));
          continue;
        }

        /* yes, update state */
        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        e->xp = p->ble_game_state.ble_ides_xp;
        e->ttl = p->ble_ttl;

        if (e->e.prevPos.x != e->e.vecPosition.x ||
            e->e.prevPos.y != e->e.vecPosition.y) {
            // erase the old position
            putPixelBlock (e->e.prevPos.x, e->e.prevPos.y, FB_X, FB_Y, e->e.pix_old);
            // grab the new data
            getPixelBlock (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, e->e.pix_old);
            /* Draw ship */
            gdispFillArea (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, Red);
            // update
            e->e.prevPos.x = e->e.vecPosition.x;
            e->e.prevPos.y = e->e.vecPosition.y;
        }

      } else {
        /* no, add him */
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
        gdispFillArea (e->e.vecPosition.x, e->e.vecPosition.y, FB_X, FB_Y, Red);

        e->xp = p->ble_game_state.ble_ides_xp;
        memcpy(&e->ble_peer_addr.addr, p->ble_peer_addr, 6);

        e->ble_peer_addr.addr_id_peer = TRUE;
        e->ble_peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

        strcpy(e->name, (char *)p->ble_peer_name);
        /*
        printf("enemy: found new enemy %x:%x:%x:%x:%x:%x\n",
          e->ble_peer_addr.addr[5],
          e->ble_peer_addr.addr[4],
          e->ble_peer_addr.addr[3],
          e->ble_peer_addr.addr[2],
          e->ble_peer_addr.addr[1],
          e->ble_peer_addr.addr[0]);
        */
        gll_push(enemies, e);
      }
    }
  }

	osalMutexUnlock (&peer_mutex);
}

static uint32_t
battle_init(OrchardAppContext *context)
{
  userconfig *config = getConfig();
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
  gdispDrawStringBox (0, 0,
                      105, gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
                      config->name,
                      bh->fontSM,
                      White,
                      justifyLeft);


  gdispDrawStringBox (0, 0,
                      215, gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
                      "00:60",
                      bh->fontSM,
                      Yellow,
                      justifyCenter);


  gdispDrawStringBox (0, 0,
                      215, gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
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
	OrchardAppRadioEvent * radio;
	ble_evt_t * evt;
	ble_gatts_evt_rw_authorize_request_t * rw;
	ble_gatts_evt_write_t * req;
  char msg[32];
  char tmp[40];
  bh = (battle_ui_t *) context->priv;

  /* MAIN GAME EVENT LOOP */
  if (event->type == timerEvent) {
    animtick++;

    if (battle_funcs[current_battle_state].tick != NULL) // some states lack a tick call
      battle_funcs[current_battle_state].tick();

    // refresh world map enemy positions every 500mS
    if (animtick % 12 == 0) {
      enemy_list_refresh();
    }

    // player update
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

  // deal with the radio
  if (event->type == radioEvent) {
    radio = (OrchardAppRadioEvent *)&event->radio;
    evt = &radio->evt;

  	switch (radio->type) {
      case connectEvent:
        // fires when we succeed on a connect i think.
          screen_alert_draw (FALSE, "Sending Challenge...");
        msg[0] = BLE_IDES_GAMEATTACK_CHALLENGE;
        bleGattcWrite (gm_handle.value_handle,
            (uint8_t *)msg, 1, FALSE);
        break;
      case gattsReadWriteAuthEvent:
        rw =
         &evt->evt.gatts_evt.params.authorize_request;
        req = &rw->request.write;
        if (rw->request.write.handle ==
            ot_handle.value_handle &&
            req->data[0] == BLE_IDES_GAMEATTACK_ACCEPT) {
          screen_alert_draw (FALSE,
              "Battle Accepted!");
          /* Offer accepted - create L2CAP link*/
          //bleL2CapConnect (BLE_IDES_OTA_PSM);
        } else {
          screen_alert_draw (FALSE,
              "Battle Declined.");
          chThdSleepMilliseconds (2000);
          bleGapDisconnect ();
          changeState(WORLD_MAP);
        }
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
              changeState(APPROVAL_DEMAND);
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


static void battle_exit(OrchardAppContext *context)
{
  userconfig *config = getConfig();
  battle_ui_t *bh;

  // store the last x/y of this guy
  config->last_x = me.vecPosition.x;
  config->last_y = me.vecPosition.y;
  configSave(config);

  // reset this state or we won't init properly next time.
  changeState(NONE);

  // free the enemy list
  if (enemies)
    gll_destroy(enemies);

  // free the UI
  bh = (battle_ui_t *)context->priv;

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

#ifdef notyet
  /* NB: add back if/when ugfx touch panel events are supported */
  geventRegisterCallback (&bh->gl, NULL, NULL);
  geventDetachSource (&bh->gl, NULL);
#endif

  free (context->priv);
  context->priv = NULL;

  // restore the LED pattern from config
  ledSetPattern(config->led_pattern);
  return;
}

/* state change and tick functions go here ----------------------------------- */

// WORLDMAP ------------------------------------------------------------------
void state_worldmap_enter(void) {
  gdispClear(Black);
  draw_world_map();
}

void state_worldmap_tick(void) {
#ifdef ENABLE_MAP_PING
  /* every four seconds (120 frames) */
  if ((animtick % 120 == 0) && (current_battle_state == WORLD_MAP)) {
    i2sPlay("game/map_ping.snd");
  }
#endif
}

void state_worldmap_exit(void) {
}

// APPOVAL_DEMAND ------------------------------------------------------------
void state_approval_demand_enter(void) {
  printf("engage: %x:%x:%x:%x:%x:%x\n",
    current_enemy->ble_peer_addr.addr[5],
    current_enemy->ble_peer_addr.addr[4],
    current_enemy->ble_peer_addr.addr[3],
    current_enemy->ble_peer_addr.addr[2],
    current_enemy->ble_peer_addr.addr[1],
    current_enemy->ble_peer_addr.addr[0]);
  screen_alert_draw (FALSE, "Connecting...");

  // we will now attempt to open a BLE gap connecto to the user.
  bleGapConnect (&current_enemy->ble_peer_addr);
}

void state_approval_demand_tick(void) {
}

void state_approval_demand_exit(void) {
}

// COMBAT --------------------------------------------------------------------

void state_combat_enter(void) {
  char fnbuf[30];
  int newmap;

  newmap = getMapTile(&me);
  sprintf(fnbuf, "game/map-%02d.rgb", newmap);
  putImageFile(fnbuf, 0,0);
  zoomEntity(&me);


  draw_hud(mycontext);
  enemy_clearall_blink();
}


void state_combat_tick(void) {
}

void state_combat_exit(void) {
}

// -------------------------------------------------------------------------

orchard_app("Sea Battle", "icons/ship.rgb", 0, battle_init, battle_start,
    battle_event, battle_exit, 1);
