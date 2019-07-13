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
#include "ides_sprite.h"
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

#include "strutil.h"

// debugs the discovery process
#undef DEBUG_ENEMY_DISCOVERY
#define DEBUG_BATTLE_STATE 1
#undef DEBUG_ENEMY_TTL

#include "gamestate.h"
#include "ships.h"

#undef ENABLE_MAP_PING     // if you want the sonar sounds

#define FRAME_DELAY 0.033f   // timer will be set to this * 1,000,000 (33mS)
#define FPS         30       // ... which represents about 30 FPS.
#define NETWORK_TIMEOUT (FPS*10)  // number of ticks to timeout (10 seconds)

#define COLOR_ENEMY  Red
#define COLOR_PLAYER HTML2COLOR(0xeeeeee) // light grey

#define SHIP_SIZE_WORLDMAP 10
#define SHIP_SIZE_ZOOMED   40
#define BULLET_SIZE        10

/* the sprite system */
static ISPRITESYS *sprites = NULL;

/* single player on this badge */
static ENTITY bullet[MAX_BULLETS];
static VECTOR bullet_pending = {0,0};

static ENEMY *last_near = NULL;

// yes, yes, everyone is an enemy
// this way, the same code works for both users.
static ENEMY *player = NULL;
static ENEMY *current_enemy = NULL;

// time left in combat (or in various related states)
static uint8_t state_time_left = 0;

OrchardAppContext *mycontext;

extern mutex_t peer_mutex;

#define PEER_ADDR_LEN	12 + 5
#define MAX_PEERMEM	(PEER_ADDR_LEN + BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED + 1)

/* private memory */
typedef struct _BattleHandles {
  GListener	gl;
  font_t	fontLG;
  font_t	fontXS;
  font_t	fontSM;
  font_t  fontSTENCIL;
  GHandle	ghTitleL;
  GHandle	ghTitleR;
  GHandle	ghACCEPT;
  GHandle	ghDECLINE;

  // these are the top row, player stats frame buffers.
  pixel_t       *l_pxbuf;
  pixel_t       *c_pxbuf;
  pixel_t       *r_pxbuf;

  // misc variables
	uint16_t cid;  // l2capchannelid for combat
  char		 txbuf[MAX_PEERMEM + BLE_IDES_L2CAP_MTU + 3];
  char		 rxbuf[BLE_IDES_L2CAP_MTU];
} BattleHandles;

// enemies -- linked list
gll_t *enemies = NULL;

/* state tracking */
static battle_state current_battle_state = NONE;
static int16_t animtick = 0;

/* prototypes */
static void changeState(battle_state nextstate);
static ENEMY *enemy_find_by_peer(uint8_t *addr);
static void render_all_enemies(void);
static void send_ship_type(uint16_t type, bool final);
static void state_vs_draw_enemy(ENEMY *e, bool is_player);
/* end protos */

static uint16_t xp_for_level(uint8_t level) {
  // return the required amount of XP for a given level
  // level should be given starting with index 1
  uint16_t xp_req[] = {
  //1    2    3     4     5     6     7     8     9    10
    0, 400, 880, 1440, 2080, 2800, 3600, 4480, 5440, 6480
  };

  if ((level <= 10) && (level >= 1)) {
    return xp_req[level];
  } else {
    return 0;
  }

}

static
int getMapTile(ENTITY *e) {
  // we have to cast to int or math will be very wrong
  int x = (int)e->vecPosition.x;
  int y = (int)e->vecPosition.y;

  return ( (( y / TILE_H ) * 4) + ( x / TILE_W ));
}

static void
entity_init(ENTITY *p, int16_t size_x, int16_t size_y, entity_type t) {
  pixel_t *buf;
  color_t color = Black;

  p->type = t;
  p->visible = false;
  p->blinking = false;
  p->ttl = -1;

  p->size_x = size_x;
  p->size_y = size_y;

  p->vecVelocity.x = 0;
  p->vecVelocity.y = 0;

  p->vecVelocityGoal.x = 0;
  p->vecVelocityGoal.y = 0;

  // we'll set this to 0,0 for now. The caller should immediately set this.
  p->vecPosition.x = 0;
  p->vecPosition.y = 0;

  // this is "ocean drag"
  p->vecGravity.x = VDRAG;
  p->vecGravity.y = VDRAG;

  p->sprite_id = isp_make_sprite(sprites);

  switch(p->type) {
    case T_PLAYER:
      color = White;
      break;
    case T_ENEMY:
      color = Red;
      break;
    case T_BULLET:
      color = Black;
      break;
    case T_SPECIAL:
      color = Yellow;
      break;
  }

  buf = boxmaker(size_x, size_y, color);
  isp_set_sprite_block(sprites, p->sprite_id, size_x, size_y, buf);
  free(buf);
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
  return test_sprite_for_land_collision(sprites, p->sprite_id);
}

