#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__
/* prototypes */
static void state_worldmap_enter(void);
static void state_worldmap_tick(void);
static void state_worldmap_exit(void);

static void state_handshake_enter(void);
static void state_handshake_tick(void);

static void state_approval_wait_enter(void);
static void state_approval_wait_tick(void);
static void state_approval_wait_exit(void);

static void state_approval_demand_enter(void);
static void state_approval_demand_tick(void);
static void state_approval_demand_exit(void);

static void state_vs_enter(void);
static void state_vs_tick(void);

static void state_combat_enter(void);
static void state_combat_tick(void);
static void state_combat_exit(void);

static void state_levelup_enter(void);
static void state_levelup_tick(void);
static void state_levelup_exit(void);

static void state_show_results_enter(void);

/* The function state table, a list of pointers to functions to run
 * the game.
 *
 * Enter always fires when entering a state, tick fires on
 * the timer, exit is always fired on state exit for cleanups and
 * other housekeeping.
 *
 * When designing routines, minimize the time spent in the enter and
 * exit states!
 *
 */

state_funcs battle_funcs[] = {
  { // none
    NULL,
    NULL,
    NULL
  },
  { // world map
    state_worldmap_enter,
    state_worldmap_tick,
    state_worldmap_exit
  },
  { // HANDSHAKE
    state_handshake_enter,
    state_handshake_tick,
    NULL,
  },
  { // approval_demand - you are attacking us, we need to prompt
    state_approval_demand_enter,
    state_approval_demand_tick,
    state_approval_demand_exit,
  },
  {  // approval_wait - we are attacking you, you need to accept
    state_approval_wait_enter,
    state_approval_wait_tick,
    state_approval_wait_exit,
  },
  {  // vs_screen
    state_vs_enter,
    state_vs_tick,
    NULL,
  },
  {  // combat
    state_combat_enter,
    state_combat_tick,
    state_combat_exit
  },
  {  // show results
    state_show_results_enter,
    NULL,
    NULL
  },
  {  // player_dead
    NULL,
    NULL,
    NULL
  },
  {  // enemy_dead
    NULL,
    NULL,
    NULL
  },
  {  // levelup
    state_levelup_enter,
    state_levelup_tick,
    state_levelup_exit
  },

};

#endif /* STATEMACINE */
