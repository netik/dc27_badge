/*
 * app-battle.c
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
#include "battle.h"
#include "gll.h"

/* this is our accleration goal */
#define VGOAL      8        // this should be per-ship.
#define VDRAG      -0.01f   // this is constant
#define VAPPROACH  12       // this is accel/decel rate
#define FRAME_DELAY 0.033f  // timer will be set to this * 1,000,000 (33mS)
#define VMULT      8        // on each time step, take this many steps.
#define SEARCH_BB  30       // If enemy is in this bounding box we can engage

// note that if VGOAL is lowered VAPPROACH must come down to match

/* single player on this badge */
gll_t *enemies;
static PLAYER me;

// the current game state */
enum game_state { WORLD_MAP, BATTLE };
enum game_state current_state = WORLD_MAP;

/* Old pixel data */
#define FB_X 10
#define FB_Y 10
static pixel_t pix_old[FB_X * FB_Y];
static int16_t ping_timer = 120; // so we get one ping at start

static void
player_init(PLAYER *p) {
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

	memset (pix_old, 0xFF, sizeof(pix_old));
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
player_update(PLAYER *p, float dt) {

 	p->vecVelocity.x = approach(
			p->vecVelocityGoal.x,
			p->vecVelocity.x,
			dt * VAPPROACH);

	p->vecVelocity.y = approach(
			p->vecVelocityGoal.y,
			p->vecVelocity.y,
			dt * VAPPROACH);

	p->vecPosition.x = p->vecPosition.x + p->vecVelocity.x * dt	* VMULT;
	p->vecPosition.y = p->vecPosition.y + p->vecVelocity.y * dt * VMULT;

	p->vecVelocity.x = p->vecVelocity.x + p->vecGravity.x * dt;
	p->vecVelocity.y = p->vecVelocity.y + p->vecGravity.y * dt;

	// check bounds
	if (p->vecPosition.x < 0) {
		p->vecPosition.x = 0 ;
	}

	if (p->vecPosition.y < 0) {
		p->vecPosition.y = 0;
	}

	if (p->vecPosition.x > 310 ) {
		p->vecPosition.x = 310;
	}

	if (p->vecPosition.y > 230) {
		p->vecPosition.y = 230;
	}
}

static void
player_check_collision(PLAYER *p) {
  /* Save what's currently on the screen */
  getPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, pix_old);

	/* linear search for a collision with land.
	 *
	 * this is somewhat inefficient, we should be checking edges of pixmap
	 *  not interior. Edges are 40 pixels, interior is 100 px. maybe we don't
	 *	 care at such a small size.
	*/
  for (int i = 0; i < FB_X * FB_Y; i++) {
     if ( pix_old[i] <= 0x9e00 || pix_old[i] >= 0xbf09 ) {

	    // collision - not in sea

			// stop the ship
	    p->vecVelocity.x = 0;
	    p->vecVelocity.y = 0;
	    p->vecVelocityGoal.x = 0;
	    p->vecVelocityGoal.y = 0;

			// no damage if in home base or enemy base

			// play the collision sound

			// take some damage

			// update damage / energy

	  }
	}
}

static void
player_render(PLAYER *p) {
  /* Draw ship */
  gdispFillArea (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, Purple);
  return;
}

static void
enemy_render(void *e) {
	ENEMY *en = e;

	gdispFillArea (en->p.vecPosition.x, en->p.vecPosition.y, FB_X, FB_Y, Red);
}

static void
enemy_renderall(void) {
	/* walk our linked list of enemies and render each one */
	gll_each(enemies, enemy_render);
}

static void
player_erase(PLAYER *p) {
	/*
	 * This test is here because right after the app is launched,
	 * before we draw the player for the first time, we don't have
	 * anything to un-draw yet.
	 */
	if (pix_old[0] == 0xFFFF)
		return;
	/* Put back what was previously on the screen */
	putPixelBlock (p->vecPosition.x, p->vecPosition.y, FB_X, FB_Y, pix_old);
	return;
}


static uint32_t
battle_init(OrchardAppContext *context)
{
	(void)context;
	ENEMY *e;

	player_init(&me);

	// three fake enemies -- we'll get these from BLE later on.
	enemies = gll_init();

	/* enemy 1 */
	e = malloc(sizeof(ENEMY));
	player_init(&(e->p));
	e->p.vecPosition.x = 90;
	e->p.vecPosition.y = 90;
	strcpy(e->name, "enemy1");
	gll_push(enemies, e);

	// don't allocate any stack space
	return (0);
}

static void draw_initial_map(void) {
	putImageFile("game/world.rgb", 0,0);

	// center the words "avoid land!"


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
		if ((e->p.vecPosition.x >= (me.vecPosition.x - (SEARCH_BB/2))) &&
		    (e->p.vecPosition.x <= (me.vecPosition.x + (SEARCH_BB/2))) &&
				(e->p.vecPosition.y >= (me.vecPosition.y - (SEARCH_BB/2))) &&
		    (e->p.vecPosition.y <= (me.vecPosition.y + (SEARCH_BB/2)))) {
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
			// otherwise play an error sound because no one is near by
			i2sPlay("game/error.snd");
			return;
	} else {
		i2sPlay("game/engage.snd");

	}

	// if so, attempt BLE connection
}

static void
battle_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	(void) context;
	float oldx, oldy;
	float newx, newy;

	if (event->type == timerEvent) {
		ping_timer++;
		/* every four seconds (120 frames) */
		if (ping_timer >= 120) {
			ping_timer = 0;
			i2sPlay("game/map_ping.snd");
		}

//		printf("v: %f,%f %f,%f\n", me.vecVelocity.x, me.vecVelocity.y, me.vecVelocityGoal.x, me.vecVelocity.y);

		oldx = me.vecPosition.x;
		oldy = me.vecPosition.y;
		player_update(&me, FRAME_DELAY);

		newx = me.vecPosition.x;
		newy = me.vecPosition.y;
		/*
		 * Redraw player ship if it changed position.
		 * Note: we cast the coordinates to integer values here
		 * when doing the comparisson to effectively round down
		 * the coordinates to whole pixels (doing a redraw for
		 * fractional pixel changes just wastes cycles without
		 * making any meaningful screen updates.
		 * Also note: after the app is first launched, we need
		 * to draw the player for the first time, but he hasn't
		 * moved yet. To get around this, check for the old pixel
		 * buffer to be unused and draw the player if it is.
		 */
		if (((coord_t)oldx != (coord_t)newx ||
		    (coord_t)oldy != (coord_t)newy) ||
		    pix_old[0] == 0xFFFF) {
			me.vecPosition.x = oldx;
			me.vecPosition.y = oldy;
			player_erase(&me);
			me.vecPosition.x = newx;
			me.vecPosition.y = newy;

      player_check_collision(&me);

			// process bullets if any
			// player_hitscan(...)

			player_render(&me);
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
				  enemy_engage();
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
	(void) context;
	return;
}

orchard_app("Sea Battle", "icons/ship.rgb", 0, battle_init, battle_start,
    battle_event, battle_exit, 1);
