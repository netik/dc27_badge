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
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "images.h"
#include "battle.h"
#include "gll.h"
#include "math.h"
#include "led.h"
#include "userconfig.h"

#define ENABLE_MAP_PING  1   // if you want the sonar sounds
#define VGOAL       8        // ship accleration goal
#define VDRAG       -0.01f   // this is constant
#define VAPPROACH   12       // this is accel/decel rate
#define FRAME_DELAY 0.033f   // timer will be set to this * 1,000,000 (33mS)
#define FPS         30       // ... which represents about 30 FPS.
#define VMULT       8        // on each time step, take this many steps.
#define ENGAGE_BB   50       // If enemy is in this bounding box we can engage
#define MAX_BULLETS 3        // duh.

// size of sub-map tiles
#define TILE_W 80
#define TILE_H 60

/* single player on this badge */
static ENTITY me;
static ENTITY bullet[MAX_BULLETS];
static bool bullet_pending = false;

// enemies -- linked list
gll_t *enemies;

// the current game state */
enum game_state { WORLD_MAP, BATTLE };
enum game_state current_state = WORLD_MAP;

static int16_t frame_counter = 0;

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

  memset (p->pix_old, 0xFF, sizeof(p->pix_old));
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

static void
entity_update(ENTITY *p, float dt) {
  if ((p->ttl > -1) && (p->visible)) {
    p->ttl--;
    if (p->ttl == 0) {
      p->visible = false;
      i2sPlay("game/splash.snd");
    }
  }

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
}

static bool
player_check_collision(ENTITY *p) {
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
      return true;
    }
  }

  return false;
}

static void
entity_erase(ENTITY *p) {
  /*
   * This test is here because right after the app is launched,
   * before we draw the player for the first time, we don't have
   * anything to un-draw yet.
   */
  if (p->pix_old[0] == 0xFFFF)
    return;

  /* Put back what was previously on the screen */
  putPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, p->pix_old);
  return;
}

