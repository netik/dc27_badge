#include "ch.h"
#include "hal.h"

#include "orchard-app.h"
#include "string.h"
#include "fontlist.h"
#include "ides_gfx.h"
#include "scroll_lld.h"

#include "gfx.h"
#include "src/gdisp/gdisp_driver.h"

#include "ffconf.h"
#include "ff.h"
#include "async_io_lld.h"
#include "badge.h"
#include "ides_sprite.h"
#include "sprite_fx.h"

static ISPRITESYS *iss=NULL;
static FXLIST fxl = { .count =0 };


void fx_destroy_ent(FX_OBJ *f);
void fx_list_remove(int pos);
void fx_destroy_ent(FX_OBJ *f);
FX_OBJ *fx_make_ent(void );
void fx_update(void);

void fx_init(ISPRITESYS *i)
{
  iss = i;
  memset(&fxl.list, 1, sizeof(FXLIST));
}

void fx_shutdown()
{

  /* fx_list_remove decrements fxl.count */
  while(fxl.count > 0)
  {
    fx_list_remove(0);
  }
}

void fx_list_add(FX_OBJ *f)
{
  if(fxl.count < (MAX_FX))
  {
    fxl.list[fxl.count] = f;
    fxl.count++;
  }
}

void fx_list_remove(int pos)
{
  int i;
  if( (pos >= 0) &&  (pos < fxl.count) )
  {
    fx_destroy_ent(fxl.list[pos]);
    /* if this is the last one in the list, just free and NULL it.*/
    for(i=pos; i < (fxl.count-1); i++)
    {
      fxl.list[i] = fxl.list[i+1];
    }
    fxl.count--;
  }
}

void fx_destroy_ent(FX_OBJ *f)
{

  if(NULL != f)
  {
    isp_destroy_sprite(iss, f->sprite);
    free(f);
  }
}


FX_OBJ *fx_make_ent(void )
{
  FX_OBJ *f=NULL;
  f = calloc(1, sizeof(FX_OBJ));
  if( (NULL != f) &&
      (NULL != iss) )
  {
    f->sprite = isp_make_sprite(iss);
    if(ISP_MAX_SPRITES == f->sprite)
    {
      free(f);
      f = NULL;
    }
    else
    {
      f->birth_time = chVTGetSystemTime();
    }
  }
  return(f);
}






/* creates a box that shrinks, lasts for spacified number of frames and shrinks to 1x1 px in that time. */
/* returns ISPID of the sprite associated with this object. or -1 on fail.  */
ISPID fx_make_sizer_box(coord_t x, coord_t y, coord_t sz_s, coord_t sz_e, color_t c_s, color_t c_e, int delay, int lifespan)
{
  FX_OBJ *f;
  ISPID ret = -1;

  f = fx_make_ent();
  if(NULL != f)
  {
    lifespan = fix_range(lifespan, 10, 5000);
    delay = fix_range(delay, 0, 5000);

    f->lifespan = lifespan;
    f->delay = delay;
    f->x = x;
    f->y = y;
    f->p1a = fix_range(sz_s, 2, 80);
    f->p1b = fix_range(sz_e, 2, 80);
    f->p2a = (int)c_s;
    f->p2b = (int)c_e;
    f->type = FX_SIZER;
    ret = f->sprite;
    fx_list_add(f);
  }
  return(ret);
}




void fx_update(void)
{
  static systime_t last_time;
  systime_t this_time;
  int delta, i, p1p, p2p, precol;
  float progress;
  color_t r1,g1,b1,r2, g2, b2, r,g,b, col;
  int  xx, yy;
  pixel_t *buf;
  FX_OBJ *f;

  this_time = chVTGetSystemTime();
  delta = this_time - last_time;
  last_time = this_time;
  if(fxl.count > 0)
  {

    for(i=0; i < fxl.count; i++)
    {
      f = fxl.list[i];

      if(f->delay > 0)
      {
        f->delay -= delta;
      }
      else
      {
        progress = (float)(f->age) / (float)(f->lifespan);
        if(progress > 1.0)
        {
          progress = 1.0;
        }
        p1p = (int) f->p1a + (int)((f->p1b - f->p1a) * progress);
        p1p = fix_range(p1p, f->p1b, f->p1a);
        p2p = (int) f->p2a + (int)((f->p2b - f->p2a) * progress);
        p2p = fix_range(p1p, f->p2b, f->p2a);
        /* don't update unless there is change */
        if( (f->has_drawn == 0) || (f->p1z != p1p) || (f->p2z != p2p) )
        {
          if(p1p < 1) { p1p = 1; }
          f->has_drawn = 1;
          f->p1z = p1p;
          f->p2z = p2p;
          switch(f->type)
          {
            case FX_SIZER:

              if(p1p < 2)
              {
                p1p = 2;
              }
              xx = f->x - (p1p/2);
              yy = f->y - (p1p/2);
              if(xx < 0) { xx =0; }
              if(yy < 0) { yy =0; }
              r1 = GET_PX_RED(f->p2a);
              g1 = GET_PX_GREEN(f->p2a);
              b1 = GET_PX_BLUE(f->p2a);
              r2 = GET_PX_RED(f->p2b);
              g2 = GET_PX_GREEN(f->p2b);
              b2 = GET_PX_BLUE(f->p2b);

              r = r1 + ( (r2 - r1) * progress);
              g = g1 + ( (g2 - g1) * progress);
              b = b1 + ( (b2 - b1) * progress);
              precol = ( r *65536) + (g * 256) + (b);
              col = HTML2COLOR(precol);

              buf = boxmaker((coord_t)p1p, (coord_t)p1p, col );
              isp_set_sprite_block(iss, f->sprite, p1p, p1p, buf);
              free(buf);
              isp_set_sprite_xy(iss, f->sprite, xx,yy );
            break;
            default:
            break;
          }
        }
        f->age += delta;
      }
      if(f->age >= f->lifespan)
      {
        fx_list_remove(i);
      }


    }
  }

}
