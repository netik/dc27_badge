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
#include "battle_states.h"
#include "gll.h"
#include "math.h"
#include "led.h"
#include "rand.h"
#include "userconfig.h"
#include "enemy.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"
#include "strutil.h"

// debugging defines ---------------------------------------
// debugs the discovery process
#undef DEBUG_ENEMY_DISCOVERY
// track battle state
#define DEBUG_BATTLE_STATE    1
// debug TTL of objects
#undef DEBUG_ENEMY_TTL
#undef ENABLE_MAP_PING                   // if you want the sonar sounds

// end debug defines ---------------------------------------

#include "battle_states.h"
#include "statemachine.h"
#include "ships.h"

#ifdef DEBUG_BATTLE_STATE
const char* battle_state_name[]  = {
  "NONE"  ,             // 0 - Not started yet.
  "WORLD_MAP"  ,        // 1 - we're in the lobby
  "HANDSHAKE",          // 2 - a packet came in and we wait for l2cap connect
  "APPROVAL_DEMAND"  ,  // 4 - I want to fight you!
  "APPROVAL_WAIT"  ,    // 5 - I am waiting to see if you want to fight me
  "VS_SCREEN"  ,        // 6 - I am showing the versus screen.
  "COMBAT"  ,           // 7 - We are fighting
  "SHOW_RESULTS"  ,     // 8 - We are showing results
  "PLAYER_DEAD"  ,      // 9 - I die!
  "ENEMY_DEAD",         // 10 - You're dead.
  "LEVELUP"             // 11 - Show the bonus screen
  };

#endif /* DEBUG_FIGHT_STATE */

/* the sprite system */
static ISPRITESYS *sprites = NULL;
// yes, yes, everyone is an enemy
// this way, the same code works for both users.
static ENEMY * player        = NULL;
static ENEMY * current_enemy = NULL;
static ENEMY * last_near     = NULL;
static ENTITY *bullet[MAX_BULLETS];

// time left in combat (or in various related states)
static uint8_t state_time_left = 0;

OrchardAppContext *mycontext;

/* private memory */
typedef struct _BattleHandles
{
  GListener gl;
  GListener gl2;
  font_t    fontLG;
  font_t    fontXS;
  font_t    fontSM;
  font_t    fontSTENCIL;
  GHandle   ghTitleL;
  GHandle   ghTitleR;
  GHandle   ghACCEPT;
  GHandle   ghDECLINE;

  // these are the top row, player stats frame buffers.
  pixel_t * l_pxbuf;
  pixel_t * c_pxbuf;
  pixel_t * r_pxbuf;

  // misc variables
  uint16_t  cid;       // l2capchannelid for combat
  char      txbuf[MAX_PEERMEM + BLE_IDES_L2CAP_MTU + 3];
  char      rxbuf[BLE_IDES_L2CAP_MTU];

  // left and right variants of the players; used in COMBAT only.
  ISPHOLDER *pl_left, *pl_right, *ce_left, *ce_right;
  // for the cruiser, we also need the shielded versions
  ISPHOLDER *pl_s_left, *pl_s_right, *ce_s_left, *ce_s_right;
} BattleHandles;

// enemies -- linked list
gll_t *enemies = NULL;

/* state tracking */
static battle_state current_battle_state = NONE;
static int16_t      animtick             = 0;

/* prototypes */
static void changeState(battle_state nextstate);
static void render_all_enemies(void);
static void send_ship_type(uint16_t type, bool final);
static void state_vs_draw_enemy(ENEMY *e, bool is_player);
static void send_position_update(uint16_t id, uint8_t opcode, uint8_t type, ENTITY *e);
static void send_bullet_create(int16_t seq, ENTITY *e, int16_t dir_x, int16_t dir_y, uint8_t is_free);
static void send_state_update(uint16_t opcode, uint16_t operand);
static void redraw_combat_clock(void);
bool entity_OOB(ENTITY *e);
/* end protos */

static
int getMapTile(ENTITY *e)
{
  // we have to cast to int or math will be very wrong
  int x = (int)e->vecPosition.x;
  int y = (int)e->vecPosition.y;

  return(((y / TILE_H) * 4) + (x / TILE_W));
}

/* approach lets us interpolate smoothly between positions */
static float
approach(float flGoal, float flCurrent, float dt)
{
  float flDifference = flGoal - flCurrent;

  if (flDifference > dt)
  {
    return(flCurrent + dt);
  }

  if (flDifference < -dt)
  {
    return(flCurrent - dt);
  }

  return(flGoal);
}

static bool
check_land_collision(ENTITY *p)
{
  return(test_sprite_for_land_collision(sprites, p->sprite_id));
}

static void redraw_player_bars(void) {
  // player's stats
  drawProgressBar(0, 26, 120, 6,
    shiptable[player->ship_type].max_hp, player->hp, FALSE, FALSE);
  drawProgressBar(0, 34, 120, 6,
    shiptable[player->ship_type].max_energy, player->energy, FALSE, FALSE);
}

static void redraw_enemy_bars(void) {
  // enemy stats
  drawProgressBar(200, 26, 120, 6,
    shiptable[current_enemy->ship_type].max_hp, current_enemy->hp, FALSE, TRUE);
  drawProgressBar(200, 34, 120, 6,
    shiptable[current_enemy->ship_type].max_energy, current_enemy->energy, FALSE, TRUE);
}


