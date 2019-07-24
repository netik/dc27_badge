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
#include "unlocks.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"
#include "strutil.h"

#include "slaballoc.h"

// debugging defines ---------------------------------------
// debugs the discovery process
#undef DEBUG_ENEMY_DISCOVERY
// track battle state and l2cap battle events
#undef DEBUG_BATTLE_STATE
// debug TTL of objects
#undef DEBUG_ENEMY_TTL
// debug map selection
#undef DEBUG_BLE_MAPS
// debug hits and mines
#undef DEBUG_WEAPONS

// end debug defines ---------------------------------------
#define ENABLE_MAP_PING    1        // if you want the sonar sounds

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
static void send_bullet_create(ENTITY *e, int16_t dir_x, int16_t dir_y, uint8_t is_free);
static void send_state_update(uint16_t opcode, uint16_t operand);
static void redraw_combat_clock(void);
static void set_ship_sprite(ENEMY *e);
static void send_mine_create(ENTITY *e);
bool entity_OOB(ENTITY *e);
/* end protos */

/* helpers */
#define PLAYER_MAX_HP (player->unlocks & UL_PLUSHP ? (uint16_t)((float)shiptable[player->ship_type].max_hp * 1.1) : shiptable[player->ship_type].max_hp)
#define ENEMY_MAX_HP  (current_enemy->unlocks & UL_PLUSHP ? (uint16_t)((float)shiptable[current_enemy->ship_type].max_hp * 1.1): shiptable[current_enemy->ship_type].max_hp)

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
    PLAYER_MAX_HP,
    player->hp, TRUE, FALSE);

  ledBar((float)( (float) player->hp  /
                  (float)PLAYER_MAX_HP ),
                  TRUE,
                  -1,
                  30,
                  8,
                  0);

  drawProgressBar(0, 34, 120, 6,
    shiptable[player->ship_type].max_energy, player->energy, FALSE, FALSE);

  // energy bar is in orange at top for player
  ledBar((float)( (float)player->energy /
                  (float)shiptable[player->ship_type].max_energy ),
                  FALSE,
                  17,
                  -1,
                  6,
                  0x000080);
}

static void redraw_enemy_bars(void) {
  // enemy stats
  drawProgressBar(200, 26, 120, 6,
    ENEMY_MAX_HP,
    current_enemy->hp, TRUE, TRUE);

  //enemy HP
  ledBar((float)( (float)current_enemy->hp /
                  (float)ENEMY_MAX_HP),
                  FALSE,
                  0,
                  -1,
                  8,
                  0);

  drawProgressBar(200, 34, 120, 6,
    shiptable[current_enemy->ship_type].max_energy,
    current_enemy->energy, FALSE, TRUE);
    // energy bar is in orange at top for player

  ledBar((float)( (float)current_enemy->energy /
                  (float)shiptable[current_enemy->ship_type].max_energy),
                  TRUE,
                  -1,
                  14,
                  7,
                  0xff8000);
}


