
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
#include "images.h"
#include "ides_sprite.h"
#include "sprite_fx.h"
#include <math.h>

/* stored default structs for initializing */
static const ISPBUF isbuf_default = {
  .buf = NULL,
  .x=0,
  .y=0,
  .xs=0,
  .ys=0
};

const ISPRITE isprite_default = {
  .bgtype = ISP_BG_DYNAMIC,
  .active = FALSE,
  .alphamap = NULL,
  .xoffs = 0,
  .yoffs = 0,
  .visible = FALSE,
  .auto_bg = TRUE,
  .mode_switch = FALSE,
  .full_restore = FALSE,
  .bgcolor = HTML2COLOR(0x2603fb)
};

typedef struct _4rect {
  RECT bx[4];
} FOUR_RECTS;

const RECT rect_default = {.x=0, .y=0, .xr=0, .yb=0};

static int allo_num=0;


void *dmalloc(size_t s, char *msg)
{
  void *ret;
  ret = malloc(s);
  if(NULL != ret)
  {
      allo_num++;
  }

//    printf("%08x  , malloc %d, anum:, %d, - %s\n", ret, s, allo_num,msg);
  return(ret);
}

void *dcalloc(size_t num, size_t siz, char *msg)
{
  void *ret;
  ret = calloc(num, siz);
  if(NULL != ret)
  {
      allo_num++;
  }

//    printf("%08x , calloc %d/%d , anum:, %d, - %s\n", ret, num, siz, allo_num, msg);
  return(ret);
}

void dfree(void *ptr, char *msg)
{
  allo_num--;
  free(ptr);
//  printf("%08x , free , anum:, %d, - %s\n", ptr,  allo_num, msg);

}

/* This is a library for simplifying sprite usage.
 * it supports
 *

ISPRITESYS *sl; // the sprite system.
ISPID s1, s2, s3; // ID numbers for each sprite.
int i=0;

// initialize a sprite system.  each ISPRITESYS does up to ISP_MAX_SPRITES at a time.
// you can create and destroy sprites all you want.
// it's often better to hide them if it's a re-used asset like a bullet.
// more than one spite system can be open and/or active at a time.
sl = isp_init();

// each new sprite has an ID number.
s1 = isp_make_sprite(sl); // make sprites.  none are drawn yet.
s2 = isp_make_sprite(sl); // they have no position or pixel data yet.
s3 = isp_make_sprite(sl);




  while()
  {
    i++;
    do_stuff();

    // pretend xpos/ypos are functions that return some position value
    // this is how you change the positional values for the sprites.
    isp_set_sprite_xy(sl, s1, xpos(i), ypos(i));
    isp_set_sprite_xy(sl, s2, xpos(i)+10, ypos(i)+10);
    isp_set_sprite_xy(sl, s3, xpos(i)+20, ypos(i)+20);


    // example of turning a sprite off.
    // maybe it's off screen, maybe it's not needed at the moment, but this is faster
    // than creating and destroying sprites constantly.
    if(should_s3_be_invisible())
    {
      hide_sprite(sl, s3);
    }
    else
    {
      show_sprite(sl, s3);
    }

    isp_draw_all_sprites(sl);
  }



  isp_shutdown(sl);  // close and free this sprite system.


*/

int  fix_range(int i, int a, int b)
{
  int min, max;
  min = (a < b)? a : b;
  max = (a > b)? a : b;

  if(i > max)
  {
    i = max;
  }
  if(i < min)
  {
    i = min;
  }
  return(i);
}

void timeit(int g, char *str)
{
  static systime_t f=0,s=0;
  if(g==0)
  {
    f = chVTGetSystemTime();
  }
  else
  {
    s = chVTGetSystemTime();
    printf("%s time: %lu\n", str, (s-f) );
  }
}


WMAP *wm_make_wmap_size(coord_t width, coord_t height)
{
  WMAP *w=NULL;

  width = fix_range(width, 0, SCREEN_W);
  height =  fix_range(height, 0, SCREEN_H);

  if( (height > 0) && (width > 0) )
  {
    w = dcalloc(1, sizeof(WMAP), "wm_make_wmap_size: w alloc");

    if ( NULL != w )
    {
      w->h = height;
      w->w = width;
  /*
  printf("wm blk alloc %d w:%d h:%d\n", height * sizeof(WROW), width, height);
  fflush(stdout);
  */
      w->map = dcalloc(height, sizeof(WROW), "wm_make_wmap_size: w->map alloc");

    }
  }
  return(w);
}

WMAP *wm_make_screensize_wmap(void)
{
  return(wm_make_wmap_size(SCREEN_W, SCREEN_H));
}


void wmap_header_dump(WMAP *w, char *name)
{
  int i, j, first;
  char rowmap[128];
  sprintf(rowmap, "%s_rows", name);

  printf("WROW %s[] = { \n ", rowmap);
  for(i=0; i < w->h; i++)
  {
    printf("  { .row[] = { ");
    first=TRUE;
    for(j=0; j < WMAP_W; j++)
    {
      if(!first)
      {
        printf(", ");
      }
      printf("0x%08lx", w->map[i].row[j]);
      first = FALSE;
    }
    printf("}, \n");
    printf("   .first_px=0, .last_px=0, .notblank=TRUE };\n");
  }
  printf("}\n\n");
  printf("WMAP %s = { .h=%d, .w=%d, map=&%s }\n", name, w->h, w->w, rowmap);
}

/* so far just a free, but if this chjanges later, this would handle shutdown cleanly. */
void wm_destroy_wmap(WMAP *w)
{
  if(NULL != w)
  {
    if(NULL != w->map)
    {
      dfree(w->map, "destroy_wmap w->map");
      w->map = NULL;
    }
    dfree(w, "destroy_wmap w");
  }
}

WMAP *wm_copy_wmap(WMAP *w)
{
  WMAP *ret = NULL;
  int i;
  if( (NULL != w->map) &&
      (w->h > 0) &&
      (w->w > 0) )
  {
    ret = wm_make_wmap_size(w->w, w->h);
    if(NULL != ret)
    {
      for(i=0; i < w->h; i++)
      {
        ret->map[i] = w->map[i];
      }
    }
  }
  return(ret);
}


static void wm_set_first_last_per_row(WMAP *w)
{
  coord_t y, x,last;
  if(NULL != w)
  {
    last = (w->w - 1);

    for(y=0; y < w->h; y++)
    {
      if(TRUE == w->map[y].notblank)
      {
        w->map[y].first_px = 0;
        w->map[y].last_px  = last;

        for(x=0; x < w->w; x++)
        {
          if(wm_value(w, x,y, 3))
          {

            w->map[y].first_px = x;
            break;
          }
        }
        for(x = last ; x >= w->map[y].first_px; x--)
        {
          if(wm_value(w, x,y, 3))
          {
            w->map[y].last_px = x;
            break;
          }
        }
      }
      else
      {
        w->map[y].first_px = last;
        w->map[y].last_px  = 0;
      }
    }
  }
}


/* iv v is 0 or 1, it sets the value.  if it's > 1, it GETS the value. */
bool_t wm_value(WMAP *w, coord_t x, coord_t y, uint8_t v)
{
  bool_t ret;
  int  offs = 0, bit=0;
  offs = WM_STEP(x);
  bit  = WM_BIT(x);
  ret=v;
/*
printf("x%u y%u offs%u bit %u w%d - %u wh%x bit%x\n", x, y, offs, bit,w->h, v, &w->h, &w->map[y].row[offs]);
*/
  if( (y < w->h) && (x < w->w) )
  {
    if(0 == v)
    {
      w->map[y].row[offs] &= ~(1 << bit);
    }
    else if(1 == v)
    {
      w->map[y].row[offs] |= 1<< bit;
      w->map[y].notblank = TRUE;
    }
    else
    {
      ret = (w->map[y].row[offs] >> bit) & 1;
    }
/*
printf("x%u, y%u, v%u off%u bit %u ret:%u-- %lx\n", x,y,v, offs, bit, ret, w->map[y].row[offs]);
*/
  }
  return(ret);
}





