/* entity.c - entity management functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "orchard-app.h"
#include "orchard-ui.h"
#include "fontlist.h"
#include "nrf52i2s_lld.h"

#include "rand.h"
#include "images.h"
#include "ides_gfx.h"
#include "ides_sprite.h"
#include "ships.h"
#include "entity.h"

void
entity_init(ENTITY *p, ISPRITESYS *sprites, int16_t size_x, int16_t size_y, entity_type t)
{
  pixel_t *buf;
  color_t  color = Black;

  p->id       = randUInt16();
  p->type     = t;
  p->visible  = FALSE;
  p->blinking = FALSE;
  p->ttl      = -1;

  p->size_x = size_x;
  p->size_y = size_y;

  p->vecVelocity.x = 0;
  p->vecVelocity.y = 0;

  p->vecVelocityGoal.x = 0;
  p->vecVelocityGoal.y = 0;

  // we'll set this to 0,0 for now. The caller should immediately set this.
  p->vecPosition.x = 0;
  p->vecPosition.y = 0;

  // this is "ocean drag"
  p->vecGravity.x = VDRAG;
  p->vecGravity.y = VDRAG;

  p->sprite_id = isp_make_sprite(sprites);

  switch (p->type)
  {
  case T_PLAYER:
    color = White;
    break;

  case T_ENEMY:
    color = Red;
    break;

  case T_BULLET_PLAYER:
    color = Black;
    break;

  case T_BULLET_ENEMY:
    color = Red;
    break;

  case T_SPECIAL:
    color = Yellow;
    break;
  }

  buf = boxmaker(size_x, size_y, color);
  isp_set_sprite_block(sprites, p->sprite_id, size_x, size_y, buf);
  free(buf);
}