static void entity_update(ENTITY *p, float dt)
{
  if ((p->ttl > -1) && (p->visible))
  {
    p->ttl--;
    if (p->ttl == 0)
    {
      p->visible = FALSE;
      i2sPlay("game/splash.snd");
      return;
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
      p->prevPos.y != p->vecPosition.y)
  {
    // check for collision with land
    isp_set_sprite_xy(sprites,
                      p->sprite_id,
                      p->vecPosition.x,
                      p->vecPosition.y);

    if (p->type == T_PLAYER || p->type == T_ENEMY) {
      if (check_land_collision(p) || entity_OOB(p))
      {

        // if in combat, you take damage.
        p->vecPosition.x     = p->prevPos.x;
        p->vecPosition.y     = p->prevPos.y;
        p->vecVelocity.x     = 0;
        p->vecVelocity.y     = 0;
        p->vecVelocityGoal.x = 0;
        p->vecVelocityGoal.y = 0;
        return;
      }
    }

    isp_set_sprite_xy(sprites,
                      p->sprite_id,
                      p->vecPosition.x,
                      p->vecPosition.y);
  }
}

static uint32_t
battle_init(OrchardAppContext *context)
{
  // don't allocate any stack space
  return(0);
}

static void draw_world_map(void)
{
  const userconfig *config = getConfig();
  GWidgetInit       wi;
  char           tmp[40];
  BattleHandles *bh;

  // get private memory
  bh = (BattleHandles *)mycontext->priv;

  // draw map
  putImageFile("game/world.rgb", 0, 0);

  // Draw UI
  gwinWidgetClearInit(&wi);

  /* Create label widget: ghTitleL */
  wi.g.show      = TRUE;
  wi.g.x         = 0;
  wi.g.y         = 0;
  wi.g.width     = 160;
  wi.g.height    = gdispGetFontMetric(bh->fontSM, fontHeight) + 2;
  wi.text        = config->name;
  wi.customDraw  = gwinLabelDrawJustifiedLeft;
  wi.customParam = 0;
  wi.customStyle = &DarkPurpleStyle;
  bh->ghTitleL   = gwinLabelCreate(0, &wi);
  gwinLabelSetBorder(bh->ghTitleL, FALSE);
  gwinSetFont(bh->ghTitleL, bh->fontSM);
  gwinRedraw(bh->ghTitleL);

  /* Create label widget: ghTitleR */
  strcpy(tmp, "Find an enemy!");
  wi.g.show     = TRUE;
  wi.g.x        = 150;
  wi.g.width    = 170;
  wi.text       = tmp;
  wi.customDraw = gwinLabelDrawJustifiedRight;
  bh->ghTitleR  = gwinLabelCreate(0, &wi);
  gwinSetFont(bh->ghTitleR, bh->fontSM);
  gwinLabelSetBorder(bh->ghTitleR, FALSE);
  gwinRedraw(bh->ghTitleR);
}

static void
copy_config_to_player(void)
{
  userconfig *config = getConfig();

  player = malloc(sizeof(ENEMY));
  memset(player, 0, sizeof(ENEMY));

  // this is done once at startup. These are the "live" values used in game,
  // where as the config is the long-term/stored value.
  memset(&(player->ble_peer_addr), 0, sizeof(ble_gap_addr_t));
  strncpy(player->name, config->name, CONFIG_NAME_MAXLEN);
  player->level = config->level;

  // we cannot determine hp or xp until ship has been selected but we can
  // guess based on last used.
  player->hp             = shiptable[config->current_ship].max_hp;
  player->energy         = shiptable[config->current_ship].max_energy;
  player->xp             = config->xp;
  player->ship_type      = config->current_ship;
  player->ship_locked_in = FALSE;
  player->ttl            = -1;
  player->last_shot_ms   = 0;
  // ENTITY player.e will be init'd during combat
}

static void
battle_start(OrchardAppContext *context)
{
  userconfig *   config = getConfig();
  BattleHandles *bh;
  GSourceHandle gs;

  bh = malloc(sizeof(BattleHandles));
  memset(bh, 0, sizeof(BattleHandles));
  bh->cid   = BLE_L2CAP_CID_INVALID;
  mycontext = context;

  // turn off the LEDs
  ledSetPattern(LED_PATTERN_WORLDMAP);

  context->priv   = bh;
  bh->fontXS      = gdispOpenFont(FONT_XS);
  bh->fontLG      = gdispOpenFont(FONT_LG);
  bh->fontSM      = gdispOpenFont(FONT_SM);
  bh->fontSTENCIL = gdispOpenFont(FONT_STENCIL);

  // gtfo if in airplane mode.
  if (config->airplane_mode)
  {
    screen_alert_draw(TRUE, "TURN OFF AIRPLANE MODE!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppRun(orchardAppByName("Badge"));
    return;
  }

  /* start the timer - 30 fps */
  orchardAppTimer(context, FRAME_DELAY * 1000000, TRUE);

  /* stand up ugfx */
  geventListenerInit(&bh->gl);
  gwinAttachListener(&bh->gl);
  geventRegisterCallback(&bh->gl, orchardAppUgfxCallback, &bh->gl);

  /*
   * Set up a listener for mouse meta events too. We need this
   * to detect simple screen touch events in addition to gwin
   * widget events.
   */
  gs = ginputGetMouse (0);
  geventListenerInit(&bh->gl2);
  geventAttachSource (&bh->gl2, gs, GLISTEN_MOUSEMETA);
  geventRegisterCallback(&bh->gl2, orchardAppUgfxCallback, &bh->gl2);

  // clear the bullet array
  memset(bullet, 0, sizeof(bullet));

  // stand us up.
  copy_config_to_player();

  // how did we get here?
  // if we have a bleGapConnection then we can
  // use data from the current peer, and we start in
  // combat instead of accept.

  // if we have a peer address, we're gonna start in a handshake state.
  if (ble_peer_addr.addr_type != 0x0)
  {
    // we're gonna sit on our hands for a minute and wait for the l2cap connect.
    changeState(HANDSHAKE);
  }
  else
  {
    changeState(WORLD_MAP);
  }
  return;
}

static void
draw_hud(OrchardAppContext *context)
{
  userconfig *   config = getConfig();
  BattleHandles *bh;

  // get private memory
  bh = (BattleHandles *)context->priv;

  // top bar
  drawBufferedStringBox(&bh->l_pxbuf,
                        0,
                        0,
                        129,
                        gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
                        config->name,
                        bh->fontSM,
                        White,
                        justifyLeft);

  drawBufferedStringBox(&bh->r_pxbuf,
                        190,
                        0,
                        130,
                        gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
                        current_enemy->name,
                        bh->fontSM,
                        White,
                        justifyRight);

  redraw_player_bars();
  redraw_enemy_bars();
}


static void
fire_bullet(ENEMY *e, int16_t dir_x, int16_t dir_y, uint8_t is_free)
{
  int count = 0;

  // if this is local (i.e. the player) we count to see if any more are
  // possible for the player's current ship type.
  if (e == player) {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullet[i] != NULL && bullet[i]->type == T_PLAYER_BULLET) {
        count++;
      }
    }

    if (count >= shiptable[e->ship_type].max_bullets) {
      // no more for you.
      return;
    }

    if (!is_free) {
      if (player->energy - shiptable[e->ship_type].shot_cost <= 0) {
        // no more energy
        i2sPlay("game/error.snd");
        return;
      }

      // bill the player.
      player->energy = player->energy - shiptable[e->ship_type].shot_cost;

      // redraw energy bar.
      redraw_player_bars();
    }
  }

  for (int i = 0; i < MAX_BULLETS; i++)
  {
    // find the first available bullet structure
    // create a new entity and fire something from it.
    if (bullet[i] == NULL) {
      bullet[i] = malloc(sizeof(ENTITY));
      entity_init(bullet[i],
        sprites,
        shiptable[e->ship_type].shot_size,
        shiptable[e->ship_type].shot_size,
        e == player ? T_PLAYER_BULLET : T_ENEMY_BULLET);
      bullet[i]->visible = TRUE;

      // bullets don't use TTL, they are ranged (see update_bullets)
      bullet[i]->ttl     = -1;

      // bullets start from the edge of the direction of fire.
      bullet[i]->vecPosition.x = e->e.vecPosition.x;
      bullet[i]->vecPosition.y = e->e.vecPosition.y;

      // and it's fired from the ship, so that velocity is added in.
      bullet[i]->vecVelocity.x = e->e.vecVelocity.x;
      bullet[i]->vecVelocity.y = e->e.vecVelocity.y;

      if (dir_x < 0) {
        bullet[i]->vecPosition.x = e->e.vecPosition.x - shiptable[e->ship_type].shot_size;
      }

      if (dir_x > 0) {
        bullet[i]->vecPosition.x = e->e.vecPosition.x + SHIP_SIZE_ZOOMED;
      }

      if (dir_y < 0) {
        bullet[i]->vecPosition.y = e->e.vecPosition.y - shiptable[e->ship_type].shot_size;
      }

      if (dir_y > 0) {
        bullet[i]->vecPosition.y = e->e.vecPosition.y + SHIP_SIZE_ZOOMED;
      }

      // center the bullet
      if (dir_x == 0) { bullet[i]->vecPosition.x += ((SHIP_SIZE_ZOOMED/2) - (shiptable[e->ship_type].shot_size/2)); }
      if (dir_y == 0) { bullet[i]->vecPosition.y += ((SHIP_SIZE_ZOOMED/2) - (shiptable[e->ship_type].shot_size/2)); }

      // the goal and the current V should be the same.
      bullet[i]->vecVelocityGoal.x = dir_x * shiptable[e->ship_type].shot_speed;
      bullet[i]->vecVelocityGoal.y = dir_y * shiptable[e->ship_type].shot_speed;

      bullet[i]->vecVelocity.x += dir_x * shiptable[e->ship_type].shot_speed;
      bullet[i]->vecVelocity.y += dir_y * shiptable[e->ship_type].shot_speed;

      // used for ranging
      bullet[i]->vecPosOrigin.x = bullet[i]->vecPosition.x;
      bullet[i]->vecPosOrigin.y = bullet[i]->vecPosition.y;

      isp_set_sprite_xy(sprites,
                        bullet[i]->sprite_id,
                        bullet[i]->vecPosition.x,
                        bullet[i]->vecPosition.y);

      // record the shot
      e->last_shot_ms = chVTGetSystemTime();

      if (e == player) {
        // fire across network if the player is firing
        send_bullet_create(i, bullet[i], dir_x, dir_y, is_free);
      }

      i2sPlay("game/shot.snd");
      return;
    }
  }
  // We failed to fire the shot becasue we're out of bullet memory.
  // this shouldn't ever happen, but we'll make some noise anyway.
  i2sPlay("game/error.snd");

}