static void entity_update(ENTITY *p, float dt)
{
  if ((p->ttl > -1) && (p->visible))
  {
    p->ttl--;
    if (p->ttl == 0)
    {
      p->visible = FALSE;
      return;
    }
  }

  // store our position
  p->prevPos.x = p->vecPosition.x;
  p->prevPos.y = p->vecPosition.y;

  p->vecVelocity.x = approach(p->vecVelocityGoal.x,
                              p->vecVelocity.x,
                              dt * p->vapproach);

  p->vecVelocity.y = approach(p->vecVelocityGoal.y,
                              p->vecVelocity.y,
                              dt * p->vapproach);

  p->vecPosition.x = p->vecPosition.x + p->vecVelocity.x * dt * p->vmult;
  p->vecPosition.y = p->vecPosition.y + p->vecVelocity.y * dt * p->vmult;

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
        if (current_battle_state == COMBAT) {
           if (p->type == T_PLAYER && !player->is_shielded) {
             i2sPlay("game/aground.snd");
             player->hp = player->hp - (int)((float)PLAYER_MAX_HP * 0.1);
             send_state_update(BATTLE_OP_HP_UPDATE, player->hp);
             redraw_player_bars();
           }
        }

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
  player->hp             = PLAYER_MAX_HP;
  player->energy         = shiptable[config->current_ship].max_energy;
  player->xp             = config->xp;
  player->ship_type      = config->current_ship;
  player->ship_locked_in = FALSE;
  player->ttl            = -1;
  player->last_shot_ms   = 0;
  player->unlocks        = config->unlocks;
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

  // initialize the slab allocator

  sl_init();

  // turn off the LEDs
  ledSetPattern(LED_PATTERN_WORLDMAP);
  led_clear();
  led_show();

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
lay_mine(ENEMY *e) {
  int count = 0;

  if (e == player) {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullet[i] != NULL && bullet[i]->type == T_PLAYER_MINE) {
        count++;
      }
    }

    // only one ship can mine, so six max.
    if (count >= MAX_MINES) {
      // no more for you.
      return;
    }

    // we can lay.
    i2sPlay("sound/click.snd");
  }

  for (int i = 0; i < MAX_BULLETS; i++)
  {
    // find the first available bullet structure
    // create a new entity and fire something from it.
    if (bullet[i] == NULL) {
      bullet[i] = malloc(sizeof(ENTITY));
      entity_init(bullet[i],
        sprites,
        MINE_SIZE,
        MINE_SIZE,
        e == player ? T_PLAYER_MINE : T_ENEMY_MINE);
      bullet[i]->visible = TRUE;
      bullet[i]->vecPosition.x = e->e.vecPosition.x;
      bullet[i]->vecPosition.y = e->e.vecPosition.y;

      // center the mine under the ship.
      bullet[i]->vecPosition.x += ((SHIP_SIZE_ZOOMED/2) - (MINE_SIZE/2));
      bullet[i]->vecPosition.y += ((SHIP_SIZE_ZOOMED/2) - (MINE_SIZE/2));

      // mines have a TTL.
      bullet[i]->ttl  = 5 * FPS;

      isp_set_sprite_xy(sprites,
                        bullet[i]->sprite_id,
                        bullet[i]->vecPosition.x,
                        bullet[i]->vecPosition.y);

      if (e == player) {
        send_mine_create(bullet[i]);
      }
#ifdef DEBUG_WEAPONS
      printf("lay mine at %f, %f with ttl %d\n",
        bullet[i]->vecPosition.x,
        bullet[i]->vecPosition.x,
        bullet[i]->ttl);
#endif
      return;
    }
  }
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
        send_bullet_create(bullet[i], dir_x, dir_y, is_free);
      }

      i2sPlay("game/shot.snd");
      return;
    }
  }
  // We failed to fire the shot becasue we're out of bullet memory.
  // this shouldn't ever happen, but we'll make some noise anyway.
  i2sPlay("game/error.snd");

}

void show_hit(ENEMY *e) {
  BattleHandles *bh;
  bh = (BattleHandles *)mycontext->priv;

  // show the hit for 100ms or so
  if (e == player) {
    isp_set_sprite_from_spholder(sprites,
      e->e.sprite_id,
      e->e.faces_right ? bh->pl_h_right : bh->pl_h_left);
  } else {
    if (e->is_cloaked) {
      isp_show_sprite(sprites, e->e.sprite_id);
    }
    isp_set_sprite_from_spholder(sprites,
      e->e.sprite_id,
      e->e.faces_right ? bh->ce_h_right : bh->ce_h_left);
  }
  isp_draw_all_sprites(sprites);
  chThdSleepMilliseconds(100);

  // restore ship
  set_ship_sprite(e);
  if (e != player) {
    if (e->is_cloaked) {
      isp_hide_sprite(sprites, e->e.sprite_id);
    }
  }
}

