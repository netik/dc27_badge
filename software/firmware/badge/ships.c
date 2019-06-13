#include "ships.h"

/* ships.c */

/* ship definition file */

/* we store a lot of ship details here. */
#define DEFAULT_VRATES VGOAL,VDRAG,VAPPROACH,VMULT

const ship_type_t shiptable[8] = {

  // max_special_ttl is used for a variety of functions, depending.
  // copying from doc and will sort later. 

  //  type_name, base_fn, 
  //  max_hp, max_dmg, max_energy, max_bullets, 
  //  shots_msec, shot_range, shot_speed,
  //  vgoal, vdrag, vapproach, vmult
  //  special_cost, special_radius, max_special_dmg, max_special_ttl, 
  //  energy_recharge_rate, specials

  // pt boat has -1 for special ttl, it's instant.
  { 
    "PT Boat", "ship1", 
    200, 10, 100, 3,
    500, 150, 50,
    DEFAULT_VRATES,
    30, 40, 25, -1, 
    20, SP_SHOT_RESTRICT
  },

  { 
    "Patrol Boat", "ship2", 
    250, 25, 120, 3,
    1000, 120, 50,
    DEFAULT_VRATES,
    40, 60, 35, 60, 
    25, SP_SHOT_EXTEND
  },

  // destroyers are only ship that can lay mines.
  // tbd: what does "do not move last 7 seconds each" mean? 
  // maybe mines don't move but persist for 7 seconds?
  { 
    "Destroyer","ship3", 
    400, 50, 120, 3,
    2000, 300, 40,
    DEFAULT_VRATES,
    40, 40, 75, 7000,
    20, SP_MINE
  },

  { 
    // cruiser has impact blast of 25, wtf is that?
    // cruiser also has a shield at 50/energy/second cost
    "Cruiser","ship4", 
    400, 45, 250, 3,
    2000, 250, 35,
    DEFAULT_VRATES,
    50, 0, 0, 1000,
    20, SP_SHIELD
  },

  {
    "Frigate","ship5",
    500, 80, 300, 3,
    500, 500, 45,
    DEFAULT_VRATES,
    60, -1, 20, 1000,
    40, SP_HEAL
  },

  { 
    "Battleship","ship6", 
    600, 60, 400, 3,
    333, 400, 35,
    DEFAULT_VRATES,
    100, 80, 40, 1000,
    40, SP_AOE
  },

  { 
    "Submarine","ship7",
    30, 30, 300, 3,
    333, 350, 40,
    DEFAULT_VRATES,
    40, -1, -1, 1000,
    20, SP_CLOAK
  },

  { 
    "Tesla","ship8",
    300, 30, 300, 3,
    333, 2000, 40,
    DEFAULT_VRATES,
    300, -1, -1, -1, 
    50, SP_TELEPORT
  }

};
