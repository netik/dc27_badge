#include "ships.h"

/* ships.c */

/* ship definition file */

/* we store a lot of ship details here. */
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
    "PT Boat", 
    200, 10, 100, 3,
    500, 150, 50,
    DEFAULT_VRATES,
    30, 40, 25, -1, 
    20, SP_SHOT_RESTRICT
  },

  { 
    "Patrol Boat",
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
    "Destroyer",
    400, 50, 120, 3,
    2000, 300, 40,
    DEFAULT_VRATES,
    40, 40, 75, 7000,
    20, SP_MINE
  },

  { 
    // cruiser has impact blast of 25, wtf is that?
    // cruiser also has a shield at 50/energy/second cost
    "Cruiser",
    400, 45, 250, 3,
    2000, 250, 35,
    DEFAULT_VRATES,
    50, 0, 0, 1000,
    20, SP_SHIELD
  },

  {
    "Frigate",
    500, 80, 300, 3,
    500, 500, 45,
    DEFAULT_VRATES,
    60, -1, 20, 1000,
    40, SP_HEAL
  },

  { 
    "Battleship",
    600, 60, 400, 3,
    333, 400, 35,
    DEFAULT_VRATES,
    100, 80, 40, 1000,
    40, SP_AOE
  },

  { 
    "Submarine",
    30, 30, 300, 3,
    333, 350, 40,
    DEFAULT_VRATES,
    40, -1, -1, 1000,
    20, SP_CLOAK
  },

  { 
    "Tesla",
    300, 30, 300, 3,
    333, 2000, 40,
    DEFAULT_VRATES,
    300, -1, -1, -1, 
    50, SP_TELEPORT
  }

};

/*
 * Initial combat positions
 * When combat begins, combatants will be on one of 16 possible
 * grid rectangles on the world map. These are the initial positions
 * for each grid, with attacker (BLE central) first and defender
 * (BLE peripheral) second.
 */

const ship_init_pos_t ship_init_pos_table[SHIP_INIT_POS_COUNT] = {
   { { 54, 162 }, { 270, 72 } },	/* Grid 0 */
   { { 36, 180 }, { 216, 35 } },	/* Grid 1 */
   { { 19, 160 }, { 252, 36 } },	/* Grid 2 */
   { { 53, 198 }, { 216, 54 } },	/* Grid 3 */
   { { 17, 163 }, { 252, 54 } },	/* Grid 4 */
   { { 54, 180 }, { 252, 54 } },	/* Grid 5 */
   { { 72, 180 }, { 252, 54 } },	/* Grid 6 */
   { { 36, 181 }, { 179, 54 } },	/* Grid 7 */
   { { 36, 180 }, { 233, 36 } },	/* Grid 8 */
   { { 18, 162 }, { 233, 36 } },	/* Grid 9 */
   { { 18, 162 }, { 270, 72 } },	/* Grid 10 */
   { { 36, 178 }, { 144, 72 } },	/* Grid 11 */
   { { 35, 144 }, { 233, 37 } },	/* Grid 12 */
   { { 18, 162 }, { 234, 54 } },	/* Grid 13 */
   { { 18, 144 }, { 252, 54 } },	/* Grid 14 */
   { { 18, 182 }, { 218, 37 } }		/* Grid 15 */
};
