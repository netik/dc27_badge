/*
 * app-battle.c
 *
 * J. Adams 4/27/2019
 *
 */
#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "i2s_lld.h"
#include "ides_gfx.h"
#include "battle.h"
#include <stdio.h>

#include "chprintf.h"

/* this is our accleration goal */
#define VGOAL      0.4 // this should be per-ship.
#define VDRAG      -0.001f // this is constant
#define VAPPROACH  0.005 // this is accel/decel rate
// note that if vgoal gets lower and lower vapproach must come down

/* set this up for testing */
PLAYER me;

static void
player_init(PLAYER *p) {
	p->vecVelocity.x = 0 ;
	p->vecVelocity.y = 0 ;

	p->vecVelocityGoal.x = 0;
	p->vecVelocityGoal.y = 0;

	p->vecPosition.x = HARBOR_LWR_X;
	p->vecPosition.y = HARBOR_LWR_Y;

	// this is "ocean drag"
	p->vecGravity.x = VDRAG;
	p->vecGravity.y = VDRAG;

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

	p->vecPosition.x = p->vecPosition.x + p->vecVelocity.x * dt;
	p->vecPosition.y = p->vecPosition.y + p->vecVelocity.y * dt;

	p->vecVelocity.x = p->vecVelocity.x + p->vecGravity.x * dt;
	p->vecVelocity.y = p->vecVelocity.y + p->vecGravity.y * dt;

	// check bounds
	if (p->vecPosition.x < 0) {
		p->vecPosition.x = 0;
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
player_render(PLAYER *p) {
	gdispFillArea(p->vecPosition.x, p->vecPosition.y,
	              10,10,
							  Purple);
}

static void
player_erase(PLAYER *p) {
	gdispFillArea(p->vecPosition.x, p->vecPosition.y,
	              10, 10,
							  Blue);
}


static uint32_t
battle_init(OrchardAppContext *context)
{
	(void)context;

	player_init(&me);

	// don't allocate any stack space
	return (0);
}

static void draw_initial_map(void) {
	putImageFile("game/world.rgb", 0,0);
}

static void
battle_start (OrchardAppContext *context)
{
	(void)context;
	gdispClear (Black);

  draw_initial_map();

	/* start the timer - 30 fps */
	orchardAppTimer(context, 330, true);
	return;
}

static void
battle_event(OrchardAppContext *context, const OrchardAppEvent *event)
{
	(void) context;

	if (event->type == timerEvent) {
		player_erase(&me);
		player_update(&me, 0.33f);
		player_render(&me);
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
	//	configSave(config);
//		orchardAppExit();
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