void update_bullets(void) {
  int i;
  uint16_t dmg;
  uint16_t max_range,travelled;

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

      // manage the mine, first handle TTL. both sides can do that.
      if (bullet[i] && ( bullet[i]->type == T_ENEMY_MINE ||
                         bullet[i]->type == T_PLAYER_MINE) ) {
        if (bullet[i]->ttl == 0) {
          // mine has expired
          i2sPlay("game/splash.snd");
          isp_destroy_sprite(sprites, bullet[i]->sprite_id);
          free(bullet[i]);
          bullet[i] = NULL;
        }
      }

      // did this bullet hit anything? if so we remove it. The enemy will
      // tell us the damage in a few.
      if (bullet[i] && (bullet[i]->type == T_ENEMY_BULLET ||
                        bullet[i]->type == T_ENEMY_MINE)) {
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

      if (bullet[i] && (bullet[i]->type == T_PLAYER_BULLET ||
                        bullet[i]->type == T_PLAYER_MINE)) {
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

          // if you are submerged, then you get hit for 1.5 x
          if (current_enemy->is_cloaked) {
            dmg = dmg * 1.5;
          }

          // mines do the ship's damage and 25% more!
          if (bullet[i]->type == T_PLAYER_MINE) {
              dmg = dmg * 1.25;
          }

          // handle unlocks
          if (current_enemy->unlocks & UL_PLUSDEF) {
            dmg = dmg * .9;

          }
          if (player->unlocks & UL_PLUSDMG) {
            dmg = dmg * 1.1;
          }

          current_enemy->hp = current_enemy->hp - dmg;

          i2sPlay("game/explode2.snd");

          send_state_update(BATTLE_OP_TAKE_DMG, dmg);
#ifdef DEBUG_WEAPONS
          printf("HIT for %d Damage, HP: %d / %d\n",
            dmg,
            current_enemy->hp,
            ENEMY_MAX_HP
          );
#endif
          isp_destroy_sprite(sprites, bullet[i]->sprite_id);
          free(bullet[i]);
          bullet[i] = NULL;

          redraw_enemy_bars();
          show_hit(current_enemy);
         }
        }
      }

      // check for oob or max distance
      if (bullet[i]) {
        if (bullet[i]->type != T_PLAYER_MINE &&
            bullet[i]->type != T_ENEMY_MINE) {
          // bullets can max out their range
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
}

bool
entity_OOB(ENTITY *e)
{
  // returns TRUE if entity is out of bounds
  if ((e->vecPosition.x >= (SCREEN_W-1) - e->size_x) ||
      (e->vecPosition.x < 0) ||
      (e->vecPosition.y < 0) ||
      (e->vecPosition.y >= (SCREEN_H-1) - e->size_y))
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

  // this logic is a bit insnae, but it's the only
  // place that we calculate the sprite look per player.

  // handle healing - make ship green
  if (e->is_healing) {
    if (e == player) {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->pl_g_right : bh->pl_g_left);
    } else {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->ce_g_right : bh->ce_g_left);
    }
    return;
  }

  // teleporting
  if (e->is_teleporting) {
    if (e == player) {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->pl_t_right : bh->pl_t_left);
    } else {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->ce_t_right : bh->ce_t_left);
    }
    return;
  }

  // cloaked
  if (e->is_cloaked) {
    if (e == player) {
      isp_set_sprite_from_spholder(sprites,
        e->e.sprite_id,
        e->e.faces_right ? bh->pl_u_right : bh->pl_u_left);
    } else {
      // for subs we switch the player to the submerged icon
      // and we hide the sprite on the other player's display.
      // dive! dive!
      isp_hide_sprite(sprites,e->e.sprite_id);
    }
    return;
  }

  // shieled puts an outline around the cruiser
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

    // are you dead or alive?
    if (e->hp <= 0) {
      putImageFile(getAvatarImage(e->ship_type,
        TRUE, 'd', e->e.faces_right),
        e->e.vecPosition.x,
        e->e.vecPosition.y);
      chThdSleepMilliseconds(500);
      return;
    }

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
  // if you've got the submarine and you're cloaked, then this
  // operation will uncloak you.
  if (e->ship_type != SHIP_SUBMARINE && e->is_cloaked == FALSE) {
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
  }

  // do the appropriate action for this special.
  switch (shiptable[e->ship_type].special_flags) {
    case SP_CLOAK:
      // this toggles the cloaking.
      e->is_cloaked = !e->is_cloaked;
      if (e == player)
        send_state_update(BATTLE_OP_SET_CLOAK,e->is_cloaked);
      e->special_started_at = chVTGetSystemTime();
      set_ship_sprite(e);
      break;
    case SP_HEAL:
      e->is_healing = TRUE;
      if (e == player)
        send_state_update(BATTLE_OP_SET_HEAL,e->is_healing);
      e->special_started_at = chVTGetSystemTime();
      set_ship_sprite(e);
      break;
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
    case SP_MINE:
      lay_mine(e);
      break;
    case SP_SHIELD:
      e->is_shielded = TRUE;
      if (e == player)
        send_state_update(BATTLE_OP_SET_SHIELD,e->is_shielded);
      e->special_started_at = chVTGetSystemTime();
      set_ship_sprite(e);
      break;
    case SP_TELEPORT:
      e->is_teleporting = TRUE;
      if (e == player)
        send_state_update(BATTLE_OP_SET_TELEPORT,e->is_teleporting);
      e->special_started_at = chVTGetSystemTime();
      i2sPlay("sound/levelup.snd");
      set_ship_sprite(e);
      break;
    default:
      break;
  }
}

static uint16_t
getPrevShip(ENEMY *e) {
  int allowed[10];
  int x;

  memset(&allowed,0,sizeof(allowed));

  for (x=0; x < (e->level + 1 > 6 ? 6 : e->level + 1); x++) {
    allowed[x] = TRUE;
  }

  if (e->unlocks & UL_SHIP1) {
    allowed[6] = TRUE;
  }

  if (e->unlocks & UL_SHIP2) {
    allowed[7] = TRUE;
  }

  for (x = e->ship_type - 1; x > -1; x--) {
    if (allowed[x]) return x;
  }

  if (allowed[7]) {
    return 7;
  } else {
    if (allowed[6]) {
      return 6;
    }
  }

  return e->level;
}