/* mapx mapy are the positions in the WMAP to start writing
 * bxs, bys are the size of buf.
 * mapx amd mapy are the starting coords on the wmap to place the info from the scanned buffer.
 * we scan the screen map in chunks because it's too big to buffer the whole thing.
 * bxs and bys are buffer X size and buffer Y size.
 */
bool_t wm_scan_from_img(WMAP *w, coord_t mapx, coord_t mapy, coord_t bxs, coord_t bys, pixel_t *buf)
{
  coord_t bufx, bufy, wmx, wmy, pos;
  bool_t ret=FALSE;
  pixel_t px;

  /*  if we have a buffer, & scanned image will not overrun the map, then scan it. */
  /*
  printf("enter wm_scan_from_img x%d y%d, sx%u sy%u w%u h%u\n", mapx, mapy, bxs, bys, w->w, w->h);
  fflush(stdout);
*/
  if( (NULL != buf) && (w->w >= (mapx+bxs)) && (w->h >= (mapy+bys)) )
  {
    for(bufy=0; bufy < bys; bufy++)
    {
      for(bufx=0; bufx < bxs; bufx++)
      {
        pos = (bufy*bxs)+bufx;
        px=buf[pos];
        if( (GET_PX_BLUE(px) < BLUE_THRESH ) ||
              (GET_PX_GREEN(px) > NOTBLUE_THRESH) ||
              (GET_PX_RED(px) > NOTBLUE_THRESH) )
        {
          wmx = mapx + bufx;
          wmy = mapy + bufy;
/*
printf("notblue: %d  x%d y%d -- r%d g%d b%d\n",px, wmx, wmy, GET_PX_RED(px), GET_PX_GREEN(px), GET_PX_BLUE(px));
*/
          wm_value(w, wmx, wmy, 1);
        }
      }
/*      printf("%s\n", line); */
    }
    ret = TRUE;
  }
/*printf("exit wm_scan_from_img x%d y%d, sx%u sy%u w%u h%u\n", mapx, mapy, bxs, bys, w->w, w->h);
*/
  return(ret);
}




/* scrapes the screen & builds the bitmask, setting water to 0 and land to 1 */
WMAP *wm_build_land_map_from_screen(void)
{
  coord_t y, bufx, bufy;
  pixel_t *buf=NULL;
  WMAP *w;

  y=0;
/* size of screen-scraping buffer.  not enough ram to scrape whole screeen */
  bufx = SCREEN_W;
  bufy = 20;
//printf("land map c alloc %d\n", bufx * bufy * sizeof(pixel_t));
  buf = dcalloc(bufx * bufy, sizeof(pixel_t), "wm_build_land_map_from_screen");
  w = wm_make_wmap_size(SCREEN_W, SCREEN_H);

  for(y=0; y < SCREEN_H; y += bufy)
  {
    /* grab 20 rows at a time */
    getPixelBlock(0, y, bufx, bufy, buf);
    wm_scan_from_img(w, 0, y, bufx, bufy, buf);
  }
  dfree(buf,"wm_build_land_map_from_screen");
  return(w);
}


/* makes a RECT bounding box around the solid, non-transparent pixels */
RECT wm_make_bounding_box(WMAP *w)
{
  coord_t top, bot, left, right, y;
  RECT r;

  top = 0;
  left =0;
  bot = (w->h -1);
  right = (w->w -1);

  for(y=0; y < bot; y++)
  {
      /* any pixel bit set to 1 means the row is not blank */
    if(TRUE == w->map[y].notblank)
    {
      top = y;
      break;
    }
  }

  /* look for blank rows on the bottom */
  for(y=bot; y > top; y--)
  {
    if(TRUE == w->map[y].notblank)
    {
      bot = y;
      break;
    }
  }
  wm_set_first_last_per_row(w);
  /* now look for notblank on the left side of the sprite
   */
  left = (w->w -1);
  right = 0;
  for(y=top; y < bot; y++)
  {
    if(TRUE == w->map[y].notblank)
    {
      /* left is the smallest found first_px in the list */
      left  = (left > w->map[y].first_px) ? w->map[y].first_px : left;
     /* right -s the largest last_px found in the list. */
      right = (right < w->map[y].last_px) ? w->map[y].last_px : right;
    }
    /* if we find soild pixels in the outer most positions, stop looking */
    if( (0 == left) && (w->w == right) )
    {
      break;
    }
  }

  /* now start on the left and look to the right for first pixel
   */

  r.x  = left;
  r.y  = top;
  r.xr = right;
  r.yb = bot;

  return(r);
}

void print_rect(char *s, RECT r)
{
  printf("%s: x:%d y:%d xr:%d yb:%d\n", s, r.x, r.y, r.xr, r.yb);
}

/* pass valid rect to limit collision test to those coords */
/* currenly uses that as course guide, it starts at the (int) containing that coord.
 * so in this case, we say 45, but this will start at 32, which is the start of that
 * int boundary.  to do it fully proper, we should mask off the extra bits on either side.  But if
 * we are not packing more than one shape into the comparison map at a time, it will be fine.
 *  01234567890123456789801234567891  23456789012345678901234567890123
 * [................................][.............|||||||||||........]
 *
 */
bool_t wm_collision_check_maps(WMAP *mapa, WMAP *mapb, RECT r)
{
  coord_t x=0, y=0, xmax=0, ymax=0, xstart=0;
  bool_t ret=FALSE;

  if(is_valid_rect(r))
  {
    xstart = WM_STEP(r.x);
    y = r.y;
    xmax = WM_STEP(r.xr);
    ymax = r.yb;
  }
  else
  {
    xmax = WM_STEP(mapa->w);
    ymax = mapa->h;
  }

  for( ; y < ymax; y++)
  {
    for(x=xstart ; x <= xmax; x++)
    {
      if(mapa->map[y].row[x] & mapb->map[y].row[x])
      {
        ret = TRUE;
        break;
      }
    }
    if(TRUE == ret)
    {
      break;
    }
  }
  return(ret);
}


void print_wmap_hits(WMAP *w)
{
  coord_t x,y;
  char s[200], st[200];
  int t=FALSE;
  for(y=0; y < w->h; y++)
  {
    strcpy(s, "");
    strcpy(st, "");
    for(x=0; x < WMAP_W; x++)
    {
      if(w->map[y].row[x])
      {
        if(t==FALSE)
        {
          sprintf(s, "x=%d ", x);
        }

        sprintf(st, "%lx ", w->map[y].row[x]);
        strcat(s, st);
        t=TRUE;
      }
    }
    if(t)
    {
      printf("y:%d - %s\n", y, s);
      t=FALSE;
    }
  }
}


/* makes a box of 1 values on a screen sized WMAP based on the passed in RECT.
 * used for box collision against land
 */
bool_t wm_make_collision_box(WMAP *w, RECT r)
{
  coord_t x,y,first, width, height;
  bool_t ret=FALSE;
  if(is_valid_rect(r))
  {
    first=r.y;
    /* it's a box.  properly set the first row, then duplicate that into the rest for speed. */
    width  = (r.xr > w->w) ? w->w : r.xr;
    height = (r.yb > w->h) ? w->h : r.yb;

    for(x=r.x; x < width; x++)
    {
      wm_value(w, x, first, 1);
    }
    for(y=first+1; y < height; y++)
    {
      memcpy(w->map[y].row, w->map[first].row, sizeof(WROW));
    }
    ret=TRUE;
  }
  return(ret);
}


