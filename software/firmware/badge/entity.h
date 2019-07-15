#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "vector.h"

typedef uint16_t entity_type;
#define T_PLAYER        0x01
#define T_ENEMY         0x02
#define T_BULLET_PLAYER 0x03
#define T_BULLET_ENEMY  0x04
#define T_SPECIAL       0x05

typedef struct _entity
{
  entity_type type;

  uint16_t    id;         /* random. used for lookups */
  ISPID       sprite_id;  /* sprite id */
  bool        visible;
  bool        blinking;
  int         ttl;             /* if -1, always visible, else a number of frames */

  VECTOR      vecVelocity;     /* current velocity */
  VECTOR      vecVelocityGoal; /* goal velocity */
  VECTOR      vecPosition;     /* position */
  VECTOR      prevPos;         /* where we were on the last frame */
  VECTOR      vecGravity;
  VECTOR      vecPositionLast; /* if this doesn't match, we'll erase and repaint */

  uint8_t     size_x;
  uint8_t     size_y;
} ENTITY;

extern void
entity_init(ENTITY *p, ISPRITESYS *sprites, int16_t size_x, int16_t size_y, entity_type t);

#endif /* _ENTITY_H_ */