void update_bullets(void) {
  int i;
  uint16_t dmg;
  uint16_t max_range,travelled;
  char st;

  for (i = 0; i < MAX_BULLETS; i++) {
    if (bullet[i]) {
      entity_update(bullet[i], FRAME_DELAY);

      // figure out how far away the bullet is using Pythagorean theorem.
      if (bullet[i]->type == T_PLAYER_BULLET) {
          max_range = shiptable[player->ship_type].shot_range;
      } else {
          max_range = shiptable[current_enemy->ship_type].shot_range;
      }

      travelled = sqrt(
        pow(bullet[i]->vecPosOrigin.x - bullet[i]->vecPosition.x, 2) +
        pow(bullet[i]->vecPosOrigin.y - bullet[i]->vecPosition.y, 2));

      // did this bullet hit anything?
      if (bullet[i] && bullet[i]->type == T_ENEMY_BULLET) {
        if (isp_check_sprites_collision(sprites,
                                        player->e.sprite_id,
                                        bullet[i]->sprite_id,
                                        FALSE,
                                        FALSE)) {
          isp_destroy_sprite(sprites, bullet[i]->sprite_id);
          free(bullet[i]);
          bullet[i] = NULL;
        }
      }

      if (bullet[i] && bullet[i]->type == T_PLAYER_BULLET) {
        if (isp_check_sprites_collision(sprites,
                                        current_enemy->e.sprite_id,
                                        bullet[i]->sprite_id,
                                        FALSE,
                                        FALSE)) {

          // TODO: calc random damage amount, send damage over to other side.
          // players are authoratitive for thier own objects, so if they hit,
          // we tell the other side to update their damage.

          // damage is random, half to full damage.

          if (current_enemy->is_shielded) {
            dmg = 0;
          } else {
            dmg = randRange(shiptable[player->ship_type].max_dmg * .75,
                            shiptable[player->ship_type].max_dmg);
          }
          current_enemy->hp = current_enemy->hp - dmg;

          i2sPlay("game/explode2.snd");

          send_state_update(BATTLE_OP_TAKE_DMG, dmg);
          printf("HIT for %d Damage, HP: %d / %d\n",
            dmg,
            current_enemy->hp,
            shiptable[current_enemy->ship_type].max_hp
          );

          isp_destroy_sprite(sprites, bullet[i]->sprite_id);
          free(bullet[i]);
          bullet[i] = NULL;

          redraw_enemy_bars();

          putImageFile(getAvatarImage(current_enemy->ship_type, FALSE, 'h',
                       current_enemy->e.faces_right),
                       current_enemy->e.vecPosition.x,
                       current_enemy->e.vecPosition.y);
          chThdSleepMilliseconds(100);

          if (current_enemy->hp <= 0) {
            st='d';
          } else {
            st='n';
          }

          putImageFile(getAvatarImage(current_enemy->ship_type, FALSE, st,
                                      current_enemy->e.faces_right),
                                      current_enemy->e.vecPosition.x,
                                      current_enemy->e.vecPosition.y);
          if (st == 'd')
            chThdSleepMilliseconds(500);
         }
        }
      }

      // check for oob or max distance
      if (bullet[i]) {
        if ((travelled > max_range) ||
            (entity_OOB(bullet[i])) ||
            (check_land_collision(bullet[i]))) {
          // expire bullet
          isp_destroy_sprite(sprites, bullet[i]->sprite_id);
          i2sPlay("game/splash.snd");
          free(bullet[i]);
          bullet[i] = NULL;
          continue;
        }
      }
    }
  }


bool
entity_OOB(ENTITY *e)
{
  // returns TRUE if entity is out of bounds
  if ((e->vecPosition.x >= SCREEN_W - e->size_x) ||
      (e->vecPosition.x < 0) ||
      (e->vecPosition.y < 0) ||
      (e->vecPosition.y >= SCREEN_H - e->size_y))
  {
    return(TRUE);
  }

  return(FALSE);
}

void expire_enemies(void *e)
{
  ENEMY *en = e;
  int    position;

  // we expire 3 seconds earlier than the ble peer checker...
  if (en->ttl < 3)
  {
    // erase the old position
#ifdef DEBUG_ENEMY_TTL
    printf("remove enemy due to ttl was: %d \n", en->ttl);
#endif
    isp_destroy_sprite(sprites, en->e.sprite_id);

    // remove it from the linked list.
    position = enemy_find_ll_pos(enemies, &(en->ble_peer_addr));
    if (position > -1)
    {
      gll_remove(enemies, position);
    }
  }
}

static bool
fire_allowed(ENEMY *e) {
  // returns true if player is allowed to fire a shot.
  if (e->last_shot_ms == 0) return TRUE;

  if (chVTTimeElapsedSinceX(e->last_shot_ms) >=
      TIME_MS2I(shiptable[e->ship_type].shot_msec)){
        return TRUE;
  }

  return FALSE;
}

static void set_ship_sprite(ENEMY *e) {
  BattleHandles *       bh;
  bh = (BattleHandles *)mycontext->priv;
  if (e->is_shielded) {
    if (e == player) {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->pl_s_right : bh->pl_s_left);
    } else {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->ce_s_right : bh->ce_s_left);
    }
  } else {
    if (e == player) {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->pl_right : bh->pl_left);
    } else {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->ce_right : bh->ce_left);
    }
  }

}

static void fire_special(ENEMY *e) {

  // fires a special.
  if (e->energy < shiptable[e->ship_type].special_cost) {
    // you can't afford it.
    i2sPlay("game/error.snd");
    return;
  }
  // charge them, mark last usage
  e->energy = e->energy - shiptable[e->ship_type].special_cost;
  e->last_special_ms = chVTGetSystemTime();

  if (e == player)
    send_state_update(BATTLE_OP_ENG_UPDATE,e->energy);

  // do the appropriate action for this special.
  switch (shiptable[e->ship_type].special_flags) {
    case SP_SHOT_FOURWAY:
      fire_bullet(e,0,-1,TRUE);  // U
      fire_bullet(e,0,1,TRUE);   // D
      fire_bullet(e,-1,0,TRUE);  // L
      fire_bullet(e,1,0,TRUE);   // R
      break;
    case SP_SHOT_FOURWAY_DIAG:
      fire_bullet(e,-1,-1,TRUE); // UL
      fire_bullet(e,1,-1,TRUE);  // UR
      fire_bullet(e,-1,1,TRUE);  // LL
      fire_bullet(e,1,1,TRUE);   // LR
    break;
    case SP_SHIELD:
      e->is_shielded = TRUE;
      if (e == player)
        send_state_update(BATTLE_OP_SET_SHIELD,e->is_shielded);

      e->special_started_at = chVTGetSystemTime();

      set_ship_sprite(e);

      break;
    default:
      break;
  }
}