bool_t wm_check_box_for_land_collision(WMAP *w, RECT r)
{
  bool_t ret = FALSE;
  WMAP *test;
  if( (NULL != w) && (is_valid_rect(r)) )
  {
    test = wm_make_screensize_wmap();
    wm_make_collision_box(test, r);
/*
  print_wmap_hits(test);
*/
  /* make a single row that has 1 in every X coord where the RECT covers.
   * no need to do this for every row, it's box, every row is the osalMutexLock */

    ret = wm_collision_check_maps(w, test, r);
    wm_destroy_wmap(test);
  }
  return(ret);
}



bool_t do_rects_clip(RECT a, RECT b)
{
  /* check for lack-of-collision, and invet value.
   * so if leftside of box2 is to the right of of the right-side of box1, they can't be intersecting
   */

 return( !(b.x > a.xr ||
           b.xr < a.x ||
           b.y > a.yb || /* larger Y values are lower on screen */
           b.yb < a.y) );
}



RECT make_rect(ISPBUF ib)
{
  static RECT r;
  r.x  = ib.x;
  r.y  = ib.y;
  r.xr = ib.x + ib.xs;
  r.yb = ib.y + ib.ys;
  return(r);
}

/* makes a RECT that encmopases both passed ISBUF areas
*/
RECT make_bounding_rect(ISPBUF a, ISPBUF b)
{
  static RECT r;
  coord_t aa, bb;
  r.x  = (a.x < b.x) ? a.x : b.x;
  r.y  = (a.y < b.y) ? a.y : b.y;
  aa = a.x + a.xs;
  bb = b.x + b.xs;
  r.xr = (aa > bb) ? aa : bb;
  aa = a.y + a.ys;
  bb = b.y + b.ys;
  r.yb = (aa > bb) ? aa : bb;
  return(r);
}


/* return false if RECT is offscreen or invalid shape - i.e. X (left) is to the right of xr (right) */
bool_t is_valid_rect(RECT r)
{
  bool_t ret = FALSE;
  if( (r.x < r.xr) && (r.y < r.yb) &&
      (r.xr < SCREEN_W) && (r.yb < SCREEN_H) )
  {
    ret = TRUE;
  }
  return(ret);
}

FOUR_RECTS get_exposed_bg_boxes(ISPRITESYS *iss, ISPID id)
{
  RECT bg, sp;
  FOUR_RECTS ret;
  coord_t lln, rln;


  memset(&ret, 1, sizeof(FOUR_RECTS));
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (iss->list[id].visible) &&
      (iss->list[id].bg_buf.xs > 0) &&
      (iss->list[id].sp_buf.xs > 0))
  {
    bg = make_rect(iss->list[id].bg_buf);
    sp = make_rect(iss->list[id].sp_buf);

    if(do_rects_clip(bg, sp))
    {
      /* there are up to 4 rectangles possible.  if the sprite is smaller than
      * the BG, but wholy contained by it, all 4 are possible */

      /* is there left rec?
       * bg left edge is to the left of sprite left edge. */
      if( (bg.x < sp.x) && (bg.x >= 0) )
      {
        ret.bx[0].x  = bg.x;
        ret.bx[0].xr = sp.x;
        ret.bx[0].y  = bg.y;
        ret.bx[0].yb = bg.yb;
        lln = sp.x;
      }
      else
      {
        lln=bg.x;
      }

      /* right? */
      if(bg.xr > sp.xr)
      {
        ret.bx[1].x  = sp.xr;
        ret.bx[1].xr = bg.xr;
        ret.bx[1].y  = bg.y;
        ret.bx[1].yb = bg.yb;
        rln = sp.xr;
      }
     else
     {
       rln = bg.xr;
     }

      /* top */
      if( (bg.y < sp.y) && (bg.y >= 0) )
      {
        ret.bx[2].x  = lln;
        ret.bx[2].xr = rln;
        ret.bx[2].y  = bg.y;
        ret.bx[2].yb = sp.y;
      }

      /* bot */
      if(bg.yb > sp.yb)
      {
        ret.bx[3].x  = lln;
        ret.bx[3].xr = rln;
        ret.bx[3].y  = sp.yb;
        ret.bx[3].yb = bg.yb;
      }
    }
  }
  return(ret);
}

void isp_copy_ipsbuf(ISPBUF *dst, ISPBUF *src)
{
  int size;
  if( (src->xs > 0) &&
      (src->ys > 0) &&
      (NULL != src->buf) )
  {
    /* duplicate everything.  the .buf is a pointer, so null that on dest,
     * and then allocate and copy from source.
     */
    memcpy(dst, src, sizeof(ISPBUF));

    size = src->xs * src->ys * sizeof(pixel_t);
    dst->buf = dmalloc(size, "isp_copy_ipsbuf()");
    if(dst->buf == NULL)
    {
      printf("Error: Unable to alloc %d bytes for sprite size X: %d Y: %d\n", size, src->xs, src->ys);
    }
    else
    {
      memcpy(dst->buf, src->buf, size);
    }
  }
}

void isp_destroy_ispbuf(ISPBUF *b)
{
  if(NULL != b->buf)
  {
    dfree(b->buf, "isp_destroy_ispbuf");
    *b = isbuf_default;
    b->buf = NULL;
  }
}

static pixel_t *crop_buf(coord_t *xs, coord_t *ys, coord_t *xoffs, coord_t *yoffs, RECT r, pixel_t *src)
{
  pixel_t  *buf=NULL;
  RECT br;
  coord_t y, width, height, copysize, src_offs, tgt_offs, src_wid;

  br.x  = 0;
  br.y  = 0;
  br.xr = *xs -1;
  br.yb = *ys -1;
  /* crop if needed */
  if( (is_valid_rect(r))  &&
      ((r.x > 0) || (r.y > 0) || (r.xr < br.xr) || (r.yb < br.yb)) )
  {
    width  = (r.xr+1) - r.x;
    height = (r.yb+1) - r.y;
    copysize = width * sizeof(pixel_t);

// printf("crop alloc %d\n", height * copysize);
    buf = dmalloc( height * copysize, "new buffer in crop_buf");
    src_wid = *xs;
    /* doing this mainly to have the memcpy code easier to read.*/
    for(y = 0; y < height; y++)
    {
      tgt_offs = y * width;
      src_offs = ((y + r.y) * src_wid) + r.x;
      memcpy(&buf[tgt_offs], &src[src_offs], copysize);
    }
    *xoffs = r.x;
    *yoffs = r.y;
    *xs = width;
    *ys = height;
  }
  return(buf);
}






void isbuf_dump(ISPBUF *s, char *str)
{
  printf("x:%d, y:%d, xs:%d, ys:%d -- %s\n", s->x, s->y, s->xs, s->ys, str);
}
void wm_dump(WMAP *w)
{
  printf("wmap h%d w%d\n", w->h, w->w);
}
void sprite_dump(ISPRITESYS *iss, ISPID id, char *str)
{
  ISPRITE *s;
  s = &iss->list[id];
  printf("-- Sprite dump #%d %s --\n", id, str);
  isbuf_dump(&s->bg_buf, "bg");
  isbuf_dump(&s->sp_buf, "sprite");
  printf("xoff:%d yoff:%d, bgt:%d stat:%d - active:%d vis:%d autobg:%d modsw:%d fr:%d\n",
         s->xoffs, s->yoffs, s->bgtype, s->status, s->active, s->visible, s->auto_bg, s->mode_switch, s->full_restore);
  if(NULL != s->alphamap)
  {
    wm_dump(s->alphamap);
  }
  else
  {
    printf("no alphamap\n");
  }

}

