#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "vector.h"

/*
 * We define entity types as macros rather than using an enum,
 * because these type definitions will also be used as part of the
 * network protocol packets, and using enums for over-the-wire
 * data structures is a bad idea. The internal storage format
 * for a C enum is Implemantation Defined (tm), which means
 * you can't count on it being a particular size. We need to
 * be unambiguous about the type and its size.
 */

typedef uint16_t entity_type;

#define T_PLAYER        0x01
#define T_ENEMY         0x02
#define T_BULLET_PLAYER 0x04
#define T_BULLET_ENEMY  0x08
#define T_MINE_PLAYER   0x10
#define T_MINE_ENEMY    0x11
#define T_SPECIAL       0x14

typedef struct _entity
{
  entity_type type;

  uint16_t    id;              /* random. used for lookups */
  ISPID       sprite_id;       /* sprite id */
  bool        visible;
  bool        blinking;
  bool        faces_right;

  int         ttl;             /* if -1, always visible, else a number of frames */

  VECTOR      vecVelocity;     /* current velocity */
  VECTOR      vecVelocityGoal; /* goal velocity */
  VECTOR      vecPosOrigin;    /* for bullets, this is their starting position */
  VECTOR      vecPosition;     /* current position */
  VECTOR      prevPos;         /* where we were on the last frame */
  VECTOR      vecGravity;
  VECTOR      vecPositionLast; /* if this doesn't match, we'll erase and repaint */

  uint8_t     size_x;
  uint8_t     size_y;
} ENTITY;

extern void
entity_init(ENTITY *p, ISPRITESYS *sprites, int16_t size_x, int16_t size_y, entity_type t);

#endif /* _ENTITY_H_ */