static void
battle_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
  (void)context;
  ENEMY *               nearest;
  BattleHandles *       bh;
  bp_header_t *         ph;
  GEvent *              pe;
  GEventMouse *         me;
  GEventGWinButton *    be;
  OrchardAppRadioEvent *radio;
  ble_evt_t *           evt;
  ble_gatts_evt_rw_authorize_request_t *rw;
  ble_gatts_evt_write_t *req;

  bp_vs_pkt_t     *pkt_vs;
  bp_state_pkt_t  *pkt_state;
  bp_entity_pkt_t *pkt_entity;
  bp_bullet_pkt_t *pkt_bullet;

  char tmp[40];

  bh = (BattleHandles *)context->priv;

  /* MAIN GAME EVENT LOOP */
  if (event->type == timerEvent)
  {
    /* animtick increments on every frame. It is reset to zero on changeState() */
    animtick++;
    /* some states lack a tick function */
    if (battle_funcs[current_battle_state].tick != NULL)
    {
      battle_funcs[current_battle_state].tick();
    }

    // player and enemy update
    if (current_battle_state == WORLD_MAP)
    {
      if (animtick % (FPS / 2) == 0)
      {
        // refresh world map enemy positions every 500mS
        enemy_list_refresh(enemies, sprites, current_battle_state);
        gll_each(enemies, expire_enemies);
      }

      entity_update(&(player->e), FRAME_DELAY);

      enemy_clearall_blink(enemies);
      nearest = getNearestEnemy(enemies, player);

      if (nearest != NULL)
      {
        nearest->e.blinking = TRUE;
      }

      if (nearest != last_near)
      {
        last_near = nearest;

        // if our state has changed, update the UI
        if (nearest == NULL)
        {
          gwinSetText(bh->ghTitleR, "Find an enemy!", TRUE);
          gwinRedraw(bh->ghTitleR);
        }
        else
        {
          // update UI
          strcpy(tmp, "Engage: ");
          strcat(tmp, nearest->name);
          gwinSetText(bh->ghTitleR, tmp, TRUE);
          gwinRedraw(bh->ghTitleR);
        }
      }
      isp_draw_all_sprites(sprites);
    } /* WORLD_MAP */

    if (current_battle_state == COMBAT)
    {
      // in combat we only have to render the player and enemy */
      entity_update(&(player->e), FRAME_DELAY);
      entity_update(&(current_enemy->e), FRAME_DELAY);

      // move bullets if any
      update_bullets();

      // TBD:check for player vs enemy collision ( ramming speed! )

      if ((player->hp <= 0) || (current_enemy->hp <= 0)) {
        changeState(SHOW_RESULTS);
        return;
      }

      isp_draw_all_sprites(sprites);
    }
  } /* timerEvent */

  // handle uGFX events
  if (event->type == ugfxEvent)
  {
    pe = event->ugfx.pEvent;
    me = (GEventMouse *)event->ugfx.pEvent;

    if (pe->type == GEVENT_TOUCH && me->buttons & GMETA_MOUSE_DOWN) {

      /*
       * When the user touches the screen in the world map and
       * there's an enemy nearby, engage them,
       */

      if (current_battle_state == WORLD_MAP) {
        if ((nearest = enemy_engage(context, enemies, player)) != NULL) {
          /* convert the enemy struct to the larger combat version */
          current_enemy = malloc(sizeof(ENEMY));
          memcpy(current_enemy, nearest, sizeof(ENEMY));
          current_enemy->e.size_x = SHIP_SIZE_ZOOMED;
          current_enemy->e.size_y = SHIP_SIZE_ZOOMED;

          changeState(APPROVAL_WAIT);
          return;
        }
      }

      /*
       * When the user touches the screen in the VS. screen and
       * they haven't chosen a ship yet, trigger a ship select.
       */

      if (current_battle_state == VS_SCREEN &&
          player->ship_locked_in == FALSE) {
        player->ship_locked_in = TRUE;
        i2sPlay("sound/ping.snd");
        send_ship_type(player->ship_type, TRUE);

        if (current_enemy->ship_locked_in && player->ship_locked_in) {
          changeState(COMBAT);
          return;
        } else {
          state_vs_draw_enemy(player, TRUE);
        }
      }

      /* Ignore other touch events for now */
      return;
    }

    if (current_battle_state == APPROVAL_DEMAND)
    {
      if (pe->type == GEVENT_GWIN_BUTTON)
      {
        be = (GEventGWinButton *)pe;
        if (be->gwin == bh->ghACCEPT)
        {
          // we will send an accept packet
          tmp[0] = BLE_IDES_GAMEATTACK_ACCEPT;
          bleGattcWrite(gm_handle.value_handle,
                        (uint8_t *)tmp, 1, FALSE);

          // on return of that packet, we'll have a l2CapConnection
          // and we'll switch to combat state.

          // we keep our gatt connection open as we'll need it to ID
          // the other user.
        }

        if (be->gwin == bh->ghDECLINE)
        {
          // send a decline and go back to the map.
          tmp[0] = BLE_IDES_GAMEATTACK_DECLINE;
          bleGattcWrite(gm_handle.value_handle,
                        (uint8_t *)tmp, 1, FALSE);
          bleGapDisconnect();
          changeState(WORLD_MAP);
        }
      }
    }

  }

  // deal with the radio
  if (event->type == radioEvent)
  {
    radio = (OrchardAppRadioEvent *)&event->radio;
    evt   = &radio->evt;

    // ble notes
    // ble_gap_role (ble_lld.h) indicates where the connection came from.
    // It can be BLE_GAP_ROLE_CENTRAL or BLE_GAP_ROLE_PERIPH.
    // CENTRAL is the one that initiates the connection.
    // PERIPH is the one that accepts.
    switch (radio->type)
    {
    case connectEvent:
      if (current_battle_state == APPROVAL_WAIT)
      {
        // fires when we succeed on a connect i think.
        screen_alert_draw(FALSE, "Sending Challenge...");
        tmp[0] = BLE_IDES_GAMEATTACK_CHALLENGE;
        bleGattcWrite(gm_handle.value_handle,
                      (uint8_t *)tmp, 1, FALSE);
      }
      break;

    case disconnectEvent:
      if (current_battle_state != SHOW_RESULTS) {
        screen_alert_draw(FALSE, "Radio link lost");
        chThdSleepMilliseconds(ALERT_DELAY);
        orchardAppExit ();
      }
      break;

    case gattsReadWriteAuthEvent:
      rw =
        &evt->evt.gatts_evt.params.authorize_request;
      req = &rw->request.write;
      if (rw->request.write.handle ==
          gm_handle.value_handle &&
          req->data[0] == BLE_IDES_GAMEATTACK_ACCEPT)
      {
        screen_alert_draw(FALSE,
                          "Battle Accepted!");
        /* Offer accepted - create L2CAP link */

        /* the attacker makes the connect first, when the l2cap
         * connect comes back we will switch over to a new state */
        bleL2CapConnect(BLE_IDES_BATTLE_PSM);
      }

      if (rw->request.write.handle ==
          gm_handle.value_handle &&
          req->data[0] == BLE_IDES_GAMEATTACK_DECLINE)
      {
        screen_alert_draw(FALSE,
                          "Battle Declined.");
        chThdSleepMilliseconds(2000);
        bleGapDisconnect();

        changeState(WORLD_MAP);
      }

      // we under attack, yo.
      if (rw->request.write.handle ==
          gm_handle.value_handle &&
          req->data[0] == BLE_IDES_GAMEATTACK_CHALLENGE)
      {
        changeState(APPROVAL_DEMAND);
      }
      break;

    case l2capRxEvent:
      // copy the recv'd packet into the buffer for all states to use
      memcpy(bh->rxbuf, radio->pkt, radio->pktlen);

      // VS_SCREEN
      ph = (bp_header_t *)bh->rxbuf;
      //printf("L2CAP: recv packet with type %d, length %d\n",
      //       ph->bp_type,
      //       radio->pktlen);

      if (current_battle_state == VS_SCREEN)
      {
        switch (ph->bp_opcode)
        {
        case BATTLE_OP_SHIP_CONFIRM:
          current_enemy->ship_locked_in = TRUE;
        /* intentional fall-through */
        case BATTLE_OP_SHIP_SELECT:
          pkt_vs = (bp_vs_pkt_t *)&bh->rxbuf;
          current_enemy->ship_type = pkt_vs->bp_shiptype;
          current_enemy->hp        = shiptable[current_enemy->ship_type].max_hp;
          current_enemy->energy    = shiptable[current_enemy->ship_type].max_energy;
          state_vs_draw_enemy(current_enemy, FALSE);

          if (current_enemy->ship_locked_in && player->ship_locked_in)
          {
            changeState(COMBAT);
            return;
          }
          break;
        }
      }

      // COMBAT
      if (current_battle_state == COMBAT)
      {
        switch (ph->bp_opcode)
        {
          case BATTLE_OP_TAKE_DMG:
            // take damage from opponent.
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            player->hp = player->hp - pkt_state->bp_operand;
            i2sPlay("game/explode2.snd");
            if (! player->is_shielded) {
              putImageFile(getAvatarImage(player->ship_type,
                TRUE, 'h', player->e.faces_right),
                player->e.vecPosition.x,
                player->e.vecPosition.y);
              chThdSleepMilliseconds(100);
              if (player->hp <= 0) {
                tmp[0]='d';
              } else {
                tmp[0]='n';
              }
              putImageFile(getAvatarImage(player->ship_type, TRUE, tmp[0],
                player->e.faces_right),
                player->e.vecPosition.x,
                player->e.vecPosition.y);            // player's stats
            } else {
              putImageFile(getAvatarImage(player->ship_type,
                TRUE, 'n', player->e.faces_right),
                player->e.vecPosition.x,
                player->e.vecPosition.y);
              chThdSleepMilliseconds(100);
              putImageFile(getAvatarImage(player->ship_type,
                TRUE, 's', player->e.faces_right),
                player->e.vecPosition.x,
                player->e.vecPosition.y);
            }
            redraw_player_bars();

            // u dead.
            if (player->hp <= 0)
              chThdSleepMilliseconds(250);
          break;
          case BATTLE_OP_SET_SHIELD:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->is_shielded = pkt_state->bp_operand;
            current_enemy->special_started_at = chVTGetSystemTime();
            set_ship_sprite(current_enemy);
            break;
          case BATTLE_OP_ENG_UPDATE:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->energy = pkt_state->bp_operand;
            redraw_player_bars();
          break;
          case BATTLE_OP_CLOCKUPDATE:
            // we are receiving clock from the other badge.
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            state_time_left = pkt_state->bp_operand;
#ifdef DEBUG_BATTLE_STATE
            printf("OP_CLOCKUPDATE: 0x%02x %d\n",
              ph->bp_opcode,
              pkt_state->bp_operand);
#endif
            redraw_combat_clock();
            break;
          case BATTLE_OP_ENTITY_CREATE:
            // enemy is firing a bullet from their current position.
            pkt_bullet = (bp_bullet_pkt_t *)&bh->rxbuf;
            fire_bullet(current_enemy,
              pkt_bullet->bp_dir_x,
              pkt_bullet->bp_dir_y,
              pkt_bullet->bp_is_free);

            // update enemy eng bar
            if (! pkt_bullet->bp_is_free) {
              current_enemy->energy = current_enemy->energy - shiptable[current_enemy->ship_type].shot_cost;
              // redraw energy bar.
              redraw_enemy_bars();
            }
          break;
          case BATTLE_OP_ENTITY_UPDATE:
            // the enemy is updating their position and velocity
            pkt_entity = (bp_entity_pkt_t *)&bh->rxbuf;
            if (pkt_entity->bp_header.bp_type == T_ENEMY)
            {
              // copy the packet in to the current_enemy.
              // players are authoratitive for themselves, always.
              current_enemy->e.vecPosition.x     = pkt_entity->bp_x;
              current_enemy->e.vecPosition.y     = pkt_entity->bp_y;
              current_enemy->e.vecVelocity.x     = pkt_entity->bp_velocity_x;
              current_enemy->e.vecVelocity.y     = pkt_entity->bp_velocity_y;
              current_enemy->e.vecVelocityGoal.x = pkt_entity->bp_velogoal_x;
              current_enemy->e.vecVelocityGoal.y = pkt_entity->bp_velogoal_y;

              if (current_enemy->e.faces_right != pkt_entity->bp_faces_right) {
                current_enemy->e.faces_right = pkt_entity->bp_faces_right;
                set_ship_sprite(current_enemy);
              }

              isp_set_sprite_xy(sprites,
                                current_enemy->e.sprite_id,
                                current_enemy->e.vecPosition.x,
                                current_enemy->e.vecPosition.y);
            }
            break;
          }
      }
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
      // kill and restart our timer so that our timers are in sync
      orchardAppTimer(context, 0, false); // shut down the timer
      // start the timer again
      orchardAppTimer(context, FRAME_DELAY * 1000000, TRUE);

      if (current_enemy == NULL)
      {
        // copy his data over and start the battle.
        current_enemy = getEnemyFromBLE(&ble_peer_addr, NULL);
      }
      changeState(VS_SCREEN);
      break;

    default:
      break;
    }
  }

  // handle ship selection screen (VS_SCREEN) ------------------------------
  if (current_battle_state == VS_SCREEN && player->ship_locked_in == FALSE)
  {
    if (event->type == keyEvent)
    {
      if (event->key.flags == keyPress)
      {
        switch (event->key.code)
        {
        case keyALeft:
          player->ship_type--;
          break;

        case keyARight:
          player->ship_type++;
          break;

        case keyBSelect:
          player->ship_locked_in = TRUE;
          i2sPlay("sound/ping.snd");
          send_ship_type(player->ship_type, TRUE);

          if (current_enemy->ship_locked_in && player->ship_locked_in)
          {
            changeState(COMBAT);
            return;
          }
          else
          {
            state_vs_draw_enemy(player, TRUE);
          }
          break;

        default:
          break;
        }

        // handle rotating selection
        if (player->ship_type == 255)
        {
          player->ship_type = 5;
        }                                                        // underflow
        if (player->ship_type > 5)
        {
          player->ship_type = 0;
        }
        player->hp     = shiptable[player->ship_type].max_hp;
        player->energy = shiptable[player->ship_type].max_energy;

        state_vs_draw_enemy(player, TRUE);
        send_ship_type(player->ship_type, FALSE);
      }
    }
  }
  // end handle ship selection screen (VS_SCREEN) -------------------------

  // handle WORLD MAP and COMBAT  -----------------------------------------

  if (current_battle_state == WORLD_MAP || current_battle_state == COMBAT)
  {
    if (event->type == keyEvent)
    {
      // Stop motion on release.
      if (event->key.flags == keyRelease)
      {
        switch (event->key.code)
        {
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
      }  // keyRelease

      if (event->key.flags == keyPress)
      {
        /* Throttle up! */
        switch (event->key.code)
        {
        case keyALeft:
          player->e.vecVelocityGoal.x = -VGOAL;
          player->e.faces_right       = false;

          if (current_battle_state == COMBAT) {
            set_ship_sprite(player);
          }
          break;

        case keyARight:
          player->e.faces_right       = true;
          player->e.vecVelocityGoal.x = VGOAL;

          if (current_battle_state == COMBAT) {
            set_ship_sprite(player);
          }
          break;

        case keyAUp:
          player->e.vecVelocityGoal.y = -VGOAL;
          break;

        case keyADown:
          player->e.vecVelocityGoal.y = VGOAL;
          break;

        case keyASelect:
          if (current_battle_state == WORLD_MAP)
          {
            // exit
            i2sPlay("sound/click.snd");
            orchardAppExit();
            return;
          }
          break;

        /* weapons system */
        case keyBUp:
          if (current_battle_state == COMBAT && fire_allowed(player))
            fire_bullet(player,0,-1,FALSE);
          break;
        case keyBDown:
          if (current_battle_state == COMBAT && fire_allowed(player))
            fire_bullet(player,0,1,FALSE);
          break;
        case keyBLeft:
          if (current_battle_state == COMBAT && fire_allowed(player))
            fire_bullet(player,-1,0,FALSE);
          break;
        case keyBRight:
          if (current_battle_state == COMBAT && fire_allowed(player))
            fire_bullet(player,1,0,FALSE);
        break;
        case keyBSelect:
          if (current_battle_state == COMBAT) {
            fire_special(player);
          }

          if (current_battle_state == WORLD_MAP)
          {
            if ((nearest = enemy_engage(context, enemies, player)) != NULL)
            {
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
              return;
            }
          }
          break;

          default:
            break;
        }
      }
    }   // keyEvent
  }
  return;
}