/* restores BG for  sprites */
static void isp_restore_bg(ISPRITESYS *iss, ISPID id)
{
  FOUR_RECTS rects;
  int i;
  coord_t xs, ys;
  bool_t drew=FALSE;


  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (ISP_STAT_DIRTY_BOTH == iss->list[id].status)  &&
      (iss->list[id].bg_buf.xs > 0) &&
      (iss->list[id].bg_buf.ys > 0) )
  {
    switch(iss->list[id].bgtype)
    {
      /* add code to fill only the exposed parts of the BG rectangle.
       * don't need to fill areas that will be covered by the sprite
       */
      case ISP_BG_FIXEDCOLOR:
        /* if it's auto_bg, and we are changing modes from Fixedcolor, fully wipe the sprite */
        if(iss->list[id].full_restore)
        {
          gdispFillArea(iss->list[id].bg_buf.x,
                        iss->list[id].bg_buf.y,
                        iss->list[id].bg_buf.xs,
                        iss->list[id].bg_buf.ys,
                        iss->list[id].bgcolor);
/*
printf(" ****** big fill x%d y%d xs%d ys%d\n", iss->list[id].bg_buf.x,iss->list[id].bg_buf.y,iss->list[id].bg_buf.xs, iss->list[id].bg_buf.ys);
*/
          iss->list[id].full_restore = FALSE;
        }
        else
        {
          drew=FALSE;
          rects = get_exposed_bg_boxes(iss, id);

          for(i=0; i< 4; i++)
          {
            if( (rects.bx[i].xr > 0) &&
                (rects.bx[i].yb > 0) )
            {
              xs=rects.bx[i].xr - rects.bx[i].x;
              ys=rects.bx[i].yb - rects.bx[i].y;
/*
printf("-----small fill x%d y%d xs%d ys%d\n", rects.bx[i].x,rects.bx[i].y,xs,ys);
*/
              if( (xs > 0) && (ys > 0) )
              {
                gdispFillArea(rects.bx[i].x,
                              rects.bx[i].y,
                              xs, ys,
                              iss->list[id].bgcolor);

                drew = TRUE;
              }
            }
          }
          if(FALSE == drew)
          {
            gdispFillArea(iss->list[id].bg_buf.x,
                          iss->list[id].bg_buf.y,
                          iss->list[id].bg_buf.xs,
                          iss->list[id].bg_buf.ys,
                          iss->list[id].bgcolor);
          }
        }

        break;
      case ISP_BG_DYNAMIC:
        if( NULL != iss->list[id].bg_buf.buf)
        {
          putPixelBlock(iss->list[id].bg_buf.x,
                        iss->list[id].bg_buf.y,
                        iss->list[id].bg_buf.xs,
                        iss->list[id].bg_buf.ys,
                        iss->list[id].bg_buf.buf);
/*
printf("dynamic fill x%d y%d xs%d ys%d\n", iss->list[id].bg_buf.x,iss->list[id].bg_buf.y,iss->list[id].bg_buf.xs, iss->list[id].bg_buf.ys);
*/
        }
        break;
    }
    /* if the sprite is invisible, flush the BG buuffers.  it's new location
     * will have nothing to do with the last saved BG. */
    iss->list[id].full_restore = FALSE;
    if(FALSE == iss->list[id].visible)
    {
      isp_destroy_ispbuf(&iss->list[id].bg_buf);
    }
  }

}




/* returns true if it was able to capture.  false if not */
static bool_t isp_capture_bg(ISPRITESYS *iss, ISPID id)
{
  bool_t ret = FALSE;
  int size;
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (iss->list[id].visible) )
  {
    if(ISP_BG_DYNAMIC == iss->list[id].bgtype)
    {
      size = iss->list[id].sp_buf.xs * iss->list[id].sp_buf.ys * sizeof(pixel_t);
      /* create or realloc buffer space as needed */
      if((NULL == iss->list[id].bg_buf.buf))
      {
//printf("cap bg alloc %d\n", size);
        iss->list[id].bg_buf.buf = (pixel_t *)dmalloc(size, "isp_capture_bg() dynamic BG buffer");
      }
      else if( (iss->list[id].bg_buf.xs != iss->list[id].sp_buf.xs) ||
               (iss->list[id].bg_buf.ys != iss->list[id].sp_buf.ys) )
      {
        dfree(iss->list[id].bg_buf.buf, "isp_capture_bg free old buffer");
        iss->list[id].bg_buf.buf = (pixel_t *)dmalloc(size, "isp_capture_bg new buffer");
      }
      /* */
      getPixelBlock(iss->list[id].sp_buf.x, iss->list[id].sp_buf.y,
                    iss->list[id].sp_buf.xs, iss->list[id].sp_buf.ys,
                    iss->list[id].bg_buf.buf);

    }
    iss->list[id].bg_buf.x  = iss->list[id].sp_buf.x;
    iss->list[id].bg_buf.y  = iss->list[id].sp_buf.y;
    iss->list[id].bg_buf.xs = iss->list[id].sp_buf.xs;
    iss->list[id].bg_buf.ys = iss->list[id].sp_buf.ys;

    ret = TRUE;
  }
  return(ret);
}

static void isp_draw_sprite_with_alpha(ISPRITESYS *iss, ISPID id)
{
  coord_t xo, yo, x,y, w,h, block_start, len, buffoffs;
  ISPBUF *s;
  bool_t solid;

  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (NULL != iss->list[id].sp_buf.buf)  &&
      (iss->list[id].sp_buf.xs > 0) &&
      (iss->list[id].sp_buf.ys > 0) )
  {
printf("in alpha draw\n");
fflush(stdout);
    s = &iss->list[id].sp_buf;
    w = s->xs;
    h = s->ys;

    /* xo,yo are the on-screen position of upper left corner of sprite */
    xo = s->x;
    yo = s->y;

    for(y=0; y < h; y++)
    {
      block_start = w;
      len =0;
      for(x=0; x < w; x++)
      {

        solid = wm_value(iss->list[id].alphamap, x,y,3);
printf("h:%d w:%d - x:%d y:%d s:%d\n",h,w, x,y,solid);
fflush(stdout);

        if(solid)
        {
          if(block_start == w)
          {
            block_start = x;
          }
          len++;
        }

        /* if it's a non-solid pixel, or the last pixel in the row, draw to screen */
        if( (!solid) || (x == (w-1)) )
        {

          buffoffs = ( y * s->ys) + x;
          putPixelBlock(x+xo, y+yo,
                        len, 1,
                        &s->buf[buffoffs]);
          block_start = w;
          len = 0;
        }

      }
    }
  }
}


/* Draw the sprite */
static void isp_draw_sprite(ISPRITESYS *iss, ISPID id)
{
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (iss->list[id].visible) &&
      (ISP_STAT_CLEAN != iss->list[id].status)  )
  {
    if( NULL != iss->list[id].sp_buf.buf)
    {
/*      if(ISP_BG_DYNAMIC == iss->list[id].bgtype)
*/
      if(0) /* bug here, don't call yet */
      {
printf("pre alpha draw\n");
fflush(stdout);
        isp_draw_sprite_with_alpha(iss, id);
      }
      else
      {
        putPixelBlock(iss->list[id].sp_buf.x,
                      iss->list[id].sp_buf.y,
                      iss->list[id].sp_buf.xs,
                      iss->list[id].sp_buf.ys,
                      iss->list[id].sp_buf.buf);
      }
    }
    iss->list[id].status = ISP_STAT_CLEAN;
  }
}




/* this is a way of storing pre-alpha sprites outside of the sprite system, for
 * quickly switching images.
 */
