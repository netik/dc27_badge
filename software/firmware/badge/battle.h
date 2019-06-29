#ifndef _BATTLE_H_
#define _BATTLE_H_

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"

#include "vector.h"
#include "gfx.h"

enum entity_type { T_PLAYER, T_ENEMY, T_BULLET, T_SPECIAL };

typedef struct _entity {
  enum entity_type type;
  bool visible;
  bool blinking;
  int ttl;                /* if -1, always visible, else a number of frames */

  VECTOR vecVelocity;     /* current velocity */
  VECTOR vecVelocityGoal; /* goal velocity */
  VECTOR vecPosition;     /* position */
  VECTOR prevPos;         /* where we were on the last frame */
  VECTOR vecGravity;
  VECTOR vecPositionLast; /* if this doesn't match, we'll erase and repaint */

  /* contents of the previous area */
  bool pix_inited;

  uint8_t size_x;
  uint8_t size_y;

  pixel_t *pix_old; /* frame buffer */

} ENTITY;

typedef struct _enemy {
  ble_gap_addr_t  ble_peer_addr;
  char     name[16];
  uint8_t  level;
  uint16_t xp;
  uint8_t  ship_type;
  uint8_t  ttl;
  ENTITY   e;
} ENEMY;

typedef struct _bullet {
  uint8_t kind;
  ENTITY b;
} BULLET;

#endif /* _BATTLE_H_ */