static void changeState(battle_state nextstate)
{
  // if we need to switch states, do so.
  if (nextstate == current_battle_state)
  {
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
  if (battle_funcs[current_battle_state].exit != NULL)
  {
    battle_funcs[current_battle_state].exit();
  }

  animtick             = 0;

  // this is the only function that should be changing the state variable.
  current_battle_state = nextstate;

  // enter the new state
  if (battle_funcs[current_battle_state].enter != NULL)
  {
    battle_funcs[current_battle_state].enter();
  }
}

static void free_enemy(void *e)
{
  ENEMY *en = e;

  free(en);
}

static void remove_all_enemies(void)
{
  /* remove all enemies. */
  if (enemies)
  {
    gll_each(enemies, free_enemy);
    gll_destroy(enemies);
    enemies = NULL;
  }
}

static void battle_exit(OrchardAppContext *context)
{
  userconfig *   config = getConfig();
  BattleHandles *bh;

  // kill the timer.
  orchardAppTimer(context, 0, FALSE);

  // free the UI
  bh = (BattleHandles *)context->priv;

  // reset this state or we won't init properly next time.
  // Do not free state-objects here, let the state-exit functions do that!
  changeState(NONE);

  if (bh->fontXS)
  {
    gdispCloseFont(bh->fontXS);
  }

  if (bh->fontLG)
  {
    gdispCloseFont(bh->fontLG);
  }

  if (bh->fontSM)
  {
    gdispCloseFont(bh->fontSM);
  }

  if (bh->fontSTENCIL)
  {
    gdispCloseFont(bh->fontSTENCIL);
  }

  if (bh->ghTitleL)
  {
    gwinDestroy(bh->ghTitleL);
  }

  if (bh->ghTitleR)
  {
    gwinDestroy(bh->ghTitleR);
  }

  geventRegisterCallback(&bh->gl, NULL, NULL);
  geventDetachSource(&bh->gl, NULL);
  geventRegisterCallback(&bh->gl2, NULL, NULL);
  geventDetachSource(&bh->gl2, NULL);

  i2sPlay (NULL);

  // disconnect anything
  if (bh->cid != BLE_L2CAP_CID_INVALID)
    bleL2CapDisconnect (bh->cid);
  bleGapDisconnect ();

  // free players and related objects.
  // the sprite system will have been brought down
  // by the state transition to NONE above.
  if (player)
  {
    free(player);
    player = NULL;
  }

  if (current_enemy)
  {
    free(current_enemy);
    current_enemy = NULL;
  }

  for (int i = 0; i < MAX_BULLETS; i++)
  {
    if (bullet[i])
    {
      free(bullet[i]);
      bullet[i] = NULL;
    }
  }

  last_near = NULL;

  free(context->priv);
  context->priv = NULL;

  // restore the LED pattern from config
  ledSetPattern(config->led_pattern);

  // flush combat status if any
  config->in_combat = 0;
  configSave(config);

  /* Also sync our over-the-air status */
  bleGapUpdateState(config->last_x, config->last_y,
    config->xp, config->level, config->current_ship, config->in_combat);

  return;
}

static void
render_all_enemies(void)
{
  // renders all enemies based on enemy linked list
  // will not update unless enemy moves
}

/* state change and tick functions go here -------------------------------- */
// HANDSHAKE --------------------------------------------------------------
void state_handshake_enter(void)
{
  gdispClear(Black);
  screen_alert_draw(TRUE, "HANDSHAKING...");
  state_time_left = 5;
}

void state_handshake_tick(void)
{
  // in this state we are being attacked and we are holding pending a
  // l2cap connection with a battle in it. It's still possible to TIMEOUT
  // and we have to handle that if it happens. We'l hold for five seconds.
  if (animtick % FPS == 0)
  {
    state_time_left--;
    if (state_time_left <= 0)
    {
      screen_alert_draw(TRUE, "HS TIMEOUT :(");
      changeState(WORLD_MAP);
    }
  }
}

// WORLDMAP ---------------------------------------------------------------
void state_worldmap_enter(void)
{
  userconfig *config = getConfig();

  if (current_enemy != NULL)
  {
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

  // draw player for first time.
  entity_init(&player->e, sprites, SHIP_SIZE_WORLDMAP, SHIP_SIZE_WORLDMAP, T_PLAYER);
  player->e.vecPosition.x = config->last_x;
  player->e.vecPosition.y = config->last_y;
  player->e.visible       = TRUE;

  isp_set_sprite_xy(sprites,
                    player->e.sprite_id,
                    player->e.vecPosition.x,
                    player->e.vecPosition.y);

  sprites->list[player->e.sprite_id].status = ISP_STAT_DIRTY_BOTH;
  isp_draw_all_sprites(sprites);

  // load the enemy list
  enemies = gll_init();
  enemy_list_refresh(enemies, sprites, current_battle_state);

  render_all_enemies();
}

void state_worldmap_tick(void)
{
  userconfig *config = getConfig();

#ifdef ENABLE_MAP_PING
  /* every four seconds (120 frames) go ping */
  if (animtick % (FPS * 4) == 0)
  {
    i2sPlay("game/map_ping.snd");
  }
#endif
  // Update BLE
  // if we moved, we have to update BLE.
  if (player->e.vecPosition.x != player->e.prevPos.x ||
      player->e.vecPosition.y != player->e.prevPos.y)
  {
    bleGapUpdateState((uint16_t)player->e.vecPosition.x,
                      (uint16_t)player->e.vecPosition.y,
                      config->xp,
                      config->level,
                      config->current_ship,
                      config->in_combat);
  }
}

void state_worldmap_exit(void)
{
  BattleHandles *bh;
  userconfig *   config = getConfig();

  // get private memory
  bh = (BattleHandles *)mycontext->priv;

  // store our last position for later.
  config->last_x = player->e.vecPosition.x;
  config->last_y = player->e.vecPosition.y;
  configSave(config);

  if (bh->ghTitleL)
  {
    gwinDestroy(bh->ghTitleL);
    bh->ghTitleL = NULL;
  }

  if (bh->ghTitleR)
  {
    gwinDestroy(bh->ghTitleR);
    bh->ghTitleR = NULL;
  }

  /* wipe enemies */
  remove_all_enemies();

  /* tear down sprite system */
  isp_shutdown(sprites);
}

// APPROVAL_WAIT -------------------------------------------------------------
// we are attacking someone, wait for consent.
void state_approval_wait_enter(void)
{
  // we will now attempt to open a BLE gap connect to the user.
  printf("engage: %x:%x:%x:%x:%x:%x\n",
         current_enemy->ble_peer_addr.addr[5],
         current_enemy->ble_peer_addr.addr[4],
         current_enemy->ble_peer_addr.addr[3],
         current_enemy->ble_peer_addr.addr[2],
         current_enemy->ble_peer_addr.addr[1],
         current_enemy->ble_peer_addr.addr[0]);

  gdispClear(Black);
  screen_alert_draw(FALSE, "Connecting...");

  bleGapConnect(&current_enemy->ble_peer_addr);
  // when the gap connection is accepted, we will attempt a
  // l2cap connect

  animtick = 0;
}

void state_approval_wait_tick(void)
{
  // we shouldn't be in this state for more than five seconds or so.
  if (animtick > NETWORK_TIMEOUT)
  {
    // forget it.
    screen_alert_draw(TRUE, "TIMED OUT");
    chThdSleepMilliseconds(ALERT_DELAY);
    bleGapDisconnect();
    changeState(WORLD_MAP);
  }
}

void state_approval_wait_exit(void)
{
  // no-op.
}

// APPOVAL_DEMAND ------------------------------------------------------------
// Someone is attacking us.
void state_approval_demand_enter(void)
{
  BattleHandles * bh;
  ble_peer_entry *peer;
  char            buf[36];
  int             fHeight;
  GWidgetInit     wi;

  animtick = 0;
  bh       = (BattleHandles *)mycontext->priv;
  gdispClear(Black);

  peer = blePeerFind(ble_peer_addr.addr);
  if (peer == NULL)
  {
    snprintf(buf, 36, "Badge %X:%X:%X:%X:%X:%X",
             ble_peer_addr.addr[5], ble_peer_addr.addr[4],
             ble_peer_addr.addr[3], ble_peer_addr.addr[2],
             ble_peer_addr.addr[1], ble_peer_addr.addr[0]);
  }
  else
  {
    snprintf(buf, 36, "%s", peer->ble_peer_name);
  }

  putImageFile("images/undrattk.rgb", 0, 0);
  i2sPlay("sound/klaxon.snd");

  fHeight = gdispGetFontMetric(bh->fontSM, fontHeight);

  gdispDrawStringBox(0, 50 -
                     fHeight,
                     gdispGetWidth(), fHeight,
                     buf, bh->fontSM, White, justifyCenter);

  gdispDrawStringBox(0, 50, gdispGetWidth(), fHeight,
                     "IS CHALLENGING YOU", bh->fontSM, White, justifyCenter);

  gwinSetDefaultStyle(&RedButtonStyle, FALSE);
  gwinSetDefaultFont(bh->fontSM);
  gwinWidgetClearInit(&wi);

  wi.g.show     = TRUE;
  wi.g.x        = 0;
  wi.g.y        = 210;
  wi.g.width    = 150;
  wi.g.height   = 30;
  wi.text       = "DECLINE";
  bh->ghDECLINE = gwinButtonCreate(0, &wi);

  wi.g.x       = 170;
  wi.text      = "ACCEPT";
  bh->ghACCEPT = gwinButtonCreate(0, &wi);

  gwinSetDefaultStyle(&BlackWidgetStyle, FALSE);
}

void state_approval_demand_tick(void)
{
  if (animtick > NETWORK_TIMEOUT)
  {
    // user chose nothing.
    screen_alert_draw(TRUE, "TIMED OUT");
    chThdSleepMilliseconds(ALERT_DELAY);
    bleGapDisconnect();
    changeState(WORLD_MAP);
  }
}

void state_approval_demand_exit(void)
{
  BattleHandles *bh;

  bh = mycontext->priv;

  gwinDestroy(bh->ghACCEPT);
  gwinDestroy(bh->ghDECLINE);
}

// COMBAT --------------------------------------------------------------------

void state_combat_enter(void)
{
  char        fnbuf[30];
  int         newmap;
  userconfig *config = getConfig();
  BattleHandles * bh;
  bh       = (BattleHandles *)mycontext->priv;

  // we're in combat now, send our last advertisement.
  state_time_left   = 120;
  config->in_combat = 1;
  configSave(config);

  bleGapUpdateState(config->last_x,
                    config->last_y,
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
  if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
  {
    newmap = getMapTile(&player->e);
    printf("BLECENTRAL: map %d from player at %f, %f\n",
           newmap,
           player->e.vecPosition.x,
           player->e.vecPosition.y
           );
  }
  else
  {
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
  entity_init(&player->e, sprites, SHIP_SIZE_ZOOMED, SHIP_SIZE_ZOOMED, T_PLAYER);
  entity_init(&current_enemy->e, sprites, SHIP_SIZE_ZOOMED, SHIP_SIZE_ZOOMED, T_ENEMY);

  // load sprites
  bh->pl_left  = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'n', false));
  bh->pl_right = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'n', true));
  bh->ce_left  = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'n', false));
  bh->ce_right = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'n', true));

  // player - need shield if using Cruiser.
  if (player->ship_type == SHIP_CRUISER) {
    bh->pl_s_left  = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 's', false));
    bh->pl_s_right = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 's', true));
  }

  // enemy - need shield if using Cruiser.
  if (current_enemy->ship_type == SHIP_CRUISER) {
    bh->ce_s_left  = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 's', false));
    bh->ce_s_right = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 's', true));
  }


  /* move the player to the starting position */
  if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
  {
    // Attacker (the central) gets left side, and faces right.
    player->e.vecPosition.x        = ship_init_pos_table[newmap].attacker.x;
    player->e.vecPosition.y        = ship_init_pos_table[newmap].attacker.y;
    current_enemy->e.vecPosition.x = ship_init_pos_table[newmap].defender.x;
    current_enemy->e.vecPosition.y = ship_init_pos_table[newmap].defender.y;
    player->e.faces_right          = true;
    current_enemy->e.faces_right   = false;
  }
  else
  {
    // Defender gets the right side, and faces left.
    player->e.vecPosition.x        = ship_init_pos_table[newmap].defender.x;
    player->e.vecPosition.y        = ship_init_pos_table[newmap].defender.y;
    current_enemy->e.vecPosition.x = ship_init_pos_table[newmap].attacker.x;
    current_enemy->e.vecPosition.y = ship_init_pos_table[newmap].attacker.y;
    player->e.faces_right          = false;
    current_enemy->e.faces_right   = true;
  }

  // draw player
  set_ship_sprite(player);

  isp_set_sprite_xy(sprites,
                    player->e.sprite_id,
                    player->e.vecPosition.x,
                    player->e.vecPosition.y);

  // draw enemy
  set_ship_sprite(current_enemy);

  isp_set_sprite_xy(sprites,
                    current_enemy->e.sprite_id,
                    current_enemy->e.vecPosition.x,
                    current_enemy->e.vecPosition.y);

  isp_draw_all_sprites(sprites);

}