static uint16_t
getNextShip(ENEMY *e) {

  int allowed[10];
  int x;

  memset(&allowed,0,sizeof(allowed));
  for (x=0; x < (e->level + 1 > 6 ? 6 : e->level + 1); x++) {
    allowed[x] = TRUE;
  }

  if (e->unlocks & UL_SHIP1) {
    allowed[6] = TRUE;
  }

  if (e->unlocks & UL_SHIP2) {
    allowed[7] = TRUE;
  }

  for (x = e->ship_type + 1; x < 10; x++) {
    if (allowed[x]) return x;
  }

  return 0;
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
  ble_l2cap_evt_ch_tx_t * tx;

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

      // only the master controls the game
      if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
      {
        if ((player->hp <= 0) || (current_enemy->hp <= 0)) {
          send_state_update(BATTLE_OP_ENDGAME, 0);
          changeState(SHOW_RESULTS);
          return;
        }
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

      if (current_battle_state == VS_SCREEN) {
        player->ship_locked_in = !player->ship_locked_in;
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
      if (current_battle_state != SHOW_RESULTS &&
          current_battle_state != LEVELUP) {
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
        chThdSleepMilliseconds(ALERT_DELAY_SHORT);
        bleGapDisconnect();

        orchardAppExit();
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
          pkt_vs = (bp_vs_pkt_t *)&bh->rxbuf;
          current_enemy->ship_locked_in = TRUE;
          current_enemy->unlocks = pkt_vs->bp_unlocks;
        /* intentional fall-through */
        case BATTLE_OP_SHIP_SELECT:
          pkt_vs = (bp_vs_pkt_t *)&bh->rxbuf;
          current_enemy->ship_type = pkt_vs->bp_shiptype;
          current_enemy->hp        = ENEMY_MAX_HP;
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
#ifdef DEBUG_WEAPONS
            printf("TAKEDMG: %d (%d / %d)\n",
              pkt_state->bp_operand,
              player->hp,
              PLAYER_MAX_HP);
#endif
            i2sPlay("game/explode2.snd");
            if (! player->is_shielded) {
              show_hit(player);
            } else {
              // blink the sheild.
              putImageFile(getAvatarImage(player->ship_type,
                TRUE, 'n', player->e.faces_right),
                player->e.vecPosition.x,
                player->e.vecPosition.y);
              chThdSleepMilliseconds(100);
              set_ship_sprite(player);
            }
            redraw_player_bars();

            // u dead.
            if (player->hp <= 0)
              chThdSleepMilliseconds(250);
          break;
          // really we should have replaced all of these with a single
          // SPECIAL_FLAGS call. oh well.
          case BATTLE_OP_SET_SHIELD:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->is_shielded = pkt_state->bp_operand;
            current_enemy->special_started_at = chVTGetSystemTime();
            set_ship_sprite(current_enemy);
            break;
          case BATTLE_OP_SET_HEAL:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->is_healing = pkt_state->bp_operand;
            current_enemy->special_started_at = chVTGetSystemTime();
            set_ship_sprite(current_enemy);
            break;
          case BATTLE_OP_SET_CLOAK:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->is_cloaked = pkt_state->bp_operand;
            current_enemy->special_started_at = chVTGetSystemTime();
            // if we're coming out of cloak we have to update.
            if (!current_enemy->is_cloaked) {
              isp_show_sprite(sprites,current_enemy->e.sprite_id);
            }
            set_ship_sprite(current_enemy);
            break;
          case BATTLE_OP_SET_TELEPORT:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->is_teleporting = pkt_state->bp_operand;
            // woooosh
            if (current_enemy->is_teleporting)
              i2sPlay("sound/levelup.snd");
            current_enemy->special_started_at = chVTGetSystemTime();
            set_ship_sprite(current_enemy);
            break;
          case BATTLE_OP_ENG_UPDATE:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->energy = pkt_state->bp_operand;
            redraw_enemy_bars();
            break;
          case BATTLE_OP_HP_UPDATE:
            pkt_state = (bp_state_pkt_t *)&bh->rxbuf;
            current_enemy->hp = pkt_state->bp_operand;
            redraw_enemy_bars();
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
          case BATTLE_OP_MINE_CREATE:
            pkt_bullet = (bp_bullet_pkt_t *)&bh->rxbuf;
            lay_mine(current_enemy);
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
          case BATTLE_OP_ENDGAME:
            changeState(SHOW_RESULTS);
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
      // after transmit, release the buffer.
      tx = &evt->evt.l2cap_evt.params.tx;
      // And then look at tx->sdu_buf.p_data, tx->sdu_buf.len.
      sl_release(tx->sdu_buf.p_data);
      break;

    case l2capConnectEvent:
      // now we have a peer connection.
      // we send with bleL2CapSend(data, size) == NRF_SUCCESS ...
#ifdef DEBUG_BATTLE_STATE
      printf("BATTLE: l2CapConnection Success\n");
#endif
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
  if (current_battle_state == VS_SCREEN)
  {
    if (event->type == keyEvent)
    {
      if (event->key.flags == keyPress)
      {
        switch (event->key.code)
        {
        case keyALeft:
          if (!player->ship_locked_in) {
            player->ship_type = getPrevShip(player);
          }
        break;

        case keyARight:
          // this is bit tricky!
          if (!player->ship_locked_in) {
            player->ship_type = getNextShip(player);
          }
          break;

        case keyBSelect:
          player->ship_locked_in = !player->ship_locked_in;
          i2sPlay("sound/ping.snd");
          send_ship_type(player->ship_type, player->ship_locked_in);

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

        player->hp     = PLAYER_MAX_HP;
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
          player->e.vecVelocityGoal.x = -player->e.vgoal;
          player->e.faces_right       = false;

          if (current_battle_state == COMBAT) {
            set_ship_sprite(player);
          }
          break;

        case keyARight:
          player->e.faces_right       = true;
          player->e.vecVelocityGoal.x = player->e.vgoal;

          if (current_battle_state == COMBAT) {
            set_ship_sprite(player);
          }
          break;

        case keyAUp:
          player->e.vecVelocityGoal.y = -player->e.vgoal;
          break;

        case keyADown:
          player->e.vecVelocityGoal.y = player->e.vgoal;
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

  // send one ping only.
#ifdef ENABLE_MAP_PING
  i2sPlay("game/map_ping.snd");
#endif
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
#ifdef DEBUG_BATTLE_STATE
  printf("engage: %x:%x:%x:%x:%x:%x\n",
         current_enemy->ble_peer_addr.addr[5],
         current_enemy->ble_peer_addr.addr[4],
         current_enemy->ble_peer_addr.addr[3],
         current_enemy->ble_peer_addr.addr[2],
         current_enemy->ble_peer_addr.addr[1],
         current_enemy->ble_peer_addr.addr[0]);
#endif

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
void combat_load_sprites(void) {
  BattleHandles *bh;
  bh = mycontext->priv;

  // load sprites
  bh->pl_left  = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'n', false));
  bh->pl_right = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'n', true));
  bh->ce_left  = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'n', false));
  bh->ce_right = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'n', true));

  // hit images
  bh->pl_h_left  = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'h', false));
  bh->pl_h_right = isp_get_spholder_from_file(
    getAvatarImage(player->ship_type, true, 'h', true));
  bh->ce_h_left  = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'h', false));
  bh->ce_h_right = isp_get_spholder_from_file(
    getAvatarImage(current_enemy->ship_type, false, 'h', true));

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

  // player - need heal if using Frigate.
  if (player->ship_type == SHIP_FRIGATE) {
    bh->pl_g_left  = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 'g', false));
    bh->pl_g_right = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 'g', true));
  }

  // enemy - need heal if using Frigate.
  if (current_enemy->ship_type == SHIP_FRIGATE) {
    bh->ce_g_left  = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 'g', false));
    bh->ce_g_right = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 'g', true));
  }

  // player - need submerged if using sub -- we will not load a submerged
  // one for the enemy, because they will just be set to !visible
  if (player->ship_type == SHIP_SUBMARINE) {
    bh->pl_u_left  = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 'u', false));
    bh->pl_u_right = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 'u', true));
  }

  // player - need heal if using Frigate.
  if (player->ship_type == SHIP_TESLA) {
    bh->pl_t_left  = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 't', false));
    bh->pl_t_right = isp_get_spholder_from_file(
      getAvatarImage(player->ship_type, true, 't', true));
  }

  // enemy - need heal if using Frigate.
  if (current_enemy->ship_type == SHIP_TESLA) {
    bh->ce_t_left  = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 't', false));
    bh->ce_t_right = isp_get_spholder_from_file(
      getAvatarImage(current_enemy->ship_type, false, 't', true));
  }
}
void state_combat_enter(void)
{
  char        fnbuf[30];
  int         newmap;
  userconfig *config = getConfig();

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
#ifdef DEBUG_BLE_MAPS
    printf("BLECENTRAL: map %d from player at %f, %f\n",
           newmap,
           player->e.vecPosition.x,
           player->e.vecPosition.y
           );
#endif
  }
  else
  {
    newmap = getMapTile(&current_enemy->e);
#ifdef DEBUG_BLE_MAPS
    printf("BLEPREPH: map %d from player at %f, %f\n",
           newmap,
           current_enemy->e.vecPosition.x,
           current_enemy->e.vecPosition.y
           );
#endif
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

  // set velocity params according to the ship
  player->e.vecGravity.x  = shiptable[player->ship_type].vdrag;
  player->e.vecGravity.y  = shiptable[player->ship_type].vdrag;
  player->e.vgoal         =
    player->unlocks & UL_SPEED ? (int)((float)shiptable[player->ship_type].vgoal * 1.2) :
                                 shiptable[player->ship_type].vgoal;
  player->e.vdrag         = shiptable[player->ship_type].vdrag;
  player->e.vapproach     = shiptable[player->ship_type].vapproach;
  player->e.vmult         = shiptable[player->ship_type].vmult;

  current_enemy->e.vecGravity.x  = shiptable[current_enemy->ship_type].vdrag;
  current_enemy->e.vecGravity.y  = shiptable[current_enemy->ship_type].vdrag;

  current_enemy->e.vgoal         =
    current_enemy->unlocks & UL_SPEED ? (int)((float)shiptable[current_enemy->ship_type].vgoal * 1.2) :
                                        shiptable[current_enemy->ship_type].vgoal;

  current_enemy->e.vapproach     = shiptable[current_enemy->ship_type].vapproach;
  current_enemy->e.vmult         = shiptable[current_enemy->ship_type].vmult;
  current_enemy->e.vdrag         = shiptable[current_enemy->ship_type].vdrag;

  combat_load_sprites();

  /* move the player to the starting position */
  if (ble_gap_role == BLE_GAP_ROLE_CENTRAL)
  {
    // Attacker (the central) gets left side, and faces right.
    player->e.vecPosition.x        = ship_init_pos_table[newmap].attacker.x;
    player->e.vecPosition.y        = ship_init_pos_table[newmap].attacker.y;
    player->e.faces_right          = true;

    current_enemy->e.vecPosition.x = ship_init_pos_table[newmap].defender.x;
    current_enemy->e.vecPosition.y = ship_init_pos_table[newmap].defender.y;
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
    chThdSleepMilliseconds(100);
    send_state_update(BATTLE_OP_ENDGAME, 0);
    i2sPlay("sound/foghrn.snd");
    changeState(SHOW_RESULTS);
  }
}