static void
entity_update(ENTITY *p, float dt) {

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
      // check for collision with land
      isp_set_sprite_xy(sprites,
                        p->sprite_id,
                        p->vecPosition.x,
                        p->vecPosition.y);

      if (check_land_collision(p)) {
        // TBD
        p->vecPosition.x = p->prevPos.x;
        p->vecPosition.y = p->prevPos.y;
        p->vecVelocity.x = 0;
        p->vecVelocity.y = 0;
        p->vecVelocityGoal.x = 0;
        p->vecVelocityGoal.y = 0;
        return;
      };

      isp_set_sprite_xy(sprites,
                        p->sprite_id,
                        p->vecPosition.x,
                        p->vecPosition.y);

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
  if (enemies) {
    gll_each(enemies, enemy_clear_blink);
  }
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

static void
expire_enemies(void *e) {
  ENEMY *en = e;
  int position;
  // we expire 3 seconds earlier than the ble peer checker...
  if (en->ttl < 3) {
    // erase the old position
#ifdef DEBUG_ENEMY_TTL
    printf("remove enemy due to ttl was: %d \n",en->ttl);
#endif
    isp_destroy_sprite(sprites, en->e.sprite_id);

    // remove it from the linked list.
    position = enemy_find_ll_pos(&(en->ble_peer_addr));
    if (position > -1) {
      gll_remove(enemies, position);
    }
  }
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
        } else {
          e->ttl = p->ble_ttl;
        }

        e->e.prevPos.x = e->e.vecPosition.x;
        e->e.prevPos.y = e->e.vecPosition.y;

        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        isp_set_sprite_xy(sprites,
          e->e.sprite_id,
          e->e.vecPosition.x,
          e->e.vecPosition.y);

          e->xp = p->ble_game_state.ble_ides_xp;
          e->level = p->ble_game_state.ble_ides_level;
          e->hp = shiptable[p->ble_game_state.ble_ides_ship_type].max_hp;
          e->energy = shiptable[p->ble_game_state.ble_ides_ship_type].max_energy;
          e->ship_type = p->ble_game_state.ble_ides_ship_type;
      } else {
        /* no, add him */
        if (p->ble_game_state.ble_ides_incombat) {
#ifdef DEBUG_ENEMY_DISCOVERY
          printf("enemy: ignoring in-combat %x:%x:%x:%x:%x:%x\n",
            p->ble_peer_addr[5],
            p->ble_peer_addr[4],
            p->ble_peer_addr[3],
            p->ble_peer_addr[2],
            p->ble_peer_addr[1],
            p->ble_peer_addr[0]);
#endif
          continue;
        }

#ifdef DEBUG_ENEMY_DISCOVERY
          printf("enemy: found new enemy (ic=%d) %x:%x:%x:%x:%x:%x\n",
            p->ble_game_state.ble_ides_incombat,
            p->ble_peer_addr[5],
            p->ble_peer_addr[4],
            p->ble_peer_addr[3],
            p->ble_peer_addr[2],
            p->ble_peer_addr[1],
            p->ble_peer_addr[0]);
#endif
        e = malloc(sizeof(ENEMY));

        if (current_battle_state == COMBAT) {
          entity_init(&(e->e),SHIP_SIZE_ZOOMED,SHIP_SIZE_ZOOMED, T_ENEMY);
        } else {
          entity_init(&(e->e),SHIP_SIZE_WORLDMAP,SHIP_SIZE_WORLDMAP, T_ENEMY);
        }

        e->e.visible = true;
        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        isp_set_sprite_xy(sprites,
          e->e.sprite_id,
          e->e.vecPosition.x,
          e->e.vecPosition.y);

        e->e.prevPos.x = p->ble_game_state.ble_ides_x;
        e->e.prevPos.y = p->ble_game_state.ble_ides_y;


        memcpy(&e->ble_peer_addr.addr, p->ble_peer_addr, 6);

        e->ble_peer_addr.addr_id_peer = TRUE;
        e->ble_peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

        // copy over game data
        strcpy(e->name, (char *)p->ble_peer_name);
        e->xp = p->ble_game_state.ble_ides_xp;
        e->level = p->ble_game_state.ble_ides_level;
        e->ship_type = p->ble_game_state.ble_ides_ship_type;
        e->ship_locked_in = FALSE;
        e->hp = shiptable[e->ship_type].max_hp;
        e->energy = shiptable[e->ship_type].max_energy;
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
  BattleHandles *bh;

  // get private memory
  bh = (BattleHandles *)mycontext->priv;

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
copy_config_to_player(void) {
  userconfig *config = getConfig();

  player = malloc(sizeof(ENEMY));
  // this is done once at startup. These are the "live" values used in game,
  // where as the config is the long-term/stored value.
  memset(&(player->ble_peer_addr), 0, sizeof(ble_gap_addr_t));
  strncpy(player->name, config->name, CONFIG_NAME_MAXLEN);
  player->level = config->level;

  // we cannot determine hp or xp until ship has been selected but we can
  // guess based on last used.
  player->hp = shiptable[config->current_ship].max_hp;
  player->energy = shiptable[config->current_ship].max_energy;
  player->xp = config->xp;
  player->ship_type = config->current_ship;
  player->ship_locked_in = FALSE;
  player->ttl = -1;

  // ENTITY player.e will be init'd during combat
}

static void
battle_start (OrchardAppContext *context)
{
  userconfig *config = getConfig();
  BattleHandles *bh;
  bh = malloc(sizeof(BattleHandles));
  memset(bh, 0, sizeof(BattleHandles));
  bh->cid = BLE_L2CAP_CID_INVALID;
  mycontext = context;

  // turn off the LEDs
  ledSetPattern(LED_PATTERN_WORLDMAP);

  context->priv = bh;
  bh->fontXS = gdispOpenFont (FONT_XS);
  bh->fontLG = gdispOpenFont (FONT_LG);
  bh->fontSM = gdispOpenFont (FONT_SM);
  bh->fontSTENCIL = gdispOpenFont (FONT_STENCIL);

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

  // stand us up.
  copy_config_to_player();

  // how did we get here?
  // if we have a bleGapConnection then we can
  // use data from the current peer, and we start in
  // combat instead of accept.

  // if we have a peer address, we're gonna start in a handshake state.
  if (ble_peer_addr.addr_type != 0x0) {
    // we're gonna sit on our hands for a minute and wait for the l2cap connect.
    changeState(HANDSHAKE);
  } else {
    changeState(WORLD_MAP);
  }
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
    if ((e->e.vecPosition.x >= (player->e.vecPosition.x - (ENGAGE_BB/2))) &&
        (e->e.vecPosition.x <= (player->e.vecPosition.x + (ENGAGE_BB/2))) &&
        (e->e.vecPosition.y >= (player->e.vecPosition.y - (ENGAGE_BB/2))) &&
        (e->e.vecPosition.y <= (player->e.vecPosition.y + (ENGAGE_BB/2)))) {
      return e;
    }
    currNode = currNode->next;
  }
  return NULL;
}