void redraw_combat_clock(void) {
  // display the clock.
  char tmp[20];
  int min;
  BattleHandles *bh;
  bh = mycontext->priv;

  min = state_time_left / 60;

  sprintf(tmp, "%02d:%02d",
          min,
          state_time_left - (min * 60));

  // update clock
  drawBufferedStringBox(&bh->c_pxbuf,
                        130,
                        0,
                        60,
                        gdispGetFontMetric(bh->fontSM, fontHeight) + 2,
                        tmp,
                        bh->fontSM,
                        state_time_left > 10 ? Yellow : Red,
                        justifyCenter);
}

void update_combat_clock(void) {
  // if we are the master then we send clock.
  if (animtick % FPS == 0) {
      state_time_left--;
      send_state_update(BATTLE_OP_CLOCKUPDATE, state_time_left);
      redraw_combat_clock();
  }

  if (state_time_left == 0)
  {
    // time up.
    send_state_update(BATTLE_OP_CLOCKUPDATE, state_time_left);
    send_state_update(BATTLE_OP_ENDGAME, 0);
    i2sPlay("sound/foghrn.snd");
    changeState(SHOW_RESULTS);
  }
}

void regen_energy(ENEMY *e) {
  // regen energy
  if (e->energy != shiptable[e->ship_type].max_energy) {
    e->energy = e->energy + shiptable[e->ship_type].energy_recharge_rate;

    if (e->energy > shiptable[e->ship_type].max_energy) {
      e->energy = shiptable[e->ship_type].max_energy;
    }

    if (e == player) {
      redraw_player_bars();
    }

    if (e == current_enemy) {
      redraw_enemy_bars();
    }
  }
}