void regen_energy(ENEMY *e) {
  // regen energy

  // you can't regen energy underwater.
  if (e->is_cloaked) return;

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

void regen_health(ENEMY *e) {
  // regen energy

  int16_t maxhp;

  if (e == player) {
    maxhp = PLAYER_MAX_HP;
  } else {
    maxhp = ENEMY_MAX_HP;
  }

  if (e->is_healing) {
    // The FRIGATE is the only ship that can regen.
    // It can regen for one second, so this will restore 125 HP
    e->hp = e->hp + 5;

    if ((e == player) && (player->unlocks & UL_REPAIR)) {
        e->hp = e->hp + 2;
    }

    if (e->hp > maxhp) {
      e->hp = maxhp;
    }

    if (e == player) {
      send_state_update(BATTLE_OP_HP_UPDATE,e->hp);
      redraw_player_bars();
    }

    if (e == current_enemy) {
      redraw_enemy_bars();
    }
  }
}

void check_special_timeouts(ENEMY *e) {
  RECT r;

  if ((e->ship_type == SHIP_CRUISER) &&
      (e->is_shielded) &&
      (chVTTimeElapsedSinceX(e->special_started_at) >=
        TIME_MS2I(shiptable[e->ship_type].max_special_ttl))) {
          e->is_shielded = FALSE;
          set_ship_sprite(e);
          return;
  }

  if ((e->ship_type == SHIP_FRIGATE) &&
      (e->is_healing) &&
      (chVTTimeElapsedSinceX(e->special_started_at) >=
        TIME_MS2I(shiptable[e->ship_type].max_special_ttl))) {
          e->is_healing = FALSE;
          set_ship_sprite(e);
          return;
  }

  // do the animation for the tesla, now that enough time went by
  if ((e->ship_type == SHIP_TESLA) &&
      (e->is_teleporting) &&
      (chVTTimeElapsedSinceX(e->special_started_at) >=
        TIME_MS2I(500))) { // delay before teleporting
          e->is_teleporting = FALSE;
          if (e == player) {
            // animation over, teleport user to new loc.
            // store our position
#define SAFE_TELEPORT
#ifdef SAFE_TELEPORT
            do {
#endif
              r.x = randRange16(0,320-SHIP_SIZE_ZOOMED);
              r.y = randRange16(0,240-SHIP_SIZE_ZOOMED);

              r.xr = r.x + e->e.size_x;
              r.yb = r.y + e->e.size_y;

              // much like asteroids, you can teleport into oblivion...
              // figure out if we've died.
              e->e.vecPosition.x = r.x;
              e->e.vecPosition.y = r.y;
#ifdef SAFE_TELEPORT
            } while (wm_check_box_for_land_collision(sprites->wm, r) ||
                entity_OOB(&(e->e)));
#endif

            if (wm_check_box_for_land_collision(sprites->wm, r) ||
                entity_OOB(&(e->e))) {
              // you die. The next timer tick will do the anim.
              e->hp = 0;
              i2sPlay("game/explode2.snd");

              send_state_update(BATTLE_OP_HP_UPDATE,e->hp);
              redraw_player_bars();
            } else {
              // move player. combat tick will set the new
              // position for our sprite
              isp_set_sprite_xy(sprites,
                                e->e.sprite_id,
                                e->e.vecPosition.x,
                                e->e.vecPosition.y);

              // transmit our new position so the other guy gets it.
              send_position_update(
                player->e.id,
                BATTLE_OP_ENTITY_UPDATE,
                T_ENEMY,
                &(player->e)
              );
            }
          }
          set_ship_sprite(e);
          return;
  }

  // if you are cloaked we charge you energy (1 ENG / frame))
  // and tell the other player about it.
  if (e->is_cloaked) {
    e->energy = e->energy - 1;

    // slow down this rate, otherwise we will drop packets
    if (e == player && animtick % 3 == 0)
      send_state_update(BATTLE_OP_ENG_UPDATE,e->energy);

    if (e == player) {
      redraw_player_bars();
    }

    if (e == current_enemy) {
      redraw_enemy_bars();
    }
  }

  // the sub will resurface if it runs out of energy underwater
  if ((e->ship_type == SHIP_SUBMARINE) &&
      (e->is_cloaked) && (e->energy <= 0)) {
          e->is_cloaked = FALSE;
          isp_show_sprite(sprites,e->e.sprite_id);
          set_ship_sprite(e);
          return;
  }
}

void check_ship_collisions(void) {
  if (isp_check_sprites_collision(sprites,
                                  player->e.sprite_id,
                                  current_enemy->e.sprite_id,
                                  FALSE,
                                  FALSE)) {
        // ping-pong ball style simulation with lossage
        player->e.vecVelocity.x     = -player->e.vecVelocity.x * .85;
        player->e.vecVelocity.y     = -player->e.vecVelocity.y * .85;
        player->e.vecVelocityGoal.x = -player->e.vecVelocityGoal.x * .85;
        player->e.vecVelocityGoal.y = -player->e.vecVelocityGoal.y * .85;
        // sound
        // take damage
        if (!player->is_shielded) {
            i2sPlay("game/aground.snd");
            player->hp = player->hp - (int)((float)PLAYER_MAX_HP * 0.1);
            send_state_update(BATTLE_OP_HP_UPDATE, player->hp);
            redraw_player_bars();
        }
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

  regen_health(player);
  regen_health(current_enemy);

  check_special_timeouts(player);
  check_special_timeouts(current_enemy);

  /*
   * If we closed down the sprite library, then don't bother
   * checking for sprite collisions or doing redraws.
   */

  if (sprites == NULL)
    return;

  check_ship_collisions();
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

  // clear spholders - player
  isp_destroy_spholder(bh->pl_left);
  bh->pl_left = NULL;
  isp_destroy_spholder(bh->pl_right);
  bh->pl_right = NULL;
  isp_destroy_spholder(bh->pl_s_left);
  bh->pl_s_left = NULL;
  isp_destroy_spholder(bh->pl_s_right);
  bh->pl_s_right = NULL;
  isp_destroy_spholder(bh->pl_g_left);
  bh->pl_g_left = NULL;
  isp_destroy_spholder(bh->pl_g_right);
  bh->pl_g_right = NULL;
  isp_destroy_spholder(bh->pl_u_left);
  bh->pl_u_left = NULL;
  isp_destroy_spholder(bh->pl_u_right);
  bh->pl_u_right = NULL;
  isp_destroy_spholder(bh->pl_t_left);
  bh->pl_t_left = NULL;
  isp_destroy_spholder(bh->pl_t_right);
  bh->pl_t_right = NULL;

  // clear spholders - hit images
  isp_destroy_spholder(bh->pl_h_left);
  bh->pl_h_left = NULL;
  isp_destroy_spholder(bh->pl_h_right);
  bh->pl_h_right = NULL;
  isp_destroy_spholder(bh->ce_h_left);
  bh->ce_h_left = NULL;
  isp_destroy_spholder(bh->ce_h_right);
  bh->ce_h_right = NULL;

  // clear spholders - enemy
  isp_destroy_spholder(bh->ce_left);
  bh->ce_left = NULL;
  isp_destroy_spholder(bh->ce_right);
  bh->ce_right = NULL;
  isp_destroy_spholder(bh->ce_s_left);
  bh->ce_s_left = NULL;
  isp_destroy_spholder(bh->ce_s_right);
  bh->ce_s_right = NULL;
  isp_destroy_spholder(bh->ce_g_left);
  bh->ce_g_left = NULL;
  isp_destroy_spholder(bh->ce_g_right);
  bh->ce_g_right = NULL;
  isp_destroy_spholder(bh->ce_t_left);
  bh->ce_t_left = NULL;
  isp_destroy_spholder(bh->ce_t_right);
  bh->ce_t_right = NULL;

  /* tear down sprite system */
  isp_shutdown(sprites);
  sprites = NULL;
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

  if ( (player->hp == PLAYER_MAX_HP) &&
       (current_enemy->hp == ENEMY_MAX_HP)) {
    sprintf(tmp, "Y U NO PLAY? (0 XP!)");
  } else {
    // figure out who won
    if (player->hp > current_enemy->hp) {
        xpgain = calc_xp_gain(TRUE);
        config->won++;
        sprintf(tmp, "YOU WIN! (+%d XP)",xpgain);
        i2sPlay("game/victory.snd");
    } else {
      // record time of death
      config->lastdeath = chVTGetSystemTime();

      // reward (some) XP and exit
      xpgain = calc_xp_gain(FALSE);
      config->lost++;
      sprintf(tmp, "YOU LOSE! (+%d XP)",xpgain);
    }

    if (player->hp == current_enemy->hp) {
      // Everybody wins. Half.
      xpgain = calc_xp_gain(TRUE) / 2;

      config->lost++;
      sprintf(tmp, "TIE! (+%d XP)",xpgain);
    }

    config->xp += xpgain;
  }

  config->in_combat = 0;
  configSave(config);
  // it's now safe to tear down the connection.
  // we do not alarm for disconect events in SHOW_RESULTS
  if (bh->cid != BLE_L2CAP_CID_INVALID)
    bleL2CapDisconnect (bh->cid);

  bleGapDisconnect();

  // award xp here.
  screen_alert_draw(false, tmp);
  chThdSleepMilliseconds(ALERT_DELAY);

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
                       "CHOOSE!",
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
  bp_state_pkt_t pkt;
  uint8_t *buf;
  buf = sl_alloc();

  if (buf == NULL) {
    screen_alert_draw(TRUE, "PKT ALLOC FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }

  pkt.bp_header.bp_opcode = opcode;
  pkt.bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt.bp_operand = operand;
  pkt.bp_pad = 0xffff;
  memcpy(buf, &pkt, sizeof(bp_state_pkt_t));

  if (bleL2CapSend((uint8_t *)buf, sizeof(bp_state_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_ship_type(uint16_t type, bool final)
{
  bp_vs_pkt_t pkt;
  uint8_t *buf;
  buf = sl_alloc();
  userconfig *config = getConfig();

  if (buf == NULL) {
    screen_alert_draw(TRUE, "PKT ALLOC FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }

  pkt.bp_header.bp_opcode = final ? BATTLE_OP_SHIP_CONFIRM : BATTLE_OP_SHIP_SELECT;
  pkt.bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt.bp_shiptype         = type;
  pkt.bp_unlocks          = config->unlocks;

  memcpy(buf, &pkt, sizeof(bp_vs_pkt_t));

  if (bleL2CapSend((uint8_t *)buf, sizeof(bp_vs_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_position_update(uint16_t id, uint8_t opcode, uint8_t type, ENTITY *e)
{
  bp_entity_pkt_t pkt;
  uint8_t *buf;
  buf = sl_alloc();

  if (buf == NULL) {
    screen_alert_draw(TRUE, "PKT ALLOC FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }

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

  memcpy(buf, &pkt, sizeof(bp_entity_pkt_t));

  if (bleL2CapSend((uint8_t *)buf, sizeof(bp_entity_pkt_t)) != NRF_SUCCESS)
  {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_mine_create(ENTITY *e)
{
  bp_bullet_pkt_t *pkt;
  uint8_t *buf;
  buf = sl_alloc();

  if (buf == NULL) {
    screen_alert_draw(TRUE, "PKT ALLOC FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }

  // get private memory
  pkt = (bp_bullet_pkt_t *)buf;

  pkt->bp_header.bp_opcode = BATTLE_OP_MINE_CREATE;
  pkt->bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt->bp_header.bp_id     = e->id;
  pkt->bp_origin_x         = (uint16_t)e->vecPosition.x;
  pkt->bp_origin_y         = (uint16_t)e->vecPosition.y;

  if (bleL2CapSend((uint8_t *)buf, sizeof(bp_bullet_pkt_t)) != NRF_SUCCESS) {
    screen_alert_draw(TRUE, "BLE XMIT FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }
}

static void send_bullet_create(ENTITY *e, int16_t dir_x, int16_t dir_y, uint8_t is_free)
{
  bp_bullet_pkt_t *pkt;
  uint8_t *buf;
  buf = sl_alloc();

  if (buf == NULL) {
    screen_alert_draw(TRUE, "PKT ALLOC FAILED!");
    chThdSleepMilliseconds(ALERT_DELAY);
    orchardAppExit();
    return;
  }

  // get private memory
  pkt = (bp_bullet_pkt_t *)buf;

  pkt->bp_header.bp_opcode = BATTLE_OP_ENTITY_CREATE;
  pkt->bp_header.bp_type   = T_ENEMY; // always enemy.
  pkt->bp_header.bp_id = e->id;
  pkt->bp_dir_x      = dir_x;
  pkt->bp_dir_y      = dir_y;
  pkt->bp_is_free    = is_free;

  if (bleL2CapSend((uint8_t *)buf, sizeof(bp_bullet_pkt_t)) != NRF_SUCCESS) {
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

  // play theme on both badges
  i2sPlay("sound/ffenem.snd");

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