static void
draw_hud(OrchardAppContext *context) {
  userconfig *config = getConfig();
  BattleHandles *bh;

  // get private memory
  bh = (BattleHandles *)context->priv;

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

  // player's stats
  drawProgressBar(0,26,120,6,shiptable[player->ship_type].max_hp, player->hp, false, false);
  drawProgressBar(0,34,120,6,shiptable[player->ship_type].max_energy, player->energy, false, false);

  // enemy stats
  drawProgressBar(200,26,120,6,shiptable[current_enemy->ship_type].max_hp, current_enemy->hp, false, true);
  drawProgressBar(200,34,120,6,shiptable[current_enemy->ship_type].max_energy, current_enemy->energy, false, true);
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

#ifdef notyet
static void
fire_bullet(ENTITY *from, VECTOR *bullet_pending) {
  for (int i=0; i < MAX_BULLETS; i++) {
    // find the first available bullet structure
    if (bullet[i].visible == false) {
      // bug: we should not init this again
      entity_init(&bullet[i],BULLET_SIZE,BULLET_SIZE,T_BULLET);

      bullet[i].visible = true;
      bullet[i].ttl = 30; // about a second
      // start the bullet from player position
      bullet[i].vecPosition.x = from->vecPosition.x;
      bullet[i].vecPosition.y = from->vecPosition.y;
      // copy the frame buffer from the parent.
      bullet[i].vecVelocityGoal.x = bullet_pending->x * 50;
      bullet[i].vecVelocityGoal.y = bullet_pending->y * 50;
      bullet_pending->x = bullet_pending->y = 0;
      i2sPlay("game/shot.snd");
      return;
    }
  }
}
#endif

bool
entity_OOB(ENTITY *e) {
  // returns true if entity is out of bounds
  if ( (e->vecPosition.x >= SCREEN_W - e->size_x) ||
       (e->vecPosition.x < 0) ||
       (e->vecPosition.y < 0) ||
       (e->vecPosition.y >= SCREEN_H - e->size_y))
    return true;

  return false;
}

static ENEMY* getEnemyFromBLE(ble_gap_addr_t *peer) {
  ble_peer_entry * p;
  printf("searching for %x:%x:%x:%x:%x:%x\n",
    peer->addr[5],
    peer->addr[4],
    peer->addr[3],
    peer->addr[2],
    peer->addr[1],
    peer->addr[0]);

  for (int i = 0; i < BLE_PEER_LIST_SIZE; i++) {
    p = &ble_peer_list[i];
    if (p->ble_used == 0)
      continue;

    if (p->ble_isbadge == TRUE) {
      printf("found peer enemy %x:%x:%x:%x:%x:%x (TTL %d)\n",
        p->ble_peer_addr[5],
        p->ble_peer_addr[4],
        p->ble_peer_addr[3],
        p->ble_peer_addr[2],
        p->ble_peer_addr[1],
        p->ble_peer_addr[0],
        p->ble_ttl);
      if (memcmp(p->ble_peer_addr, peer->addr, 6) == 0) {
        // if we match, we create an enemy
        current_enemy = malloc(sizeof(ENEMY));
        memcpy(current_enemy->ble_peer_addr.addr, p->ble_peer_addr, 6);
        current_enemy->ble_peer_addr.addr_id_peer = TRUE;
        current_enemy->ble_peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
        strcpy(current_enemy->name, (char *)p->ble_peer_name);
        current_enemy->ship_type = p->ble_game_state.ble_ides_ship_type;
        current_enemy->xp = p->ble_game_state.ble_ides_xp;
        current_enemy->hp = shiptable[current_enemy->ship_type].max_hp;
        current_enemy->energy = shiptable[current_enemy->ship_type].max_energy;
        current_enemy->level = p->ble_game_state.ble_ides_level;

        current_enemy->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        current_enemy->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        current_enemy->level = p->ble_game_state.ble_ides_level;
        current_enemy->ship_locked_in = FALSE;
        return current_enemy;
      }
    }
  }

  return NULL;
}

static void
battle_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
  (void) context;
  userconfig *config = getConfig();
  ENEMY *nearest;
  BattleHandles *bh;
  GEvent * pe;
  GEventGWinButton * be;
  OrchardAppRadioEvent * radio;
  ble_evt_t * evt;
  ble_gatts_evt_rw_authorize_request_t * rw;
  ble_gatts_evt_write_t * req;

  bp_vs_pkt_t *pkt_vs;

  char msg[32];
  char tmp[40];
  bh = (BattleHandles *) context->priv;

  /* MAIN GAME EVENT LOOP */
  if (event->type == timerEvent) {
    /* animtick increments on every frame. It is reset to zero on changeState() */
    animtick++;
    /* some states lack a tick function */
    if (battle_funcs[current_battle_state].tick != NULL)
      battle_funcs[current_battle_state].tick();

    // player and enemy update
    if (current_battle_state == WORLD_MAP) {
      if (animtick % (FPS/2) == 0) {
        // refresh world map enemy positions every 500mS
        enemy_list_refresh();
        gll_each(enemies, expire_enemies);
      }

      entity_update(&(player->e), FRAME_DELAY);

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
      isp_draw_all_sprites(sprites);
    } /* WORLD_MAP */

    if (current_battle_state == COMBAT) {
      /* in combat we only have to render the player and enemy */
      // move player
      entity_update(&(player->e), FRAME_DELAY);
    }

  } /* timerEvent */

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

          // we keep our gatt connection open as we'll need it to ID
          // the other user.
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

    if (current_battle_state == LEVELUP) {
      pe = event->ugfx.pEvent;
      if (pe->type == GEVENT_GWIN_BUTTON) {
        be = (GEventGWinButton *)pe;
        if (be->gwin == bh->ghACCEPT) {
          // now that they've accepted, actually do the level-up.
          i2sPlay ("sound/click.snd");
          config->level++;
          configSave(config);
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

        /* the attacker makes the connect first, when the l2cap
           connect comes back we will switch over to a new state */
        bleL2CapConnect (BLE_IDES_BATTLE_PSM);
      }

      if (rw->request.write.handle ==
          gm_handle.value_handle &&
          req->data[0] == BLE_IDES_GAMEATTACK_DECLINE) {
        screen_alert_draw (FALSE,
                           "Battle Declined.");
        chThdSleepMilliseconds (2000);
        bleGapDisconnect ();

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
      // VS_SCREEN
      if (current_battle_state == VS_SCREEN) {
        memcpy(bh->rxbuf, radio->pkt, radio->pktlen);

        printf("recv packet with type %d, length %d\n",
          ((bp_header_t *)&bh->rxbuf)->bp_type,
          radio->pktlen
        );

        switch (((bp_header_t *)&bh->rxbuf)->bp_opcode) {
          case BATTLE_OP_SHIP_CONFIRM:
            current_enemy->ship_locked_in = TRUE;
            /* intentional fall-through */
          case BATTLE_OP_SHIP_SELECT:
            printf("got ship type packet\n");
            pkt_vs = (bp_vs_pkt_t *) &bh->rxbuf;
            current_enemy->ship_type = pkt_vs->bp_shiptype;
            current_enemy->hp = shiptable[current_enemy->ship_type].max_hp;
            current_enemy->energy = shiptable[current_enemy->ship_type].max_energy;
            state_vs_draw_enemy(current_enemy, false);

            if (current_enemy->ship_locked_in && player->ship_locked_in) {
              changeState(COMBAT);
              return;
            }
          break;
        }
      }

      // COMBAT
      // ... TBD ...

      break;
    case l2capTxEvent:
      // after transmit... they got the data.
      break;
    case l2capConnectEvent:
      // now we have a peer connection.
      // we send with bleL2CapSend(data, size) == NRF_SUCCESS ...
      printf("BATTLE: l2CapConnection Success\n");
      // stash the channel id
    	bh->cid = evt->evt.l2cap_evt.local_cid;
      if (current_enemy == NULL) {
        // copy his data over and start the battle.
        current_enemy = getEnemyFromBLE(&ble_peer_addr);
      }
      changeState(VS_SCREEN);
      break;
    default:
      break;
    }
  }

  // handle ship selection screen (VS_SCREEN) ------------------------------
  if (current_battle_state == VS_SCREEN && player->ship_locked_in == FALSE) {
    if (event->type == keyEvent) {
      if (event->key.flags == keyPress) {
        switch (event->key.code) {
        case keyALeft:
          player->ship_type--;
          break;
        case keyARight:
          player->ship_type++;
          break;
        case keyBSelect:
          player->ship_locked_in = TRUE;
  				i2sPlay ("sound/ping.snd");
          send_ship_type(player->ship_type, TRUE);

          if (current_enemy->ship_locked_in && player->ship_locked_in) {
            changeState(COMBAT);
            return;
          } else {
            state_vs_draw_enemy(player, TRUE);
          }
          break;
        default:
          break;
        }

        if (player->ship_type == 255) { player->ship_type = 5; } // underflow
        if (player->ship_type > 5) { player->ship_type = 0; }
        player->hp = shiptable[player->ship_type].max_hp;
        player->energy = shiptable[player->ship_type].max_energy;

        state_vs_draw_enemy(player, TRUE);
        send_ship_type(player->ship_type, FALSE);
      }
    }
  }
  // end handle ship selection screen (VS_SCREEN) -------------------------

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

  // Handle keypresses based on state
    if (event->type == keyEvent) {

      /* if the user lets go, we release the "throttle" */
      if ((event->key.flags == keyRelease) &&
          (current_battle_state == WORLD_MAP || current_battle_state == COMBAT)) {
        switch (event->key.code) {
        case keyALeft:
          player->e.vecVelocityGoal.x = 0;
          break;
        case keyARight:
          player->e.vecVelocityGoal.x = 0;
          break;
        case keyAUp:
          player->e.vecVelocityGoal.y = 0;
          break;
        case keyADown:
          player->e.vecVelocityGoal.y = 0;
          break;
        default:
          break;
        }
      }

      if (event->key.flags == keyPress)  {
        if (current_battle_state == WORLD_MAP || current_battle_state == COMBAT) {
          /* on keypress, we'll throttle up */
          switch (event->key.code) {
          case keyALeft:
            player->e.vecVelocityGoal.x = -VGOAL;
            break;
          case keyARight:
            player->e.vecVelocityGoal.x = VGOAL;
            break;
          case keyAUp:
            player->e.vecVelocityGoal.y = -VGOAL;
            break;
          case keyADown:
            player->e.vecVelocityGoal.y = VGOAL;
            break;
          case keyASelect:
            if (current_battle_state == WORLD_MAP) {
              i2sPlay ("sound/click.snd");
              orchardAppExit ();
            }
            break;
          case keyBSelect:
            if (current_battle_state == WORLD_MAP) {
              if ((nearest = enemy_engage(context)) != NULL) {
                /* we need to preserve the current enemy so we have things like
                 * their address and name. The pointer to the pixmp will be lost
                 * as changeState flushes the enemy list, but we don't use
                 * that for enemy drawing until we are in the combat state.
                 *
                 * We'll alloc some new space below, which we will release
                 * when we return to the world map.
                 */

                 /* convert the enemy struct to the larger combat version */
                 current_enemy = malloc(sizeof(ENEMY));
                 memcpy(current_enemy, nearest, sizeof(ENEMY));
                 current_enemy->e.size_x = SHIP_SIZE_ZOOMED;
                 current_enemy->e.size_y = SHIP_SIZE_ZOOMED;

                 changeState(APPROVAL_WAIT);
              }
            }
            break;
          default:
            break;
          }
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

static void remove_all_enemies(void) {
  /* remove all enemies. */
  if (enemies) {
    gll_each(enemies, free_enemy);
    gll_destroy(enemies);
    enemies = NULL;
  }
}

static void battle_exit(OrchardAppContext *context)
{
  userconfig *config = getConfig();
  BattleHandles *bh;

  // free the UI
  bh = (BattleHandles *)context->priv;
  // store the last x/y of this guy
  config->last_x = player->e.vecPosition.x;
  config->last_y = player->e.vecPosition.y;
  configSave(config);

  // reset this state or we won't init properly next time.
  // Do not free state-objects here, let the state-exit functions do that!
  changeState(NONE);

  if (bh->fontXS)
    gdispCloseFont (bh->fontXS);

  if (bh->fontLG)
    gdispCloseFont (bh->fontLG);

  if (bh->fontSM)
    gdispCloseFont (bh->fontSM);

  if (bh->fontSTENCIL)
      gdispCloseFont (bh->fontSTENCIL);

  if (bh->ghTitleL)
    gwinDestroy (bh->ghTitleL);

  if (bh->ghTitleR)
    gwinDestroy (bh->ghTitleR);

  geventRegisterCallback (&bh->gl, NULL, NULL);
  geventDetachSource (&bh->gl, NULL);


  if (player) {
    free(player);
  }

  if (current_enemy) {
    free(current_enemy);
  }

  free (context->priv);
  context->priv = NULL;

  // restore the LED pattern from config
  ledSetPattern(config->led_pattern);
  return;
}

static void
render_all_enemies(void) {
  // renders all enemies based on enemy linked list
  // will not update unless enemy moves
}

/* state change and tick functions go here -------------------------------- */
// HANDSHAKE --------------------------------------------------------------
void state_handshake_enter(void) {
  gdispClear(Black);
  screen_alert_draw(true, "HANDSHAKING...");
  state_time_left = 5;
}

void state_handshake_tick(void) {
  // in this state we are being attacked and we are holding pending a
  // l2cap connection with a battle in it. It's still possible to TIMEOUT
  // and we have to handle that if it happens. We'l hold for five seconds.
  if (animtick % FPS == 0) {
    state_time_left--;
    if (state_time_left <= 0) {
      screen_alert_draw(true, "HS TIMEOUT :(");
      changeState(WORLD_MAP);
    }
  }
}

// WORLDMAP ---------------------------------------------------------------
void state_worldmap_enter(void) {
  userconfig *config = getConfig();

  if (current_enemy != NULL) {
    /* we most likely came here from an approval_wait state
     * or the end of a battle. Release the enemy. */

    free_enemy(current_enemy);
    current_enemy = NULL;
  }

  gdispClear(Black);
  draw_world_map();

  // init sprite system and capture water areas
  sprites = isp_init();
  // PERFORMANCE: this call is VERY SLOW. NEEDS WORK.
  isp_scan_screen_for_water(sprites);

  // draw player for first tiplayer.e.
  entity_init(&player->e, SHIP_SIZE_WORLDMAP, SHIP_SIZE_WORLDMAP, T_PLAYER);
  player->e.vecPosition.x = config->last_x;
  player->e.vecPosition.y = config->last_y;
  player->e.visible = true;

  isp_set_sprite_xy(sprites,
    player->e.sprite_id,
    player->e.vecPosition.x,
    player->e.vecPosition.y);

  sprites->list[player->e.sprite_id].status = ISP_STAT_DIRTY_BOTH;
  isp_draw_all_sprites(sprites);

  // load the enemy list
  enemies = gll_init();
  enemy_list_refresh();

  render_all_enemies();
}

void state_worldmap_tick(void) {
  userconfig *config = getConfig();
#ifdef ENABLE_MAP_PING
  /* every four seconds (120 frames) go ping */
  if (animtick % (FPS*4) == 0) {
    i2sPlay("game/map_ping.snd");
  }
#endif
  // Update BLE
  // if we moved, we have to update BLE.
  if (player->e.vecPosition.x != player->e.prevPos.x ||
      player->e.vecPosition.y != player->e.prevPos.y) {
        bleGapUpdateState ((uint16_t)player->e.vecPosition.x,
                           (uint16_t)player->e.vecPosition.y,
                           config->xp,
                           config->level,
                           config->current_ship,
                           config->in_combat);
  }
}

void state_worldmap_exit(void) {
  BattleHandles *bh;
  userconfig *config = getConfig();

  // get private memory
  bh = (BattleHandles *)mycontext->priv;

  // store our last position for later.
  config->last_x = player->e.vecPosition.x;
  config->last_y = player->e.vecPosition.y;
  configSave(config);

  if (bh->ghTitleL) {
    gwinDestroy (bh->ghTitleL);
    bh->ghTitleL= NULL;
  }

  if (bh->ghTitleR) {
    gwinDestroy (bh->ghTitleR);
    bh->ghTitleR = NULL;
  }

  /* wipe enemies */
  remove_all_enemies();

  /* tear down sprite system */
  isp_shutdown(sprites);

}

// APPROVAL_WAIT -------------------------------------------------------------
// we are attacking someone, wait for consent.
void state_approval_wait_enter(void) {
  // we will now attempt to open a BLE gap connect to the user.
  printf("engage: %x:%x:%x:%x:%x:%x\n",
    current_enemy->ble_peer_addr.addr[5],
    current_enemy->ble_peer_addr.addr[4],
    current_enemy->ble_peer_addr.addr[3],
    current_enemy->ble_peer_addr.addr[2],
    current_enemy->ble_peer_addr.addr[1],
    current_enemy->ble_peer_addr.addr[0]);

  gdispClear(Black);
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
    bleGapDisconnect ();
    changeState(WORLD_MAP);
  }
}

void state_approval_wait_exit(void) {
  // no-op.
}

// APPOVAL_DEMAND ------------------------------------------------------------
// Someone is attacking us.
void state_approval_demand_enter(void) {
  BattleHandles *bh;
	ble_peer_entry * peer;
  char buf[36];
	int fHeight;
	GWidgetInit wi;

  animtick = 0;
  bh = (BattleHandles *)mycontext->priv;
  gdispClear(Black);

  peer = blePeerFind (ble_peer_addr.addr);
  if (peer == NULL)
		snprintf (buf, 36, "Badge %X:%X:%X:%X:%X:%X",
		    ble_peer_addr.addr[5],  ble_peer_addr.addr[4],
		    ble_peer_addr.addr[3],  ble_peer_addr.addr[2],
		    ble_peer_addr.addr[1],  ble_peer_addr.addr[0]);
	else
		snprintf (buf, 36, "%s", peer->ble_peer_name);

  putImageFile ("images/undrattk.rgb", 0, 0);
  i2sPlay("sound/klaxon.snd");

  fHeight = gdispGetFontMetric (bh->fontSM, fontHeight);

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
    bleGapDisconnect();
    changeState(WORLD_MAP);
  }
}

void state_approval_demand_exit(void) {
  BattleHandles *bh;
  bh = mycontext->priv;

  gwinDestroy (bh->ghACCEPT);
  gwinDestroy (bh->ghDECLINE);
}

// COMBAT --------------------------------------------------------------------

void state_combat_enter(void) {
  char fnbuf[30];
  int newmap;
  userconfig *config = getConfig();

  // we're in combat now, send our last advertisement.
  state_time_left = 120;
  config->in_combat = 1;
  configSave(config);

  bleGapUpdateState ((uint16_t)player->e.vecPosition.x,
                     (uint16_t)player->e.vecPosition.y,
    								 config->xp,
    								 config->level,
    								 config->current_ship,
    								 config->in_combat);

  /* rebuild the player pixmap */
  player->e.size_x = SHIP_SIZE_ZOOMED;
  player->e.size_y = SHIP_SIZE_ZOOMED;

  /* the attacker's position is what we base the starting map on.
   * we have to pick one, because if we pick two, and players are in
   * different quads, we're in trouble.
   */
	if (ble_gap_role == BLE_GAP_ROLE_CENTRAL) {
    newmap = getMapTile(&player->e);
    printf("BLECENTRAL: map %d from player at %f, %f\n",
       newmap,
       player->e.vecPosition.x,
       player->e.vecPosition.y
    );
  } else {
    newmap = getMapTile(&current_enemy->e);
    printf("BLEPREPH: map %d from player at %f, %f\n",
       newmap,
       current_enemy->e.vecPosition.x,
       current_enemy->e.vecPosition.y
    );
  }

  sprintf(fnbuf, "game/map-%02d.rgb", newmap);
  putImageFile(fnbuf, 0,0);
  draw_hud(mycontext);

  // re-init the sprite system
  sprites = isp_init();
  // PERFORMANCE: this call is VERY SLOW. NEEDS WORK.
  isp_scan_screen_for_water(sprites);

  // players
  entity_init(&player->e, SHIP_SIZE_ZOOMED, SHIP_SIZE_ZOOMED, T_PLAYER);
  entity_init(&current_enemy->e, SHIP_SIZE_ZOOMED, SHIP_SIZE_ZOOMED, T_ENEMY);

  /* move the player to the starting position */
	if (ble_gap_role == BLE_GAP_ROLE_CENTRAL) {
    // Attacker (the central) gets left.
    player->e.vecPosition.x = ship_init_pos_table[newmap].attacker.x;
    player->e.vecPosition.y = ship_init_pos_table[newmap].attacker.y;
    current_enemy->e.vecPosition.x = ship_init_pos_table[newmap].defender.x;
    current_enemy->e.vecPosition.y = ship_init_pos_table[newmap].defender.y;
  } else {
    // Defender gets on the right.
    player->e.vecPosition.x = ship_init_pos_table[newmap].defender.x;
    player->e.vecPosition.y = ship_init_pos_table[newmap].defender.y;
    current_enemy->e.vecPosition.x = ship_init_pos_table[newmap].attacker.x;
    current_enemy->e.vecPosition.y = ship_init_pos_table[newmap].attacker.y;
  }

  // draw player
  isp_load_image_from_file(sprites,
    player->e.sprite_id,
    getAvatarImage(player->ship_type, true, 'n', true));

  isp_set_sprite_xy(sprites,
                    player->e.sprite_id,
                    player->e.vecPosition.x,
                    player->e.vecPosition.y);

  isp_load_image_from_file(sprites,
    current_enemy->e.sprite_id,
    getAvatarImage(current_enemy->ship_type, false, 'n', false));

  isp_set_sprite_xy(sprites,
                    current_enemy->e.sprite_id,
                    current_enemy->e.vecPosition.x,
                    current_enemy->e.vecPosition.y);

  isp_draw_all_sprites(sprites);

}

void state_combat_tick(void) {
  char tmp[10];
  int min;

  BattleHandles *bh;
  bh = mycontext->priv;

  if (animtick % FPS == 0) {
    state_time_left--;
    min = state_time_left / 60;

    sprintf(tmp, "%02d:%02d",
      min,
      state_time_left - (min * 60));

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

    if (state_time_left == 0) {
      // time up.
      i2sPlay("sound/foghrn.snd");
      changeState(SHOW_RESULTS);
    }
  }
}

void state_combat_exit(void) {
  BattleHandles *bh;
  bh = mycontext->priv;

  free(bh->l_pxbuf);
  bh->l_pxbuf = NULL;

  free(bh->c_pxbuf);
  bh->c_pxbuf = NULL;

  free(bh->r_pxbuf);
  bh->r_pxbuf = NULL;

  /* tear down sprite system */
  isp_shutdown(sprites);

  // this can probably be removed?
  player->e.size_x = 10;
  player->e.size_y = 10;
}

static void state_levelup_enter(void) {
  GWidgetInit wi;
  BattleHandles *p = mycontext->priv;
  userconfig *config = getConfig();
  char tmp[40];
  int curpos;

  gdispClear(Black);
  i2sPlay("sound/levelup.snd");
  putImageFile("game/map-07.rgb",0,0);

  // insiginia
  sprintf(tmp, "game/rank-%d.rgb", config->level+1);
  putImageFile(tmp, 35, 79);
  gdispDrawBox(35, 79, 53, 95,Grey);

  curpos = 10;
  gdispDrawStringBox (35,
		                  curpos,
                      300,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
		                  "RANK UP!",
                      p->fontSTENCIL,
                      Yellow,
                      justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontSTENCIL, fontHeight);

  gdispDrawStringBox (35,
		                  curpos,
                      300,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
		                  rankname[config->level+1],
                      p->fontSTENCIL,
                      Yellow,
                      justifyLeft);


  if (config->level+1 < 6) {
    // this unlocks a ship
    putImageFile(getAvatarImage(config->level+1, true, 'n', false),
                 270,79);
    gdispDrawBox(270, 79, 40, 40, White);
    curpos = 79;
    strncpy(tmp, shiptable[config->level+1].type_name,40);
    strntoupper(tmp, 40);

    gdispDrawStringBox (110,
  		                  curpos,
                        210,
                        gdispGetFontMetric(p->fontSM, fontHeight),
  		                  tmp,
                        p->fontSM,
                        White,
                        justifyLeft);

    curpos = curpos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;

    gdispDrawStringBox (110,
  		                  curpos,
                        210,
                        gdispGetFontMetric(p->fontSM, fontHeight),
    		                  "UNLOCKED!",
                        p->fontSM,
                        White,
                        justifyLeft);
    curpos = 145;
  } else {
    // we'll draw the next level message in it's place.
    curpos = 79;
  }
  gdispDrawStringBox (110,
		                  curpos,
                      210,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
		                  "NEXT LEVEL",
                      p->fontSTENCIL,
                      Yellow,
                      justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontSTENCIL, fontHeight) + 2;

  sprintf(tmp, "AT %d XP",  xp_for_level(config->level+2));
  gdispDrawStringBox (110,
		                  curpos,
                      210,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
		                  tmp,
                      p->fontSTENCIL,
                      Yellow,
                      justifyLeft);

  // draw UI
  gwinSetDefaultStyle(&RedButtonStyle, FALSE);
  gwinWidgetClearInit(&wi);
  wi.g.show = TRUE;
  wi.g.x = 85;
  wi.g.y = 206;
  wi.g.width = 150;
  wi.g.height = 34;
  wi.text = "CONTINUE";

  p->ghACCEPT = gwinButtonCreate(0, &wi);

  gwinSetDefaultStyle(&BlackWidgetStyle, FALSE);
}

static void state_levelup_tick(void) {
  /* no-op we will hold the levelup screen indefinately until the user
   * chooses. the user is also in-combat and cannot accept new fights
   */
}

static void state_levelup_exit(void) {
  BattleHandles *p = mycontext->priv;

  if (p->ghACCEPT != NULL) {
    gwinDestroy (p->ghACCEPT);
    p->ghACCEPT = NULL;
  }
}

static void state_vs_draw_enemy(ENEMY *e, bool is_player) {
  /* draw the player's ship and stats */
  BattleHandles *p = mycontext->priv;

  int curpos;
  int curpos_x;

  char tmp[40];

  if (is_player) {
    curpos_x = 60;
  } else {
    curpos_x = 220;
  }

  // us
  putImageFile(getAvatarImage(e->ship_type, is_player, 'n', true), curpos_x, 89);
  gdispDrawBox(curpos_x, 89, 40, 40, White);

  if (is_player) {
    // clear left side
    gdispFillArea(0, 95,
                  59, 30,
                  HTML2COLOR(0x151285));

    // clear name area
    gdispFillArea(0, 132,
                  160, gdispGetFontMetric(p->fontXS, fontHeight),
                  HTML2COLOR(0x151285));
    curpos_x = 0;
  } else {
    // clear right side
    gdispFillArea(261, 95,
                  59, 26,
                 HTML2COLOR(0x151285));

    curpos_x = 262;
  }

  // our stats
  curpos=95;

  sprintf(tmp, "HP %d", e->hp);
  gdispDrawStringBox (curpos_x,
                      curpos,
                      58,
                      gdispGetFontMetric(p->fontXS, fontHeight),
                      tmp,
                      p->fontXS,
                      White,
                      is_player ? justifyRight : justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontXS, fontHeight);
  sprintf(tmp, "EN %d", e->energy);
  gdispDrawStringBox (curpos_x,
                      curpos,
                      58,
                      gdispGetFontMetric(p->fontXS, fontHeight),
                      tmp,
                      p->fontXS,
                      White,
                      is_player ? justifyRight : justifyLeft);
  if (is_player) {
    curpos_x = 0;
  } else {
    curpos_x = 160;
  }

  // clear name area
  gdispFillArea(curpos_x, 130,
                160, gdispGetFontMetric(p->fontXS, fontHeight),
                HTML2COLOR(0x151285));
  // draw name
  gdispDrawStringBox (curpos_x,
                      130,
                      160,
                      gdispGetFontMetric(p->fontXS, fontHeight),
                      shiptable[e->ship_type].type_name,
                      p->fontXS,
                      White,
                      justifyCenter);

  if (! e->ship_locked_in) {
    gdispFillArea(curpos_x + 25, 160, 110, 30, Red);
    gdispDrawStringBox (curpos_x + 25,
                        164,
                        110,
                        gdispGetFontMetric(p->fontSM, fontHeight),
                        "WAITING",
                        p->fontSM,
                        White,
                        justifyCenter);
  } else {
    gdispFillArea(curpos_x + 25, 160, 110, 30, Green);
    gdispDrawStringBox (curpos_x + 25,
                        164,
                        110,
                        gdispGetFontMetric(p->fontSM, fontHeight),
                        "READY",
                        p->fontSM,
                        White,
                        justifyCenter);

  }
}

static void send_ship_type(uint16_t type, bool final) {
  BattleHandles *p;

  // get private memory
  p = (BattleHandles *)mycontext->priv;
  bp_vs_pkt_t pkt;

  memset (p->txbuf, 0, sizeof(p->txbuf));
  pkt.bp_header.bp_opcode = final ? BATTLE_OP_SHIP_CONFIRM : BATTLE_OP_SHIP_SELECT;
  pkt.bp_header.bp_type = T_ENEMY; // always enemy.
  pkt.bp_shiptype = type;
  pkt.bp_pad = 0xffff;
  memcpy (p->txbuf, &pkt, sizeof(bp_vs_pkt_t));

  if (bleL2CapSend ((uint8_t *)p->txbuf, sizeof(bp_vs_pkt_t)) != NRF_SUCCESS) {
    screen_alert_draw(true, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit ();
    return;
  }
}

static void state_vs_enter(void) {
  BattleHandles *p = mycontext->priv;
  userconfig *config = getConfig();

  state_time_left = 20;

  putImageFile("game/world.rgb", 0,0);

	if (ble_gap_role == BLE_GAP_ROLE_CENTRAL) {
    // Was gonna play this on both devices, but then
    // it sounds out of sync and crazy.
    i2sPlay("sound/ffenem.snd");
  }

  // boxes
  gdispFillArea(0, 14, 320, 30, Black);
  gdispFillArea(0, 50, 320, 100, HTML2COLOR(0x151285));
  gdispFillArea(0, 199, 320, 30, Black);

  gdispDrawStringBox (0,
                      14,
                      320,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                      "CONNECTED! CHOOSE SHIPS!",
                      p->fontSTENCIL,
                      Yellow,
                      justifyCenter);

  gdispDrawStringBox (0,
                      199,
                      320,
                      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                      ":15",
                      p->fontSTENCIL,
                      Yellow,
                      justifyCenter);

  // names
  gdispDrawStringBox (0,
                      60,
                      160,
                      gdispGetFontMetric(p->fontSM, fontHeight),
                      config->name,
                      p->fontSM,
                      White,
                      justifyCenter);
  gdispDrawStringBox (160,
                      60,
                      160,
                      gdispGetFontMetric(p->fontSM, fontHeight),
                      current_enemy->name,
                      p->fontSM,
                      White,
                      justifyCenter);

  state_vs_draw_enemy(player, TRUE);
  state_vs_draw_enemy(current_enemy, FALSE);

  // we'll now tell them what our ship is.
	send_ship_type(player->ship_type, false);
}

static void state_vs_tick(void) {
  // update the clock
  BattleHandles *p = mycontext->priv;
  char tmp[40];

  if (animtick % FPS == 0) {
    state_time_left--;
    sprintf(tmp, ":%02d",
      state_time_left);

    gdispFillArea(150,
      199,
      100,
      gdispGetFontMetric(p->fontSTENCIL, fontHeight),
      Black);

    gdispDrawStringBox (0,
                        199,
                        320,
                        gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                        tmp,
                        p->fontSTENCIL,
                        Yellow,
                        justifyCenter);
  }

  if (state_time_left == 0) {
    // time up -- force to next screen.
    i2sPlay("game/engage.snd");
    changeState(COMBAT);
  }

}

static void state_vs_exit(void) {
  // no-op for now
}


// -------------------------------------------------------------------------

orchard_app("Sea Battle",
            "icons/ship.rgb",
            0,
            battle_init, battle_start,
            battle_event, battle_exit, 1);