void check_special_timeouts(ENEMY *e) {
  if ((e->ship_type == SHIP_CRUISER) &&
      (e->is_shielded) &&
      (chVTTimeElapsedSinceX(e->special_started_at) >=
        TIME_MS2I(shiptable[e->ship_type].max_special_ttl))) {
          e->is_shielded = FALSE;
          set_ship_sprite(e);
  }
}

void state_combat_tick(void)
{
  if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
    update_combat_clock();

  if (player == NULL)
    return;

  // update motion
  if (player->e.vecPosition.x != player->e.prevPos.x ||
      player->e.vecPosition.y != player->e.prevPos.y)
  {
    send_position_update(
      player->e.id,
      BATTLE_OP_ENTITY_UPDATE,
      T_ENEMY,
      &(player->e)
      );
  }

  // regen energy
  regen_energy(player);
  regen_energy(current_enemy);

  check_special_timeouts(player);
  check_special_timeouts(current_enemy);

  // force sprite redraw
  isp_draw_all_sprites(sprites);
}

void state_combat_exit(void)
{
  BattleHandles *bh;
  bh = mycontext->priv;

  free(bh->l_pxbuf);
  bh->l_pxbuf = NULL;

  free(bh->c_pxbuf);
  bh->c_pxbuf = NULL;

  free(bh->r_pxbuf);
  bh->r_pxbuf = NULL;

  // clear spholders
  isp_destroy_spholder(bh->pl_left);
  bh->pl_left = NULL;
  isp_destroy_spholder(bh->pl_right);
  bh->pl_right = NULL;
  isp_destroy_spholder(bh->ce_left);
  bh->ce_left = NULL;
  isp_destroy_spholder(bh->ce_right);
  bh->ce_right = NULL;

  /* tear down sprite system */
  isp_shutdown(sprites);
}

static uint16_t calc_xp_gain(uint8_t won) {
  userconfig *config = getConfig();
  float factor = 1;

  int8_t leveldelta;

  leveldelta = current_enemy->level - config->level;
  // range:
  //
  //  >=  3 = 2x XP     RED
  //  >=  2 = 1.50x XP  ORG
  //  >=  1 = 1.25x XP  YEL
  //  ==  0 = 1x XP     GREEN
  //  <= -1 = .75x XP   GREY
  //  <= -2 = .25x XP   GREY
  //  <= -3 = 0 XP      GREY

  // you killed someone harder than you
  if (leveldelta >= 3)
    factor = 2;
  else
    if (leveldelta >= 2)
      factor = 1.50;
    else
      if (leveldelta >= 1)
        factor = 1.25;

  // you killed someone weaker than you
  if (leveldelta <= -3)
    factor = 0;
  else
    if (leveldelta <= -2)
      factor = .25;
    else
      if (leveldelta == -1)
        factor = .75;

  if (won) {
    return (80 + ((config->level) * 16)) * factor;
  }
  else {
    // someone killed you, so you get much less XP, but we will give
    // some extra XP if someone higher than you killed you.
    if (factor < 1)
      factor = 1;

    return (10 + ((config->level) * 16)) * factor;
  }
}

static void state_show_results_enter(void) {
  char tmp[40];
  userconfig *config = getConfig();
  int xpgain = 0;
  BattleHandles *bh;
  // free the UI
  bh = (BattleHandles *)mycontext->priv;

  // figure out who won
  if (player->hp > current_enemy->hp) {
      xpgain = calc_xp_gain(TRUE);
      config->won++;
      sprintf(tmp, "YOU WIN! (+%d XP)",xpgain);
  } else {
    // record time of death
    config->lastdeath = chVTGetSystemTime();

    // reward (some) XP and exit
    xpgain = calc_xp_gain(FALSE);
    config->lost++;
    sprintf(tmp, "YOU LOSE! (+%d XP)",xpgain);
  }

  if (player->hp == current_enemy->hp) {
    // Everybody wins.
    xpgain = calc_xp_gain(TRUE);
    config->lost++;
    sprintf(tmp, "TIE! (+%d XP)",xpgain);
  }

  config->xp += xpgain;
  configSave(config);

  // it's now safe to tear down the connection.
  // we do not alarm for disconect events in SHOW_RESULTS
  if (bh->cid != BLE_L2CAP_CID_INVALID)
    bleL2CapDisconnect (bh->cid);
  bleGapDisconnect();

  // award xp here.
  screen_alert_draw(false, tmp);
  chThdSleepMilliseconds(ALERT_DELAY * 2);

  // did we level up?
  if ((config->level + 1 != calc_level(config->xp)) && (config->level + 1 != LEVEL_CAP)) {
    changeState(LEVELUP);
    return;
  }

  orchardAppExit();
}

static void state_levelup_enter(void)
{
  BattleHandles *p      = mycontext->priv;
  userconfig *   config = getConfig();
  char           tmp[40];
  int            curpos;

  gdispClear(Black);
  i2sPlay("sound/levelup.snd");
  putImageFile("game/map-07.rgb", 0, 0);

  // insiginia
  sprintf(tmp, "game/rank-%d.rgb", config->level + 1);
  putImageFile(tmp, 35, 79);
  gdispDrawBox(35, 79, 53, 95, Grey);

  curpos = 10;
  gdispDrawStringBox(35,
                     curpos,
                     300,
                     gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                     "RANK UP!",
                     p->fontSTENCIL,
                     Yellow,
                     justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontSTENCIL, fontHeight);

  gdispDrawStringBox(35,
                     curpos,
                     300,
                     gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                     rankname[config->level + 1],
                     p->fontSTENCIL,
                     Yellow,
                     justifyLeft);


  if (config->level + 1 < 6)
  {
    // this unlocks a ship
    putImageFile(getAvatarImage(config->level + 1, TRUE, 'n', FALSE),
                 270, 79);
    gdispDrawBox(270, 79, 40, 40, White);
    curpos = 79;
    strncpy(tmp, shiptable[config->level + 1].type_name, 40);
    strntoupper(tmp, 40);

    gdispDrawStringBox(110,
                       curpos,
                       210,
                       gdispGetFontMetric(p->fontSM, fontHeight),
                       tmp,
                       p->fontSM,
                       White,
                       justifyLeft);

    curpos = curpos + gdispGetFontMetric(p->fontSM, fontHeight) + 2;

    gdispDrawStringBox(110,
                       curpos,
                       210,
                       gdispGetFontMetric(p->fontSM, fontHeight),
                       "UNLOCKED!",
                       p->fontSM,
                       White,
                       justifyLeft);
    curpos = 145;
  }
  else
  {
    // we'll draw the next level message in it's place.
    curpos = 79;
  }
  gdispDrawStringBox(110,
                     curpos,
                     210,
                     gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                     "NEXT LEVEL",
                     p->fontSTENCIL,
                     Yellow,
                     justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontSTENCIL, fontHeight) + 2;

  sprintf(tmp, "AT %d XP", xp_for_level(config->level + 2));
  gdispDrawStringBox(110,
                     curpos,
                     210,
                     gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                     tmp,
                     p->fontSTENCIL,
                     Yellow,
                     justifyLeft);
  config->level++;
  configSave(config);

  chThdSleepMilliseconds(ALERT_DELAY);
  orchardAppExit();
}