ISPHOLDER *isp_make_spholder(void)
{
  ISPHOLDER *ret=NULL;
  ret = dcalloc(1, sizeof(ISPHOLDER), "isp_make_spholder()");
  if(NULL != ret)
  {
    ret->sprite = isbuf_default;
    ret->alpha = NULL;
  }
  return(ret);
}

void isp_destroy_spholder(ISPHOLDER *sph)
{
  if( (NULL != sph)  )
  {
    isp_destroy_ispbuf(&sph->sprite);
    wm_destroy_wmap(sph->alpha);
    dfree(sph, "isp_destroy_spholder free ISPHOLDER");
  }
}

/* makes a proper ISPHOLDER from a passed in buffer.  crops image, makes alpha */
ISPHOLDER *isp_buf_to_spholder(coord_t xs, coord_t ys, pixel_t *buf)
{
  ISPHOLDER *ret=NULL;
  WMAP *wm;
  RECT r;
  if( (NULL != buf) && (xs > 0) && (ys > 0) )
  {
    ret = isp_make_spholder();
    if(NULL != ret)
    {
      ret->sprite.xs = xs;
      ret->sprite.ys = ys;

      /* make a bit map from the image.  use that to determine the bounding box
       * for cropping the sprite_tester
       */
      wm = wm_make_wmap_size(xs, ys);
      wm_scan_from_img(wm, 0, 0, xs, ys, buf);
      wm_set_first_last_per_row(wm);
      r = wm_make_bounding_box(wm);
      wm_destroy_wmap(wm);

      ret->sprite.buf = crop_buf(&ret->sprite.xs, &ret->sprite.ys,
                                 &ret->sprite.x,  &ret->sprite.y, r, buf);

      if(NULL == ret->sprite.buf)
      {
        ret->sprite.buf = buf;
      }
      else
      {
        dfree(buf, "isp_buf_to_spholder");
      }

      ret->alpha = wm_make_wmap_size(ret->sprite.xs, ret->sprite.ys);

      wm_scan_from_img(ret->alpha, 0, 0, ret->sprite.xs,
                       ret->sprite.ys, ret->sprite.buf);
      wm_set_first_last_per_row(ret->alpha);
    }
  }
  else
  {
    printf("ERROR: isp_buf_to_spholder xs%d ys%d buf %08x\n", xs, ys, buf);
  }

  return(ret);
}


bool_t isp_set_sprite_from_spholder(ISPRITESYS *iss, ISPID id, ISPHOLDER *sph)
{
  coord_t x,y;
  bool_t ret = false;

  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (NULL != sph) &&
      (sph->sprite.xs > 0) &&
      (sph->sprite.ys > 0) &&
      (NULL != sph->sprite.buf) )
  {
    /* reset x,y back to passed-in pre-offset value, because we don't know
    *  what the offset on the incoming one will be.*/
    x = iss->list[id].sp_buf.x - iss->list[id].xoffs;
    y = iss->list[id].sp_buf.y - iss->list[id].yoffs;

    isp_destroy_ispbuf(&iss->list[id].sp_buf);
    iss->list[id].xoffs = sph->sprite.x;
    iss->list[id].yoffs = sph->sprite.y;
    isp_copy_ipsbuf(&iss->list[id].sp_buf, &sph->sprite);
    isp_set_sprite_xy(iss, id, x, y);

    wm_destroy_wmap(iss->list[id].alphamap);
    iss->list[id].alphamap = wm_copy_wmap(sph->alpha);
    if(1 )//ISP_STAT_DIRTY_BOTH != iss->list[id].status) /* if BG is dirty, FG is dirty, so don't change status */
    {
      iss->list[id].status = ISP_STAT_DIRTY_SP;
    }

    ret = TRUE;
  }
  return(ret);
}


ISPHOLDER *isp_get_spholder_from_sprite(ISPRITESYS *iss, ISPID id)
{
  ISPHOLDER *ret=NULL;
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (iss->list[id].sp_buf.xs > 0) &&
      (iss->list[id].sp_buf.ys > 0))
  {
    ret = isp_make_spholder();
    if(NULL != ret)
    {
      isp_copy_ipsbuf(&ret->sprite, &iss->list[id].sp_buf);
      ret->sprite.x = iss->list[id].xoffs;
      ret->sprite.y = iss->list[id].yoffs;
      ret->alpha = wm_copy_wmap(iss->list[id].alphamap);
    }
  }
  return(ret);
}



/* load images from fileaname into SPHOLDER */
ISPHOLDER *isp_get_spholder_from_file(char *name)
{
  FILE *f;
  uint16_t h;
  uint16_t w;
  GDISP_IMAGE hdr;
  size_t len;
  ISPHOLDER *ret;
  pixel_t *buf;


  if (! (f = fopen(name, "r") ) )
  {
  	return (NULL);
  }

  fread(&hdr, 1, sizeof(GDISP_IMAGE), f);
  h = hdr.gdi_height_hi << 8 | hdr.gdi_height_lo;
  w = hdr.gdi_width_hi << 8 | hdr.gdi_width_lo;

  len = h * w * sizeof(pixel_t);
//printf("sp file alloc %d\n", len);

  buf = dmalloc(len,"isp_get_spholder_from_file()");
  fread(buf, 1, len, f);
  fclose(f);
  ret = isp_buf_to_spholder(w, h, buf);
  dfree(buf, "isp_get_spholder_from_file");
  return(ret);
}


/* this just initialises the sprite system, does not create any sprites.
 *
 */
ISPRITESYS *isp_init(void)
{
  ISPID i;
  ISPRITESYS *iss=NULL;
  iss = (ISPRITESYS *)dmalloc(sizeof(ISPRITESYS), "isp_init()");
  if(NULL != iss)
  {
    iss->wm = NULL;
    for(i=0; i < ISP_MAX_SPRITES; i++)
    {
      iss->list[i] = isprite_default;
      iss->list[i].bg_buf = isbuf_default;
      iss->list[i].sp_buf = isbuf_default;
    }
  }
  return(iss);
}

void isp_scan_screen_for_water(ISPRITESYS *iss)
{
  iss->wm = wm_build_land_map_from_screen();
}

static bool_t isp_is_live(ISPRITESYS *iss, ISPID id)
{
  return( (id < ISP_MAX_SPRITES) && (iss->list[id].active) );
}

bool_t isp_is_sprite_visible(ISPRITESYS *iss, ISPID id)
{
  return( (id < ISP_MAX_SPRITES) &&
           (iss->list[id].active) &&
           (iss->list[id].visible) );
}



/* overlap is TRUE if they have to overlap, not just touch to be considered a colision */
/* if drawtest is TRUE, it tests against both sprite and BG for collsions.  this is because
 * a spite that previously occupied the same space as anopther sprite in the last frame
 * will trigger a background restoration in it's previous location, which will end up drawBufferedStringBox
 * on top of the other sptite - so BOTH have to be flagged dirty.
 */