static void
player_render(ENTITY *p) {
  /* Draw ship */
  gdispFillArea (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, Purple);
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
enemy_render(void *e) {
  ENEMY *en = e;
  if (en->e.blinking) {
    // blink this enemy at 2 Hz.
    if ( (frame_counter % FPS) < (FPS/2) )
      return;
  }
  gdispFillArea (en->e.vecPosition.x, en->e.vecPosition.y, FB_X, FB_Y, Red);
}

static void
enemy_renderall(void) {
  // @brief walk our linked list of enemies and render each one
  gll_each(enemies, enemy_render);
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

static void
enemy_erase(void *e) {
  ENEMY *en = e;
  entity_erase(&(en->e));
}

static void
enemy_erase_all(void) {
  gll_each(enemies, enemy_erase);
}

static void enemy_update(void *e) {
  ENEMY *en = e;
  // do whatever update here, like get positions from network
  // grab the pixels under the new position
  getPixelBlock (en->e.vecPosition.x, en->e.vecPosition.y, FB_X, FB_Y, en->e.pix_old);
}

static void
enemy_update_all(void) {
  gll_each(enemies, enemy_update);
}


static uint32_t
battle_init(OrchardAppContext *context)
{
  (void)context;
  ENEMY *e;

  entity_init(&me);
  me.visible = true;

  for (int i=0; i < MAX_BULLETS; i++) {
		entity_init(&bullet[i]);
  }

  // fake enemies -- we'll get these from BLE later on.
  enemies = gll_init();
  e = malloc(sizeof(ENEMY));
  entity_init(&(e->e));
  e->e.visible = true;
  e->e.vecPosition.x = 90;
  e->e.vecPosition.y = 90;
  strcpy(e->name, "enemy1");
  gll_push(enemies, e);

  // turn off the LEDs
  led_clear();
  ledSetPattern(LED_PATTERN_WORLDMAP);
  // don't allocate any stack space
  return (0);
}

static void draw_initial_map(void) {
  putImageFile("game/world.rgb", 0,0);

  // Draw UI
  // TBD
}

static void
battle_start (OrchardAppContext *context)
{
  (void)context;
  gdispClear (Black);

  draw_initial_map();

  /* start the timer - 30 fps */
  orchardAppTimer(context, FRAME_DELAY * 1000000, true);
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
enemy_engage(void) {

  ENEMY *e = getNearestEnemy();

  if (e == NULL) {
    // play an error sound because no one is near by
    i2sPlay("game/error.snd");
    return;
  } else {
    // zoom in
    char fnbuf[20];
    i2sPlay("game/engage.snd");

    // attempt connection...

    // if success...

    int newmap = getMapTile(&me);
    printf("(%f, %f) -> %d\n", me.vecPosition.x, me.vecPosition.y, newmap);
    sprintf(fnbuf, "game/map-%02d.rgb", newmap);
    putImageFile(fnbuf, 0,0);
    zoomEntity(&me);
    player_render(&me);
  }

}

static void
fire_bullet(ENTITY *from, int angle) {
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
      bullet[i].vecVelocityGoal.x = 50;
      bullet[i].vecVelocityGoal.y = -50;
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
  VECTOR prevme;
  ENEMY *nearest;

  /* MAIN GAME EVENT LOOP */
  if (event->type == timerEvent) {
    frame_counter++;
#ifdef ENABLE_MAP_PING
    /* every four seconds (120 frames) */
    if ((frame_counter % 120 == 0) && (current_state == WORLD_MAP)) {
      i2sPlay("game/map_ping.snd");
    }
#endif

    // erase everything.
    entity_erase(&me);
    prevme.x = me.vecPosition.x;
    prevme.y = me.vecPosition.y;

    // bullets
    for (i=0; i < MAX_BULLETS; i++) {
      if (bullet[i].visible == true) {
        entity_erase(&bullet[i]);
      }
    }

    // we have to update bullets in the game loop, not outside or
    // we'll have all sorts of artifacts on the screen.
    // do we need a new bullet?
    if (bullet_pending) {
      fire_bullet(&me, 0);
      bullet_pending = false;
    };

    // enemy update
    enemy_erase_all();
    enemy_update_all();

    // player update
    entity_update(&me, FRAME_DELAY);
    getPixelBlock (me.vecPosition.x, me.vecPosition.y, FB_X, FB_Y, me.pix_old);

    // bullets
    for (i=0; i < MAX_BULLETS; i++) {
      if (bullet[i].visible == true) {
        entity_update(&bullet[i], FRAME_DELAY);
        if (entity_OOB(&bullet[i])) {
          // out of bounds, remove.
          bullet[i].visible = false;
        }
        getPixelBlock(bullet[i].vecPosition.x, bullet[i].vecPosition.y, FB_X, FB_Y, bullet[i].pix_old);
      }
    }

    if (player_check_collision(&me) || entity_OOB(&me)) {
      // put the player back and get the pixel again
      me.vecPosition.x = prevme.x;
      me.vecPosition.y = prevme.y;
      getPixelBlock (me.vecPosition.x, me.vecPosition.y, FB_X, FB_Y, me.pix_old);
    }

    bullets_render();
    player_render(&me);

    // render enemies
    enemy_clearall_blink();
    nearest = getNearestEnemy();
    if (nearest != NULL) {
      nearest->e.blinking = true;
    }

    enemy_renderall();
  }

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
      case keyBUp:
        orchardAppExit();
        break;
      case keyBDown:
        if (current_state != BATTLE) {
          current_state = BATTLE;
          enemy_engage();
        }
        break;
      case keyASelect:
        if (current_state == BATTLE) {
          // can only fire in battle map.
          bullet_pending = true;
        } else {
          i2sPlay("game/error.snd");
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

static void
battle_exit(OrchardAppContext *context)
{
  userconfig *config = getConfig();

  (void) context;

  // free the enemy list
  gll_destroy(enemies);
  
  // restore the LED pattern from config
  led_clear();
  ledSetPattern(config->led_pattern);
  return;
}

orchard_app("Sea Battle", "icons/ship.rgb", 0, battle_init, battle_start,
    battle_event, battle_exit, 1);