static void state_vs_draw_enemy(ENEMY *e, bool is_player)
{
  /* draw the player's ship and stats */
  BattleHandles *p = mycontext->priv;

  int curpos;
  int curpos_x;

  char tmp[40];

  if (is_player)
  {
    curpos_x = 60;
  }
  else
  {
    curpos_x = 220;
  }

  // us
  putImageFile(getAvatarImage(e->ship_type, is_player, 'n', TRUE), curpos_x, 89);
  gdispDrawBox(curpos_x, 89, 40, 40, White);

  if (is_player)
  {
    // clear left side
    gdispFillArea(0, 95,
                  59, 30,
                  HTML2COLOR(0x151285));

    // clear name area
    gdispFillArea(0, 132,
                  160, gdispGetFontMetric(p->fontXS, fontHeight),
                  HTML2COLOR(0x151285));
    curpos_x = 0;
  }
  else
  {
    // clear right side
    gdispFillArea(261, 95,
                  59, 26,
                  HTML2COLOR(0x151285));

    curpos_x = 262;
  }

  // our stats
  curpos = 95;

  sprintf(tmp, "HP %d", e->hp);
  gdispDrawStringBox(curpos_x,
                     curpos,
                     58,
                     gdispGetFontMetric(p->fontXS, fontHeight),
                     tmp,
                     p->fontXS,
                     White,
                     is_player ? justifyRight : justifyLeft);

  curpos = curpos + gdispGetFontMetric(p->fontXS, fontHeight);
  sprintf(tmp, "EN %d", e->energy);
  gdispDrawStringBox(curpos_x,
                     curpos,
                     58,
                     gdispGetFontMetric(p->fontXS, fontHeight),
                     tmp,
                     p->fontXS,
                     White,
                     is_player ? justifyRight : justifyLeft);
  if (is_player)
  {
    curpos_x = 0;
  }
  else
  {
    curpos_x = 160;
  }

  // clear name area
  gdispFillArea(curpos_x, 130,
                160, gdispGetFontMetric(p->fontXS, fontHeight),
                HTML2COLOR(0x151285));
  // draw name
  gdispDrawStringBox(curpos_x,
                     130,
                     160,
                     gdispGetFontMetric(p->fontXS, fontHeight),
                     shiptable[e->ship_type].type_name,
                     p->fontXS,
                     White,
                     justifyCenter);

  if (!e->ship_locked_in)
  {
    gdispFillArea(curpos_x + 25, 160, 110, 30, Red);
    gdispDrawStringBox(curpos_x + 25,
                       164,
                       110,
                       gdispGetFontMetric(p->fontSM, fontHeight),
                       "WAITING",
                       p->fontSM,
                       White,
                       justifyCenter);
  }
  else
  {
    gdispFillArea(curpos_x + 25, 160, 110, 30, Green);
    gdispDrawStringBox(curpos_x + 25,
                       164,
                       110,
                       gdispGetFontMetric(p->fontSM, fontHeight),
                       "READY",
                       p->fontSM,
                       White,
                       justifyCenter);
  }
}

static void send_state_update(uint16_t opcode, uint16_t operand)
{
  BattleHandles *p;

  // get private memory
  p = (BattleHandles *)mycontext->priv;
  bp_state_pkt_t pkt;

  memset(p->txbuf, 0, sizeof(p->txbuf));
  pkt.bp_header.bp_opcode = opcode;
  pkt.bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt.bp_operand = operand;
  pkt.bp_pad = 0xffff;
  memcpy(p->txbuf, &pkt, sizeof(bp_state_pkt_t));

  if (bleL2CapSend((uint8_t *)p->txbuf, sizeof(bp_state_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_ship_type(uint16_t type, bool final)
{
  BattleHandles *p;

  // get private memory
  p = (BattleHandles *)mycontext->priv;
  bp_vs_pkt_t pkt;

  memset(p->txbuf, 0, sizeof(p->txbuf));
  pkt.bp_header.bp_opcode = final ? BATTLE_OP_SHIP_CONFIRM : BATTLE_OP_SHIP_SELECT;
  pkt.bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt.bp_shiptype         = type;
  memcpy(p->txbuf, &pkt, sizeof(bp_vs_pkt_t));

  if (bleL2CapSend((uint8_t *)p->txbuf, sizeof(bp_vs_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_position_update(uint16_t id, uint8_t opcode, uint8_t type, ENTITY *e)
{
  BattleHandles *p;

  // get private memory
  p = (BattleHandles *)mycontext->priv;
  bp_entity_pkt_t pkt;

  memset(p->txbuf, 0, sizeof(p->txbuf));
  pkt.bp_header.bp_id = id;

  // pkt.bp_header.bp_shiptype = 0x0; // we ignore this on update/create
  pkt.bp_header.bp_opcode = opcode;
  pkt.bp_header.bp_type   = type; // always enemy.

  pkt.bp_x          = e->vecPosition.x;
  pkt.bp_y          = e->vecPosition.y;
  pkt.bp_velocity_x = e->vecVelocity.x;
  pkt.bp_velocity_y = e->vecVelocity.y;
  pkt.bp_velogoal_x = e->vecVelocityGoal.x;
  pkt.bp_velogoal_y = e->vecVelocityGoal.y;
  pkt.bp_faces_right = e->faces_right;

  memcpy(p->txbuf, &pkt, sizeof(bp_entity_pkt_t));

  if (bleL2CapSend((uint8_t *)p->txbuf, sizeof(bp_entity_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_bullet_create(int16_t seq, ENTITY *e, int16_t dir_x, int16_t dir_y, uint8_t is_free)
{
  BattleHandles *p;
  bp_bullet_pkt_t *pkt;

  // get private memory
  p = (BattleHandles *)mycontext->priv;

  pkt = (bp_bullet_pkt_t *)p->txbuf;
  pkt = &pkt[seq];
  memset(pkt, 0, sizeof(bp_bullet_pkt_t));

  pkt->bp_header.bp_opcode = BATTLE_OP_ENTITY_CREATE;
  pkt->bp_header.bp_type   = T_ENEMY; // always enemy.

  pkt->bp_header.bp_id = e->id;

  pkt->bp_dir_x      = dir_x;
  pkt->bp_dir_y      = dir_y;
  pkt->bp_is_free    = is_free;

  if (bleL2CapSend((uint8_t *)pkt, sizeof(bp_bullet_pkt_t)) != NRF_SUCCESS) {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void state_vs_enter(void)
{
  BattleHandles *p      = mycontext->priv;
  userconfig    *config = getConfig();

  state_time_left = 20;

  putImageFile("game/world.rgb", 0, 0);

  if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
  {
    // Was gonna play this on both devices, but then
    // it sounds out of sync and crazy.
    i2sPlay("sound/ffenem.snd");
  }

  // boxes
  gdispFillArea(0, 14, 320, 30, Black);
  gdispFillArea(0, 50, 320, 100, HTML2COLOR(0x151285));
  gdispFillArea(0, 199, 320, 30, Black);

  gdispDrawStringBox(0,
                     14,
                     320,
                     gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                     "CONNECTED! CHOOSE SHIPS!",
                     p->fontSTENCIL,
                     Yellow,
                     justifyCenter);


  // names
  gdispDrawStringBox(0,
                     60,
                     160,
                     gdispGetFontMetric(p->fontSM, fontHeight),
                     config->name,
                     p->fontSM,
                     White,
                     justifyCenter);
  gdispDrawStringBox(160,
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
  send_ship_type(player->ship_type, FALSE);
}

static void state_vs_tick(void)
{
  // update the clock
  BattleHandles *p = mycontext->priv;
  char           tmp[40];

  if (animtick % FPS == 0)
  {
    state_time_left--;
    sprintf(tmp, ":%02d",
            state_time_left);

    gdispFillArea(150,
                  199,
                  100,
                  gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                  Black);

    gdispDrawStringBox(0,
                       199,
                       320,
                       gdispGetFontMetric(p->fontSTENCIL, fontHeight),
                       tmp,
                       p->fontSTENCIL,
                       Yellow,
                       justifyCenter);
  }

  if (state_time_left == 0)
  {
    // time up -- force to next screen.
    i2sPlay("game/engage.snd");
    changeState(COMBAT);
  }
}

// -------------------------------------------------------------------------

orchard_app("Sea Battle",
            "icons/ship.rgb",
            0,
            battle_init, battle_start,
            battle_event, battle_exit, 1);
