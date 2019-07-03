#ifndef _IDES_SPRITE_
#define _IDES_SPRITE_

#define ISP_MAX_SPRITES 32
#define ISP_PATH_LENGTH 64

/* 0 water, 1 land */
#define WMAP_W SCREEN_W / 32
#define WMAP_H SCREEN_H
#define BLUE_THRESH 200
#define NOTBLUE_THRESH 30


#define GET_PX_GREEN( p ) ( ( ((p & 7) << 3) | ((p & 57344) >> 13) )* 4)
#define GET_PX_BLUE( p ) ( ( (p & 7963) >> 8) * 8)
#define GET_PX_RED( p ) ( ( (p & 248) >> 3 ) * 8)


/* WM_STEP defines which array index a particular coord is in. */
#define WM_BIT(c)  ( (uint32_t)( (c) % 32) )
#define WM_STEP(c) ( (uint32_t)( (c) / 32) )


typedef struct _w_row {
  uint32_t row[WMAP_W];
} WROW;

typedef struct _w_map {
  coord_t h;
  coord_t w; /* might switch to adjustable size. */
   WROW map[WMAP_H];
} WMAP;

enum ides_sprite_status_t {
  ISP_STAT_DIRTY_BOTH, /* position or contents of sprite have changed, need to repaint BG and sprite */
  ISP_STAT_DIRTY_SP,   /* only sprite has changed, same size and position as BG.  BG doesn't need repainting */
  ISP_STAT_CLEAN      /* nothing changed since last repaint */
} ;

enum ides_sprite_draw_type{
  ISP_DRAW_BLOCK, /* dump block in here, data is copied, stored, and managed internally */
  ISP_DRAW_PTR,   /* image data is merely a pointer, can be updated externally, but need to flag it dirty for redraw */
  ISP_DRAW_GIF   /* uses GIF library for alpha .*/
} ;

enum ides_sprite_bg_type {
  ISP_BG_DYNAMIC,
  ISP_BG_FIXEDCOLOR,
} ;

typedef uint8_t ISPID;

typedef struct _ides_sprite_buffer {
  pixel_t *buf;
  coord_t x;
  coord_t y;
  coord_t xs;
  coord_t ys;
} ISPBUF;

typedef struct _ides_sprite {
  ISPBUF bg_buf; /* background buffer */
  ISPBUF sp_buf; /* sprite buffer */
  enum ides_sprite_draw_type type;
  enum ides_sprite_bg_type bgtype;
  enum ides_sprite_status_t status;
  bool_t active;
  bool_t visible;
  bool_t auto_bg;
  color_t bgcolor;
  char fname[ISP_PATH_LENGTH];
} ISPRITE;

typedef struct _rectangle {
  coord_t x;  /* X left */
  coord_t y;  /* Y top */
  coord_t xr; /* X right  */
  coord_t yb; /* Y bottom */
} RECT;


typedef struct _ides_sprite_list{
  ISPRITE list[ISP_MAX_SPRITES];
  WMAP *wm;
} ISPRITESYS;


extern ISPRITESYS *isp_init(void);
extern void isp_scan_screen_for_water(ISPRITESYS *iss);
extern ISPID isp_make_sprite(ISPRITESYS *iss);
extern void isp_hide_sprite(ISPRITESYS *iss, ISPID id);
extern void isp_destroy_sprite(ISPRITESYS *iss, ISPID id);
extern void isp_set_sprite_xy(ISPRITESYS *iss, ISPID id, coord_t x, coord_t y);
extern void isp_set_sprite_block(ISPRITESYS *iss, ISPID id, coord_t xs, coord_t ys, pixel_t * buf, bool_t copy_buf);
extern void isp_set_sprite_bgcolor(ISPRITESYS *iss, ISPID id, color_t col);
extern void isp_set_sprite_wmap_bgcolor(ISPRITESYS *iss, ISPID id, color_t col);
extern void isp_release_sprite_bgcolor(ISPRITESYS *iss, ISPID id);
extern bool_t isp_check_sprite_collision(ISPRITESYS *iss, ISPID id1, ISPID id2, bool_t overlap);
extern void isp_draw_all_sprites(ISPRITESYS *iss);
extern void isp_shutdown(ISPRITESYS *iss);
extern void sprite_tester(void);
extern bool_t is_valid_rect(RECT r);
extern RECT make_rect(ISPBUF ib);
extern WMAP *wm_make_wmap(void);
extern void sprite_tester_mapaware(void);
extern bool_t wm_value(WMAP *w, coord_t x, coord_t y, uint8_t v);
extern WMAP *wm_build_land_map_from_screen(void);
extern bool_t wm_check_box_for_land_collision(WMAP *map, RECT r);
extern pixel_t *boxmaker(coord_t x, coord_t y, color_t col);
#endif
