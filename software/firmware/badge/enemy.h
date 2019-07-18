/* enemy handling and processing code */

#ifndef __ENEMY_H__
#define __ENEMY_H__
#include "battle_states.h"

#define ENGAGE_BB    40      // If enemy is in this bounding box we can engage

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

extern uint16_t calc_hit(ENEMY *attacker, ENEMY *victim);
#endif
