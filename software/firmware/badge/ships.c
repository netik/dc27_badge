#include "ships.h"

/* ships.c */

/* ship definition file */

/* we store a lot of ship details here. */
const ship_type_t shiptable[8] = {

  // max_special_ttl is used for a variety of functions, depending.
  // copying from doc and will sort later.

  //  type_name,
  //  max_hp, max_dmg, max_energy, max_bullets,
  //  shots_msec, shot_range, shot_speed, shot_cost, shot_size
  //  vgoal, vdrag, vapproach, vmult
  //  special_cost, special_radius, max_special_dmg, max_special_ttl,
  //  energy_recharge_rate, specials

  // pt boat has -1 for special ttl, it's instant.
  {
    "PT Boat",
    200, 25, 100, 4 ,
    500, 150, 50, 30, 5,
    20, -0.005, 20, 8, // PT boat is swiftest but does least damage.
    30, 40, 25, -1,
    1, SP_SHOT_FOURWAY_DIAG // 4-way shot on diag!
  },

  {
    "Patrol Boat",
    250, 40, 120, 4,
    1000, 120, 50, 30, 5,
    DEFAULT_VRATES,
    40, 60, 35, 60,
    1, SP_SHOT_FOURWAY
  },

  // destroyers are only ship that can lay mines.
  // Mines don't move but persist for 7 seconds?
  {
    "Destroyer",
    400, 50, 120, 3,
    2000, 300, 40, 60, 7,
    DEFAULT_VRATES,
    40, 40, 75, 7000,
    2, SP_MINE
  },

  {
    // cruiser has impact blast of 25 (at end of range?)
    // cruiser also has a shield at 50/energy/second cost
    "Cruiser",
    400, 45, 250, 3,
    1250, 250, 35, 60, 7,
    DEFAULT_VRATES,
    250, 0, 0, 2000,
    2, SP_SHIELD
  },

  {
    "Frigate",
    500, 80, 300, 3,
    500, 500, 45, 70, 10,
    DEFAULT_VRATES,
    150, -1, 20, 1000,
    3, SP_HEAL  // make this be a acclearation to a goal, not instant.
  },

  {
    "Battleship",
    600, 60, 400, 3,
    333, 400, 35, 100, 10,
    DEFAULT_VRATES,
    100, 80, 40, 1000,
    3, SP_AOE // 3 shots in all directions? 4-way shot on diag!
  },

  {
    "Submarine",
    30, 30, 300, 3,
    333, 350, 40, 45, 6,
    DEFAULT_VRATES,
    40, -1, -1, 1000,
    3, SP_CLOAK // hides from other user. maybe show every N frames to tease?
  },

  {
    "Tesla",
    300, 30, 300, 3,
    333, 2000, 40, 45, 10,
    DEFAULT_VRATES,
    300, -1, -1, -1,
    4, SP_TELEPORT // teleports to random place, and if land, you die.
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
   { { 36, 180 }, { 233, 46 } },	/* Grid 8 */
   { { 18, 162 }, { 223, 46 } },	/* Grid 9 */
   { { 18, 162 }, { 270, 72 } },	/* Grid 10 */
   { { 36, 178 }, { 144, 72 } },	/* Grid 11 */
   { { 35, 144 }, { 233, 47 } },	/* Grid 12 */
   { { 18, 162 }, { 234, 54 } },	/* Grid 13 */
   { { 18, 144 }, { 252, 54 } },	/* Grid 14 */
   { { 18, 182 }, { 218, 47 } }		/* Grid 15 */
};
