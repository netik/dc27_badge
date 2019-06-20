// the game state machine
#ifdef DEBUG_BATTLE_STATE
const char* battle_state_name[]  = {
  "NONE"  ,             // 0 - Not started yet.
  "WORLD_MAP"  ,        // 1 - we're in the lobby
  "GRANT_ACCEPT" ,      // 2 - Shown when you get an unlock from a BLACK_BADGE
  "GRANT_SCREEN"  ,     // 3 - Choose grant to send to user
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

typedef enum _battle_state {
  NONE,             // 0 - Not started yet.
  WORLD_MAP,        // 1 - We're in the lobby.
  GRANT_ACCEPT,     // 2 - We're getting a grant
  GRANT_SCREEN,     // 3 - We're sending a grant
  APPROVAL_DEMAND,  // 4 - I want to fight you!
  APPROVAL_WAIT,    // 5 - I am waiting to see if you want to fight me
  VS_SCREEN,        // 6 - I am showing the versus screen.
  COMBAT,           // 7 - We are fighting
  SHOW_RESULTS,     // 8 - We are showing results, and will wait for an ACK
  PLAYER_DEAD,      // 9 - I die!
  ENEMY_DEAD,       // 10 - You're dead.
  LEVELUP,          // 11 - I am on the bonus screen, leave me alone.
} battle_state;

typedef struct _state {
  void (*enter)(void);
  void (*tick)(void);
  void (*exit)(void);
} state_funcs;

/* prototypes */

static void state_worldmap_enter(void);
static void state_worldmap_tick(void);
static void state_worldmap_exit(void);

static void state_approval_wait_enter(void);
static void state_approval_wait_tick(void);
static void state_approval_wait_exit(void);

static void state_approval_demand_enter(void);
static void state_approval_demand_tick(void);
static void state_approval_demand_exit(void);

static void state_combat_enter(void);
static void state_combat_tick(void);
static void state_combat_exit(void);

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
  { // grant accept
    NULL,
    NULL,
    NULL
  },
  { // grant screen
    NULL,
    NULL,
    NULL
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
    NULL,
    NULL,
    NULL
  },
  {  // combat
    state_combat_enter,
    state_combat_tick,
    state_combat_exit
  },
  {  // show results
    NULL,
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
    NULL,
    NULL,
    NULL
  },

};
