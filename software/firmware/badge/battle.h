#ifndef _BATTLE_H_
#define _BATTLE_H_

#include "vector.h"

typedef struct _player {
  VECTOR vecVelocity;     /* current velocity */
  VECTOR vecVelocityGoal; /* goal velocity */
  VECTOR vecPosition;     /* position */
  VECTOR vecGravity;
} PLAYER;

#define HARBOR_LWR_X 24
#define HARBOR_LWR_Y 225

#define HARBOR_UPP_X 252
#define HARBOR_UPP_Y 60

#endif /* _BATTLE_H_ */
