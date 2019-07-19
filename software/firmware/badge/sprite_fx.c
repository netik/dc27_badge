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
static FXLIST fxl = { .list = NULL, .count =0 };


void fx_destroy_ent(FX_OBJ *f);
void fx_list_remove(int pos);
void fx_destroy_ent(FX_OBJ *f);
FX_OBJ *fx_make_ent(void );
void fx_update(void);

void fx_init(ISPRITESYS *i)
{
  iss = i;
  fxl.list = NULL;
  fxl.count=0;
}

void fx_shutdown(ISPRITESYS *i)
{
  iss = i;
}

void fx_list_add(FX_OBJ *f)
{
  FX_OBJ **l;
  int i;
  l = malloc( (fxl.count + 1) * sizeof(FX_OBJ *));
  if(NULL != fxl.list)
  {
    for(i=0; i < fxl.count; i++)
    {
      l[i] = fxl.list[i];
    }
    free(fxl.list);
  }
  l[fxl.count] = f;
  fxl.list = l;
  fxl.count++;
}

void fx_list_remove(int pos)
{
  if(pos < fxl.count)
  {
    FX_OBJ **l;
    int i;
printf("remove pos:%d c:%d",pos, fxl.count);
    if( (pos >= 0) && (pos < fxl.count) && (NULL != fxl.list) )
    {

      fx_destroy_ent(fxl.list[pos]);
      /* if this is the last one in the list, just free and NULL it.*/
      if(fxl.count > 1)
      {
        l = malloc( (fxl.count - 1) * sizeof(FX_OBJ *));
        if(NULL != fxl.list)
        {
          for(i=0; i < fxl.count; i++)
          {
            if(i != pos)
            {
              l[i] = fxl.list[i];
            }
          }
        }
      }
      else
      {
        l = NULL;
      }
      fxl.count--;
      free(fxl.list);
      fxl.list = l;
    }
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
      fx_destroy_ent(f);
      f = NULL;
    }
    else
    {
      f->birth_time = chVTGetSystemTime();
    }
  }
  return(f);
}




void fix_range(int *i, int min, int max)
{
  if(*i > max)
  {
    *i = max;
  }
  if(*i < min)
  {
    *i = min;
  }
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
    fix_range(&lifespan, 10, 5000);
    fix_range(&delay, 0, 5000);

    f->lifespan = lifespan;
    f->delay = delay;
    f->x = x;
    f->y = y;
    f->p1a = (int)sz_s;
    f->p1b = (int)sz_e;
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
    /* time since last call of this.  currently lifespan is frames */
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
        p2p = (int) f->p2a + (int)((f->p2b - f->p2a) * progress);

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
