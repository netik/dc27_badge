/* enemy.c
 *
 * functions for managing the enemy list
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "nrf52i2s_lld.h"
#include "ides_gfx.h"
#include "images.h"
#include "ides_sprite.h"
#include "images.h"
#include "battle.h"
#include "gll.h"
#include "math.h"
#include "led.h"
#include "rand.h"
#include "userconfig.h"
#include "enemy.h"
#include "entity.h"

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_l2cap_lld.h"
#include "ble_gattc_lld.h"
#include "ble_gatts_lld.h"
#include "ble_peer.h"

#include "ships.h"
#include "battle.h"
#include "battle_states.h"

extern mutex_t     peer_mutex;

ENEMY *enemy_find_by_peer(gll_t * enemies, uint8_t *addr)
{
  gll_node_t *currNode = enemies->first;

  while (currNode != NULL)
  {
    ENEMY *e = (ENEMY *)currNode->data;

    if (memcmp(&(e->ble_peer_addr.addr), addr, 6) == 0)
    {
      return(e);
    }

    currNode = currNode->next;
  }

  return(NULL);
}


void enemy_clear_blink(void *e)
{
  // @brief clear enemy blink state
  ENEMY *en = e;

  en->e.blinking = false;
}

void enemy_clearall_blink(gll_t * enemies)
{
  // @brief clear blink state of all enemies
  if (enemies)
  {
    gll_each(enemies, enemy_clear_blink);
  }
}

int enemy_find_ll_pos(gll_t * enemies, ble_gap_addr_t *ble_peer_addr)
{
  gll_node_t *currNode = enemies->first;
  int         i        = 0;

  while (currNode != NULL)
  {
    ENEMY *e = (ENEMY *)currNode->data;

    if (memcmp(&(e->ble_peer_addr), ble_peer_addr, 6) == 0)
    {
      return(i);
    }

    i++;
    currNode = currNode->next;
  }

  return(-1);
}

ENEMY *getNearestEnemy(gll_t *enemies, ENEMY *player)
{
  // given my current position? are there any enemies near me?
  gll_node_t *currNode = enemies->first;

  while (currNode != NULL)
  {
    ENEMY *e = (ENEMY *)currNode->data;

    // does not account for screen edges -- but might be ok?
    // also does not attempt to sort the enemy list. if two people in same place
    // we'll have an issue.
    if ((e->e.vecPosition.x >= (player->e.vecPosition.x - (ENGAGE_BB / 2))) &&
        (e->e.vecPosition.x <= (player->e.vecPosition.x + (ENGAGE_BB / 2))) &&
        (e->e.vecPosition.y >= (player->e.vecPosition.y - (ENGAGE_BB / 2))) &&
        (e->e.vecPosition.y <= (player->e.vecPosition.y + (ENGAGE_BB / 2))))
    {
      return(e);
    }
    currNode = currNode->next;
  }
  return(NULL);
}

ENEMY *enemy_engage(OrchardAppContext *context, gll_t *enemies, ENEMY *player)
{
  ENEMY *e;

  e = getNearestEnemy(enemies, player);

  if (e == NULL)
  {
    // play an error sound because no one is near by
    i2sPlay("game/error.snd");
    return(NULL);
  }

  return(e);
}

ENEMY *getEnemyFromBLE(ble_gap_addr_t *peer, ENEMY *current_enemy)
{
  // returns current_enemy's new allocated address if we find an
  // enemy or NULL if we do not.
  ble_peer_entry *p;

  printf("searching for %x:%x:%x:%x:%x:%x\n",
         peer->addr[5],
         peer->addr[4],
         peer->addr[3],
         peer->addr[2],
         peer->addr[1],
         peer->addr[0]);

  for (int i = 0; i < BLE_PEER_LIST_SIZE; i++)
  {
    p = &ble_peer_list[i];
    if (p->ble_used == 0)
    {
      continue;
    }

    if (p->ble_isbadge == TRUE)
    {
      printf("found peer enemy %x:%x:%x:%x:%x:%x (TTL %d)\n",
             p->ble_peer_addr[5],
             p->ble_peer_addr[4],
             p->ble_peer_addr[3],
             p->ble_peer_addr[2],
             p->ble_peer_addr[1],
             p->ble_peer_addr[0],
             p->ble_ttl);
      if (memcmp(p->ble_peer_addr, peer->addr, 6) == 0)
      {
        // if we match, we create an enemy
        current_enemy = malloc(sizeof(ENEMY));
        memcpy(current_enemy->ble_peer_addr.addr, p->ble_peer_addr, 6);
        current_enemy->ble_peer_addr.addr_id_peer = TRUE;
        current_enemy->ble_peer_addr.addr_type    = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
        strcpy(current_enemy->name, (char *)p->ble_peer_name);
        current_enemy->ship_type = p->ble_game_state.ble_ides_ship_type;
        current_enemy->xp        = p->ble_game_state.ble_ides_xp;
        current_enemy->hp        = shiptable[current_enemy->ship_type].max_hp;
        current_enemy->energy    = shiptable[current_enemy->ship_type].max_energy;
        current_enemy->level     = p->ble_game_state.ble_ides_level;

        current_enemy->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        current_enemy->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        current_enemy->level          = p->ble_game_state.ble_ides_level;
        current_enemy->ship_locked_in = FALSE;
        return(current_enemy);
      }
    }
  }

  return(NULL);
}

void enemy_list_refresh(gll_t *enemies,
                        ISPRITESYS *sprites,
                        battle_state current_battle_state)
{
  ENEMY *         e;
  ble_peer_entry *p;

  // copy enemies from BLE
  osalMutexLock(&peer_mutex);

  for (int i = 0; i < BLE_PEER_LIST_SIZE; i++)
  {
    p = &ble_peer_list[i];

    if (p->ble_used == 0)
    {
      continue;
    }

    if (p->ble_isbadge == TRUE)
    {
      /* this is one of us, copy data over. */
      /* have we seen this address ? */
      /* addresses are 48 bit / 6 bytes */
      if ((e = enemy_find_by_peer(enemies, &p->ble_peer_addr[0])) != NULL)
      {
#ifdef DEBUG_ENEMY_DISCOVERY
        printf("enemy: found old enemy %x:%x:%x:%x:%x:%x (TTL %d)\n",
               p->ble_peer_addr[5],
               p->ble_peer_addr[4],
               p->ble_peer_addr[3],
               p->ble_peer_addr[2],
               p->ble_peer_addr[1],
               p->ble_peer_addr[0],
               p->ble_ttl);
#endif
        // is this dude in combat? Get rid of him.
        if (p->ble_game_state.ble_ides_incombat)
        {
          e->ttl = 0;
        }
        else
        {
          e->ttl = p->ble_ttl;
        }

        e->e.prevPos.x = e->e.vecPosition.x;
        e->e.prevPos.y = e->e.vecPosition.y;

        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        isp_set_sprite_xy(sprites,
                          e->e.sprite_id,
                          e->e.vecPosition.x,
                          e->e.vecPosition.y);

        e->xp        = p->ble_game_state.ble_ides_xp;
        e->level     = p->ble_game_state.ble_ides_level;
        e->hp        = shiptable[p->ble_game_state.ble_ides_ship_type].max_hp;
        e->energy    = shiptable[p->ble_game_state.ble_ides_ship_type].max_energy;
        e->ship_type = p->ble_game_state.ble_ides_ship_type;
      }
      else
      {
        /* no, add him */
        if (p->ble_game_state.ble_ides_incombat)
        {
#ifdef DEBUG_ENEMY_DISCOVERY
          printf("enemy: ignoring in-combat %x:%x:%x:%x:%x:%x\n",
                 p->ble_peer_addr[5],
                 p->ble_peer_addr[4],
                 p->ble_peer_addr[3],
                 p->ble_peer_addr[2],
                 p->ble_peer_addr[1],
                 p->ble_peer_addr[0]);
#endif
          continue;
        }

#ifdef DEBUG_ENEMY_DISCOVERY
        printf("enemy: found new enemy (ic=%d) %x:%x:%x:%x:%x:%x\n",
               p->ble_game_state.ble_ides_incombat,
               p->ble_peer_addr[5],
               p->ble_peer_addr[4],
               p->ble_peer_addr[3],
               p->ble_peer_addr[2],
               p->ble_peer_addr[1],
               p->ble_peer_addr[0]);
#endif
        e = malloc(sizeof(ENEMY));

        if (current_battle_state == COMBAT)
        {
          entity_init(&(e->e), sprites, SHIP_SIZE_ZOOMED, SHIP_SIZE_ZOOMED, T_ENEMY);
        }
        else
        {
          entity_init(&(e->e), sprites, SHIP_SIZE_WORLDMAP, SHIP_SIZE_WORLDMAP, T_ENEMY);
        }

        e->e.visible       = TRUE;
        e->e.vecPosition.x = p->ble_game_state.ble_ides_x;
        e->e.vecPosition.y = p->ble_game_state.ble_ides_y;

        isp_set_sprite_xy(sprites,
                          e->e.sprite_id,
                          e->e.vecPosition.x,
                          e->e.vecPosition.y);

        e->e.prevPos.x = p->ble_game_state.ble_ides_x;
        e->e.prevPos.y = p->ble_game_state.ble_ides_y;

        memcpy(&e->ble_peer_addr.addr, p->ble_peer_addr, 6);

        e->ble_peer_addr.addr_id_peer = TRUE;
        e->ble_peer_addr.addr_type    = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;

        // copy over game data
        strcpy(e->name, (char *)p->ble_peer_name);
        e->xp             = p->ble_game_state.ble_ides_xp;
        e->level          = p->ble_game_state.ble_ides_level;
        e->ship_type      = p->ble_game_state.ble_ides_ship_type;
        e->ship_locked_in = FALSE;
        e->hp             = shiptable[e->ship_type].max_hp;
        e->energy         = shiptable[e->ship_type].max_energy;
        gll_push(enemies, e);
      }
    }
  }
  osalMutexUnlock(&peer_mutex);
}