bool_t isp_check_sprites_collision(ISPRITESYS *iss, ISPID id1, ISPID id2, bool_t overlap, bool_t drawtest)
{
  RECT a, b;
  bool_t ret=FALSE;


  /* only check if they are both active and sprite data */
  if( (iss->list[id1].active) &&
      (iss->list[id2].active) &&
      (iss->list[id1].sp_buf.xs > 0) &&
      (iss->list[id2].sp_buf.xs > 0) )
  {
    /* for overlap tests, boxes have to overlap to trigger a collision.
     * we have to subrtact 1 because if pos=0, and size=4, it takes up 4 pixels
     * starting at 0: 0,1,2,3.  0+4 == 4 which is not accurate for size of box.
     * consider 2 adjacent 4-pixel boxes occupying the following pixels:
     * [0123][4567]
     * 4 is the L side of box2, and 3 is the R side of box1.
     * they are outide of each other, no overlap.
     *
     * if overlap is false, boxes can be adjacent or overlapping to trigger a collsion.
     * in this case, we want the boxes to be 1 larger
     * for this mode, Those same boxes are now:
     * [01234][45678]
     * L2 and R1 both == 4, which triggers collision.
     */
    if(drawtest)
    {
      a = make_bounding_rect(iss->list[id1].sp_buf, iss->list[id1].bg_buf);
      b = make_bounding_rect(iss->list[id2].sp_buf, iss->list[id2].bg_buf);
    }
    else
    {
      a = make_rect(iss->list[id1].sp_buf);
      b = make_rect(iss->list[id2].sp_buf);
    }
    if(overlap)
    {
      a.xr -= 1;
      a.yb -= 1;
      b.xr -= 1;
      b.yb -= 1;

    }

   ret = do_rects_clip(a,b);
  }
  return(ret);
}



/* if one dirty sprite has any coordinate overlap with another sprite, they both must become dirty
 * if visible_only is true, it will only compare against visible sprites.
 * if it's false, it will compare against visible and invisible sprites.
 */
static void isp_flag_dirty_sprite_collisions(ISPRITESYS *iss, bool_t visible_only)
{
  int i,j,ii,jj, max=0;
  ISPID sl[ISP_MAX_SPRITES];

  /* first make the list of the ones we want to compare against */
  for(i=0; i < ISP_MAX_SPRITES; i++)
  {
    if( (iss->list[i].active) &&
        (!visible_only || iss->list[i].visible) )
    {
      sl[max]=i;
      max++;
    }
  }

  for(i=0; i < max; i++)
  {
    ii = sl[i];
    for(j=ii+1; j < max; j++)
    {
      jj = sl[j];
      /* if BG isn't dirty, check for overlap with other sprites and flag both as dirty_BG if they overlap
       * we don't know which one will move first, so BG needs to be restored for both so we get full, fresh BG collection
       * only check for collisions of at least one of them is dynamic BG.
       * if boith are solid color BG, they can collide all they want, and it won't affect BG redraw
       */
      if( ((ISP_STAT_DIRTY_BOTH != iss->list[ii].status) || (ISP_STAT_DIRTY_BOTH != iss->list[jj].status)) &&
          ( (ISP_BG_DYNAMIC == iss->list[ii].bgtype) || (ISP_BG_DYNAMIC == iss->list[jj].bgtype) ) &&
          isp_check_sprites_collision(iss, ii, jj, TRUE, TRUE) )
      {
        iss->list[ii].status = ISP_STAT_DIRTY_BOTH;
        iss->list[jj].status = ISP_STAT_DIRTY_BOTH;
      }
    }
  }
}





/* creates a spite, does not draw anything */
/* returns the ID number for the sprite, or ISP_MAX_SPRITES on fail */
ISPID isp_make_sprite(ISPRITESYS *iss)
{
  ISPID i;
  for(i=0; i < ISP_MAX_SPRITES; i++)
  {
    if(!isp_is_live(iss, i))
    {
      iss->list[i].visible = TRUE;
      iss->list[i].active = TRUE;
      iss->list[i].status = ISP_STAT_DIRTY_BOTH;
      break;
    }
  }
  return(i);
}




void isp_hide_sprite(ISPRITESYS *iss, ISPID id)
{
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (TRUE == iss->list[id].visible) )
  {
    iss->list[id].full_restore = TRUE;
    iss->list[id].status = ISP_STAT_DIRTY_BOTH; /* flag it so BG gets restored */
    iss->list[id].visible = FALSE;         /* flag it so it doesn't draw */
  }
}

void isp_show_sprite(ISPRITESYS *iss, ISPID id)
{
  if( (id < ISP_MAX_SPRITES) &&
      (iss->list[id].active) &&
      (FALSE == iss->list[id].visible) )
  {
    /* make it so the BG is NOT restored on next draw pass. the BG buffer is
     * outdated when a sprite goes invisible */
    iss->list[id].status = ISP_STAT_DIRTY_BOTH; /* flag it so BG gets captured */
    iss->list[id].visible = TRUE;         /* flag it so it does draw */
  }
}



/* this will immediately repaint the background */
void isp_destroy_sprite(ISPRITESYS *iss, ISPID id)
{
  if( (NULL != iss ) && (id < ISP_MAX_SPRITES) )
  {
    isp_hide_sprite(iss, id);
    isp_restore_bg(iss, id);
    isp_destroy_ispbuf(&iss->list[id].bg_buf);
    isp_destroy_ispbuf(&iss->list[id].sp_buf);
  /*    if( NULL != iss->list[id].sp_buf.buf)
    {
      free(iss->list[id].sp_buf.buf);
    }
*/
    wm_destroy_wmap(iss->list[id].alphamap);

    iss->list[id] = isprite_default;
    iss->list[id].bg_buf = isbuf_default;
    iss->list[id].bg_buf = isbuf_default;

  }
}



/* just set the position, don't flag it.  all that's handled later.*/
void isp_set_sprite_xy(ISPRITESYS *iss, ISPID id, coord_t x, coord_t y)
{
  if(id < ISP_MAX_SPRITES)
  {
    iss->list[id].sp_buf.x = x + iss->list[id].xoffs;
    iss->list[id].sp_buf.y = y + iss->list[id].yoffs;
  }
}

void wm_print_map(WMAP *w)
{
  coord_t x,y;
  char line[84];

  for(y=0; y < w->h; y++)
  {
    for(x=0; x < w->w; x++)
    {
      line[x] = '0' + wm_value(w, x, y, 3);
      line[x+1] = '\0';
    }
    printf("x%03d y%03d %s\n", x, y, line );
  }
}

/* if copy_buf == TRUE this copies buf into it's own allocated memory.
 * if FALSE, it treats buf as a pointer, and does not save locally.  if buf is
 * changed, just call this again to flag it for re-draw, otherwise it assumes
 * the sprite is unchanged.
 *
 *
 */
void isp_set_sprite_block(ISPRITESYS *iss, ISPID id, coord_t xs, coord_t ys, pixel_t * buf)
{
  ISPHOLDER *hold;

  hold = isp_buf_to_spholder(xs, ys, buf);
  isp_set_sprite_from_spholder(iss, id, hold);
  isp_destroy_spholder(hold);
}




int isp_load_image_from_file(ISPRITESYS *iss, ISPID id, char *name)
{
  FILE *f;
  uint16_t h;
  uint16_t w;
  GDISP_IMAGE hdr;
  pixel_t *buf;
  size_t len;

  if(id < ISP_MAX_SPRITES)
  {
    if (! (f = fopen(name, "r") ) )
    {
    	return (1);
    }

    fread(&hdr, 1, sizeof(GDISP_IMAGE), f);
    h = hdr.gdi_height_hi << 8 | hdr.gdi_height_lo;
    w = hdr.gdi_width_hi << 8 | hdr.gdi_width_lo;

    len = h * w * sizeof(pixel_t);
//printf("sp file alloc %d\n", len);
    buf = dmalloc(len, "isp_load_image_from_file()");

    fread(buf, 1, len, f);
    fclose(f);

    isp_set_sprite_block(iss, id, w, h, buf);
    free (buf);
  }
  return(0);
}


/* feed this a color_t value.  like from HTML2COLOR(h) or RGB2COLOR(r,g,b);
 *  it will fill the sprite's former position with solid block of that color.
 */
void isp_set_sprite_bgcolor(ISPRITESYS *iss, ISPID id, color_t col)
{
  if(id < ISP_MAX_SPRITES)
  {
    iss->list[id].bgcolor = col;
    iss->list[id].bgtype = ISP_BG_FIXEDCOLOR;
    /* maybe free BG buffer if it exists? */
  }
}

