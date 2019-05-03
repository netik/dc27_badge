#ifndef _BATTLE_H_
#define _BATTLE_H_

#include "vector.h"

/* Old pixel data */
#define FB_X 10
#define FB_Y 10

typedef struct _entity {
  bool visible;
  VECTOR vecVelocity;     /* current velocity */
  VECTOR vecVelocityGoal; /* goal velocity */
  VECTOR vecPosition;     /* position */
  VECTOR vecGravity;
  pixel_t pix_old[FB_X * FB_Y]; /* frame buffer */
} ENTITY;

typedef struct _enemy {
  ble_gap_addr_t addr;
  char name[16];
  uint8_t level;
  ENTITY e;
} ENEMY;

typedef struct _bullet {
  uint8_t kind;
  ENTITY b;
} BULLET;

#define HARBOR_LWR_X 24
#define HARBOR_LWR_Y 225

#define HARBOR_UPP_X 252
#define HARBOR_UPP_Y 60

#endif /* _BATTLE_H_ */
