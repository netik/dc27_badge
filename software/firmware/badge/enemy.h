/* enemy handling and processing code */

#ifndef __ENEMY_H__
#define __ENEMY_H__
#include "battle_states.h"

#define ENGAGE_BB    40      // If enemy is in this bounding box we can engage

typedef struct _enemy
{
  ble_gap_addr_t ble_peer_addr;
  char           name[CONFIG_NAME_MAXLEN];
  uint8_t        level;

  // these represent the current state of the user.
  int16_t        hp;
  int16_t        xp;
  int16_t        energy;
  uint8_t        ship_type;

  systime_t      last_shot_ms;
  systime_t      last_special_ms;
  systime_t      special_started_at;

  bool           is_shielded;
  bool           is_healing;
  bool           is_cloaked;

  bool           ship_locked_in; // if the enemy has locked in their ship type

  uint16_t       unlocks;
  uint8_t        ttl;
  ENTITY         e;
} ENEMY;

extern ENEMY *enemy_find_by_peer(gll_t *enemies, uint8_t *);
extern void enemy_clearall_blink(gll_t *enemies);
extern int enemy_find_ll_pos(gll_t *enemies, ble_gap_addr_t *);
ENEMY *getNearestEnemy(gll_t *enemies, ENEMY *player);
extern ENEMY *enemy_engage(OrchardAppContext *context,
                           gll_t *enemies,
                           ENEMY *player);
ENEMY *getEnemyFromBLE(ble_gap_addr_t *peer, ENEMY *current_enemy);
void enemy_list_refresh(gll_t *enemies,
                        ISPRITESYS *sprites,
                        battle_state current_battle_state);

#endif