/* feed this a color_t value.  like from HTML2COLOR(h) or RGB2COLOR(r,g,b);
 *  it will fill the sprite's former position with solid block of that color.
 */
void isp_set_sprite_wmap_bgcolor(ISPRITESYS *iss, ISPID id, color_t col)
{
  if(id < ISP_MAX_SPRITES)
  {
    iss->list[id].bgcolor = col;
    iss->list[id].auto_bg = TRUE;
  }
}

/* releases the background color mode, and switches sprite to dynamic background mode
 */

void isp_release_sprite_bgcolor(ISPRITESYS *iss, ISPID id)
{
  if(id < ISP_MAX_SPRITES)
  {
    iss->list[id].bgcolor = isprite_default.bgcolor;
    iss->list[id].bgtype = ISP_BG_DYNAMIC;
  }
}


bool_t test_sprite_for_land_collision(ISPRITESYS *iss, ISPID id)
{
  RECT r;
  ISPBUF *buf;
  bool_t ret=FALSE;
  if( (id < ISP_MAX_SPRITES) && (iss->list[id].active) )
  {
    buf = &iss->list[id].sp_buf;
    if( (NULL != iss->wm) &&
        (buf->xs >0) &&
        (buf->ys >0) )
    {
      r.x = buf->x;
      r.y = buf->y;
      r.xr = r.x + buf->xs;
      r.yb = r.y + buf->ys;

      ret = wm_check_box_for_land_collision(iss->wm, r);
    }
  }
  return(ret);
}

void  isp_draw_all_sprites(ISPRITESYS *iss)
{
  ISPID i, id, max, slist[ISP_MAX_SPRITES];
  /* make list of active only.  This way we don't have to perform that check constantly */
  max=0;
  for(i=0; i< ISP_MAX_SPRITES; i++)
  {
    if(iss->list[i].active)
    {
      slist[max] = i;
      max++;
    }
  }
  /* Check if position and size of sprite and BG are the same, don't copy BG again. */
  for(i=0; i< max; i++)
  {
    id = slist[i];
    /* if the stored BG is not same size and position as sprite, flag as dirty */
    if( (iss->list[id].visible) &&
        ((iss->list[id].bg_buf.x != iss->list[id].sp_buf.x) ||
         (iss->list[id].bg_buf.y != iss->list[id].sp_buf.y) ||
         (iss->list[id].bg_buf.xs != iss->list[id].sp_buf.xs) ||
         (iss->list[id].bg_buf.ys != iss->list[id].sp_buf.ys)) )
    {
      iss->list[id].status = ISP_STAT_DIRTY_BOTH;
    }
  }

  /* check for box collisions here.  if there is any overlap, both have dirty BG
   *  can't have one buffer a BG that has sprite data on it */

  isp_flag_dirty_sprite_collisions(iss, false);

  if(NULL != iss->wm)
  {
    for(i=0; i< max; i++)
    {
      id = slist[i];
      if( (iss->list[id].visible) &&
          (iss->list[id].auto_bg) )
      {
        if(test_sprite_for_land_collision(iss, id))
        {

          iss->list[id].status = ISP_STAT_DIRTY_BOTH;
          if(ISP_BG_FIXEDCOLOR == iss->list[id].bgtype)
          {
            iss->list[id].mode_switch = TRUE;
            iss->list[id].full_restore = TRUE;
          }
        }
        else
        {
          if(ISP_BG_DYNAMIC == iss->list[id].bgtype)
          {
          //  iss->list[id].full_restore = TRUE;
            iss->list[id].mode_switch = TRUE;
          }
        }
      }
    }
  }

  /* first restore all dirty BG blocks*/
  for(i=0; i< max; i++)
  {
    id = slist[i];
    if(ISP_STAT_DIRTY_BOTH == iss->list[id].status)
    {
      isp_restore_bg(iss, id);
    }
  }

  /* Now that BG is restored, check if we have a WMAP, and then test all
       * auto_bg sprites against it and set their BG modes accordingly.
  */
  if(NULL != iss->wm)
  {
    for(i=0; i< max; i++)
    {
      id = slist[i];
      if(iss->list[id].auto_bg && iss->list[id].mode_switch)
      {
        iss->list[id].mode_switch = FALSE;
        if(ISP_BG_FIXEDCOLOR == iss->list[id].bgtype)
        {
          iss->list[id].bgtype = ISP_BG_DYNAMIC;
          //iss->list[id].status = ISP_STAT_DIRTY_BOTH;
        }
        else
        {
          iss->list[id].bgtype = ISP_BG_FIXEDCOLOR;
        }
      }
    }
  }

  /* now collect BG block from new sprite postitions */
  for(i=0; i< max; i++)
  {
    id = slist[i];

  /* only save BG if sprite is visibe AND either status is Dirty, or we have no BG */
    if( (iss->list[id].visible) &&
        ((ISP_STAT_DIRTY_BOTH == iss->list[id].status ) || (NULL == iss->list[id].bg_buf.buf))  )
    {
      isp_capture_bg(iss, id);
      iss->list[id].status = ISP_STAT_DIRTY_SP;
    }

  }
  /* now paint all dirty sprites */
  for(i=0; i< max; i++)
  {
    id = slist[i];
    /* this handles visibility checks and changes flag status.  just let it  figure it out. */
    isp_draw_sprite(iss, id);
  }
}



void isp_shutdown(ISPRITESYS *iss)
{
  ISPID i;
  if(NULL != iss)
  {
    if(NULL != iss->wm)
    {
      wm_destroy_wmap(iss->wm);
      dfree(iss->wm, "isp_shutdown wm");
      iss->wm = NULL;
    }
    for(i=0; i < ISP_MAX_SPRITES; i++)
    {
      isp_destroy_sprite(iss, i);
    }
  }
  dfree(iss, "isp_shutdown iss");
}


pixel_t *boxmaker(coord_t x, coord_t y, color_t col)
{

  int i, max;
  pixel_t *buf;

  x = fix_range(x, 1,100);
  y = fix_range(y, 1,100);

  max = x * y;
  buf = dmalloc(max*sizeof(pixel_t), "boxmaker()");
  for(i=0; i < max; i++)
  {
    buf[i] = col;
  }
  return(buf);
}

