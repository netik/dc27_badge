#ifndef _SHIPS_H_
#define _SHIPS_H_

#include <stdio.h>
#include "ch.h"

/* ships.h */

/* ship parameter file
 *
 * J. Adams <jna@retina.net> 6/2019
 *
 */

/*
 * this structure represents ship parameters.
 * base_fn is typically the shortened ship name used to derive the icon filenames
 *
 * example:
 *     basefn = sub
 *     LH file name = game/sub-l.rgb
 *     RH file name = game/sub-r.rgb (flipped)
 */

// bitfields 
#define SP_NONE          0
#define SP_HEAL          ( 1 << 0 )
#define SP_CLOAK         ( 1 << 1 ) 
#define SP_TELEPORT      ( 1 << 2 )
#define SP_MINE          ( 1 << 3 ) 
#define SP_SHOT_RESTRICT ( 1 << 4 ) 
#define SP_SHOT_EXTEND   ( 1 << 5 ) 
#define SP_SHIELD        ( 1 << 6 )
#define SP_AOE           ( 1 << 7 ) // TBD

// generic parameters for motion
#define VGOAL       8        // ship accleration goal
#define VDRAG       -0.01f   // this is constant
#define VAPPROACH   12       // this is accel/decel rate
#define VMULT       8        // on each time step, take this many steps.
#define ENGAGE_BB   40       // If enemy is in this bounding box we can engage
#define MAX_BULLETS 3        

// size of sub-map tiles
#define TILE_W 80
#define TILE_H 60

typedef struct {
  char type_name[12]; /* type */
  char base_fn[16];   /* icon name that all other names will be based on */
  
  /* base params */
  int16_t max_hp;
  int16_t max_dmg;
  int16_t max_energy;
  uint8_t max_bullets;

  /* fire params */
  int16_t shot_msec;   // this much time (in mS) between shots
  int16_t shot_range;  // range is in px, but needs to be frames.
  int16_t shot_speed;  // pixels/second

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
  float   energy_recharge_rate;

  /* specials (bitmap) */
  uint8_t special_flags;

} ship_type_t;

#endif /* _SHIPS_H_ */
