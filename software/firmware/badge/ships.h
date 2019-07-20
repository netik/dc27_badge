#ifndef _SHIPS_H_
#define _SHIPS_H_

#include <stdio.h>
#include "vector.h"
#include "ch.h"

/* ships.h */

/* ship parameter structures and defines
 *
 * J. Adams <jna@retina.net> 6/2019
 *
 */

// bitfields
#define SP_NONE                0
#define SP_HEAL                ( 1 << 0 )
#define SP_CLOAK               ( 1 << 1 )
#define SP_TELEPORT            ( 1 << 2 )
#define SP_MINE                ( 1 << 3 )
#define SP_SHOT_FOURWAY_DIAG   ( 1 << 4 )
#define SP_SHOT_FOURWAY        ( 1 << 5 )
#define SP_SHIELD              ( 1 << 6 )
#define SP_TWOSHOT             ( 1 << 7 )

// generic parameters for motion
#define VGOAL       8        // ship accleration goal
#define VDRAG       -0.01f   // this is constant
#define VAPPROACH   12       // this is accel/decel rate
#define VMULT       8        // on each time step, take this many steps.
#define MAX_BULLETS 32       // counts all bullets from all players, incl mines

// size of sub-map tiles
#define TILE_W 80
#define TILE_H 60

// ship type defines
#define SHIP_PTBOAT     0
#define SHIP_PATROL     1
#define SHIP_DESTROYER  2
#define SHIP_CRUISER    3
#define SHIP_FRIGATE    4
#define SHIP_BATTLESHIP 5
#define SHIP_SUBMARINE  6
#define SHIP_TESLA      7

typedef struct {
  char type_name[12]; /* type */

  /* base params */
  int16_t max_hp;
  int16_t max_dmg;
  int16_t max_energy;
  uint8_t max_bullets;

  /* fire params */
  int16_t shot_msec;   // this much time (in mS) between shots
  int16_t shot_range;  // range is in px
  int16_t shot_speed;  // pixels/second
  int16_t shot_cost;   // energy cost to fire shot
  uint8_t shot_size;   // shot gfx size

  /* accleration parameters */
  float  vdrag;
  int8_t vgoal;
  int8_t vapproach;
  int8_t vmult;

  /* special params */
  int16_t special_cost;
  int16_t special_radius; // range of the special for collisions
  int16_t max_special_dmg;
  int16_t max_special_ttl;
  float   energy_recharge_rate; // This is per-frame, not per second.

  /* specials (bitmap) */
  uint8_t special_flags;

} ship_type_t;

typedef struct ship_init_pos {
  VECTOR attacker;
  VECTOR defender;
} ship_init_pos_t;

#define SHIP_INIT_POS_COUNT 16

extern const ship_type_t shiptable[];
extern const ship_init_pos_t ship_init_pos_table[];

#define DEFAULT_VRATES VGOAL,VDRAG,VAPPROACH,VMULT

#endif /* _SHIPS_H_ */