void sprite_tester(void)
{
  static ISPRITESYS *sl=NULL;
  ISPHOLDER *hold;
  static double i = 0.0;
  static int col=0, jj=0;;
  static ISPID s1, s2, s3, s4, s5, s6, s7, s8,s9;
  pixel_t *buf;
  coord_t xs,ys;
  char str[90];

  if(NULL == sl)
  {
printf("init 2nd engine\n");
    sl = isp_init();
/*    s1 = isp_make_sprite(sl); // make sprites.  none are drawn yet.
    s2 = isp_make_sprite(sl); // they have no position or pixel data yet.
    s3 = isp_make_sprite(sl);
    s4 = isp_make_sprite(sl); // make sprites.  none are drawn yet.
    s5 = isp_make_sprite(sl); // they have no position or pixel data yet.
    s6 = isp_make_sprite(sl);
    s7 = isp_make_sprite(sl); // make sprites.  none are drawn yet.
    s8 = isp_make_sprite(sl); // they have no position or pixel data yet.
    */

    s9 = isp_make_sprite(sl);
/*
    xs=8;
    ys=15;
    buf = boxmaker(xs,ys, HTML2COLOR(0xff00ff));
    isp_set_sprite_block(sl, s1, xs, ys, buf);
    free(buf);
    xs=10;
    ys=11;
    buf = boxmaker(xs,ys, HTML2COLOR(0x00FFff));
    isp_set_sprite_block(sl, s2, xs, ys, buf);
    free(buf);

    xs=15;
    ys=9;
    buf = boxmaker(xs,ys, HTML2COLOR(0x4488ff));
    isp_set_sprite_block(sl, s3, xs, ys, buf);
    free(buf);

    xs=5;
    ys=5;
    buf = boxmaker(xs,ys, HTML2COLOR(0xff0000));
    isp_set_sprite_block(sl, s4, xs, ys, buf);
    free(buf);

    xs=20;
    ys=20;
    buf = boxmaker(xs,ys, HTML2COLOR(0x00ff00));
    isp_set_sprite_block(sl, s5, xs, ys, buf);
    free(buf);

    xs=35;
    ys=3;
    buf = boxmaker(xs,ys, HTML2COLOR(0x0000ff));
    isp_set_sprite_block(sl, s6, xs, ys, buf);
    free(buf);

    xs=15;
    ys=13;
    buf = boxmaker(xs,ys, HTML2COLOR(0x000000));
    isp_set_sprite_block(sl, s7, xs, ys, buf);
    free(buf);

    xs=5;
    ys=23;
    buf = boxmaker(xs,ys, HTML2COLOR(0xffffff));
    isp_set_sprite_block(sl, s8, xs, ys, buf);
    free(buf);


    xs=12;
    ys=12;
    buf = boxmaker(xs,ys, HTML2COLOR(0xffff00));
    isp_set_sprite_block(sl, s9, xs, ys, buf);
    free(buf);
*/
/* switch to this instead of the block of code below it to have all sprites moving
  isp_set_sprite_xy(sl, s1, 160 + (sin(i) * 25), 120 + (cos(i) * 25) );
  isp_set_sprite_xy(sl, s2, 160 + (sin(i+2) * 35), 120 + (cos(i+2) * 35) );
  isp_set_sprite_xy(sl, s3, 160 + (sin(i+4) * 55), 120 + (cos(i+4) * 55) );
  isp_set_sprite_xy(sl, s4, 160 + (sin(-i) * 25), 120 + (cos(-i) * 25) );
  isp_set_sprite_xy(sl, s5, 160 + (sin(-i+2) * 45), 120 + (cos(-i+2) * 35) );
  isp_set_sprite_xy(sl, s6, 160 + (sin(-i+4) * 65), 120 + (cos(-i+4) * 65) );
  isp_set_sprite_xy(sl, s7, 160 + (sin(-1.2*i+1) * 25), 120 + (cos(-i) * 15) );
  isp_set_sprite_xy(sl, s8, 160 + (sin(1.1*i+2.5) * 5), 120 + (cos(-i+2) * 35) );
  isp_set_sprite_xy(sl, s9, 160 + (sin(1.3*-i+4.5) * 75), 120 + (cos(-i+4) * 60) );
*/

/*  isp_set_sprite_xy(sl, s1, 160 + (sin(2) * 25), 120 + (cos(2) * 25) );
  isp_set_sprite_xy(sl, s2, 160 + (sin(2+2) * 35), 120 + (cos(2+2) * 35) );
  isp_set_sprite_xy(sl, s3, 160 + (sin(2+4) * 55), 120 + (cos(2+4) * 55) );
  isp_set_sprite_xy(sl, s4, 160 + (sin(-2) * 25), 120 + (cos(-2) * 25) );
  isp_set_sprite_xy(sl, s5, 160 + (sin(-3+2) * 45), 120 + (cos(-3+2) * 35) );
  isp_set_sprite_xy(sl, s6, 160 + (sin(-3+4) * 65), 120 + (cos(-3+4) * 65) );
  isp_set_sprite_xy(sl, s7, 160 + (sin(-1.2*4+1) * 45), 120 + (cos(-1.2) * 45) );
  isp_set_sprite_xy(sl, s8, 160 + (sin(-4+4.3) * 95), 120 + (cos(-5+3.8) * 90) );
  */
  isp_set_sprite_bgcolor(sl, s9, HTML2COLOR(0x0000ff));



xs =20;
ys= 20;
sprintf(str, " inner boxmaker in tester x%d y%d ", xs, ys);
buf = boxmaker(xs,ys, HTML2COLOR(col));
isp_set_sprite_block(sl, s9, xs, ys, buf);
dfree(buf, "boxmaker in tester cleanup");
}
isp_set_sprite_xy(sl, s9, 160 + (sin(-i+4.5) * 75), 120 + (cos(-i+4) * 60) );


xs=(int)15+(cos(i*33)*9);
ys=(int)15+(sin(i*33)*9);


if(jj > 1)
{
  sprintf(str, "boxmaker in tester x%d y%d ", xs, ys);
  buf = boxmaker(xs,ys, HTML2COLOR(col));
  isp_set_sprite_block(sl, s9, xs, ys, buf);


  dfree(buf, "boxmaker in tester cleanup");
  jj=0;
}
jj++;
col += 0x040404;
if(col > 0xffffff)
{
  col=0;
}

  i += 0.05;

  isp_draw_all_sprites(sl);
}




void sprite_tester_mapaware(void)
{
  static ISPRITESYS *sl=NULL;
  static ISPHOLDER *l,*r;
  static double i = 0.0;
  static ISPID s1, s2;
  static bool_t overland=TRUE;
  static int j=0;
  coord_t x,y;

  if(NULL == sl)
  {
    sl = isp_init();
    s1 = isp_make_sprite(sl); // make sprites.  none are drawn yet.
    s2 = isp_make_sprite(sl);

 fx_init(sl);

    isp_scan_screen_for_water(sl);


//wmap_header_dump(sl->wm, "main");

    isp_set_sprite_wmap_bgcolor(sl, s1, HTML2COLOR(0x0000ff));
    isp_set_sprite_wmap_bgcolor(sl, s2, HTML2COLOR(0x0000ff));

    isp_load_image_from_file(sl, s1, "game/s0p-n-r.rgb");
    isp_load_image_from_file(sl, s2, "game/s0p-n-l.rgb");

//    isp_hide_sprite(sl, s1);
    isp_hide_sprite(sl, s2);
    l = isp_get_spholder_from_sprite(sl, s2);
    r = isp_get_spholder_from_sprite(sl, s1);

  /*  isp_hide_sprite(sl, s2); */
/*    isp_set_sprite_block(sl, s1, xs, ys, land);
*/
  }
  x=160 + (sin(-i+4.5) * 75);
  y=120 + (cos(-i+4) * 60);


  isp_set_sprite_xy(sl, s1, x, y );
j++;
if(j > 4)
{
  j=0;
  fx_make_sizer_box(x, y, 3, 15, HTML2COLOR(0xFF0000), HTML2COLOR(0x00ff00), 30, 1500);

}
fx_update();
//  isp_set_sprite_xy(sl, s2, x, y );
  if(test_sprite_for_land_collision(sl, s1))
  {
    if(!overland)
    {
/*      isp_set_sprite_block(sl, s1, xs, ys, land); */
isp_set_sprite_from_spholder(sl, s1, l);

//      isp_hide_sprite(sl, s1);
//      isp_show_sprite(sl, s2);
      overland = TRUE;
    }
  }
  else
  {
    if(overland)
    {

      isp_set_sprite_from_spholder(sl, s1, r);

//      isp_hide_sprite(sl, s2);
  //    isp_show_sprite(sl, s1);

    /*  isp_set_sprite_block(sl, s1, xs, ys, water); */
      overland = FALSE;
    }
  }
/*  isp_set_sprite_block(sl, s1, xs, ys, buf);
  free(buf);
*/



  i += 0.15;

  isp_draw_all_sprites(sl);

}
