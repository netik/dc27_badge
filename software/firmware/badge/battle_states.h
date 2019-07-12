#ifndef _BATTLE_STATES_H_
#define _BATTLE_STATES_H_

// the game state machine
#ifdef DEBUG_BATTLE_STATE
const char* battle_state_name[]  = {
  "NONE"  ,             // 0 - Not started yet.
  "WORLD_MAP"  ,        // 1 - we're in the lobby
  "HANDSHAKE",          // 2 - a packet came in and we wait for l2cap connect
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
  HANDSHAKE,        // 2 - waiting on attacker l2cap connect
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

#endif
