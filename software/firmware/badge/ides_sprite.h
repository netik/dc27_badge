#ifndef _IDES_SPRITE_
#define _IDES_SPRITE_

#define ISP_MAX_SPRITES 32
#define ISP_PATH_LENGTH 64

/* 0 water, 1 land */
#define WMAP_W SCREEN_W / 32
#define WMAP_H SCREEN_H
#define BLUE_THRESH 190
#define NOTBLUE_THRESH 44

/* jna: some notes here after a call with Devon...  */

/* Pixels are stored on the screen in RGB565 format, but they are stored as
 * big endian with the high byte transferred to us first. We have to flip
 * this to recover the green pixel, which is split in half.
 *
 * The "standard 565" format is:
 *  MSB                                  LSB
 *  15 14 13 12 11  10 9 8 | 7 6 5  4 3 2 1 0
 *   R  R  R  R  R   G G G | G G G  B B B B B
 *
 * Unfortunately what we have from the screen is
 *  byte1 (LSB)              byte 2 (MSB)
 *  15 14 13 12 11  10 9 8 | 7 6 5  4 3 2 1 0
 *   G  G  G  B  B   B B B | R R R  R R G G G
 *
 *  Where a solid green pixel would be represented as 1110 0000 0000 0111
 *
 *  We multiply the green value by 4 to scale it back to 255 color levels.
 *  and the blue and red values by 8 to scale them back to 255 levels as well.
 */

#define GET_PX_GREEN( p ) ( ( ((p & 0x07) << 3) | ((p & 0xe000) >> 10)) * 4 )
#define GET_PX_BLUE( p ) ( ( (p & 0x1f00) >> 8) * 8 )
#define GET_PX_RED( p ) ( ( (p & 0xf8) >> 3 ) * 8 )

/* WM_STEP defines which array index a particular coord is in. */
#define WM_BIT(c)  ( (uint32_t)( (c) % 32) )
#define WM_STEP(c) ( (uint32_t)( (c) / 32) )


typedef struct _w_row {
  uint32_t row[WMAP_W];
  coord_t first_px; /* shortcut, to first solid pixel */
  coord_t last_px;
  bool_t notblank; /* shortcut for telling if a row is blank */
} WROW;

typedef struct _w_map {
  coord_t h; /* these are the number of total bits (coords) in the map */
  coord_t w; /* # of bits used in Row, not number of indexes. */
  WROW *map;
} WMAP;

enum ides_sprite_status_t {
  ISP_STAT_DIRTY_BOTH, /* position or contents of sprite have changed, need to repaint BG and sprite */
  ISP_STAT_DIRTY_SP,   /* only sprite has changed, same size and position as BG.  BG doesn't need repainting */
  ISP_STAT_CLEAN      /* nothing changed since last repaint */
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
  WMAP *alphamap;
  coord_t xoffs;
  coord_t yoffs;
  enum ides_sprite_bg_type bgtype;
  enum ides_sprite_status_t status;
  bool_t active;
  bool_t visible;
  bool_t auto_bg;
  bool_t mode_switch;
  bool_t full_restore; /* flag to fully restore the BG. */
  color_t bgcolor;
} ISPRITE;

typedef struct _ides_sprite_holder {
  ISPBUF sprite; /* uses x,y for offset  since sprite might be cropped */
  WMAP *alpha;
} ISPHOLDER;

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
extern void isp_show_sprite(ISPRITESYS *iss, ISPID id);
extern void isp_destroy_sprite(ISPRITESYS *iss, ISPID id);
extern void isp_set_sprite_xy(ISPRITESYS *iss, ISPID id, coord_t x, coord_t y);
extern void isp_set_sprite_block(ISPRITESYS *iss, ISPID id, coord_t xs, coord_t ys, pixel_t * buf);
extern void isp_set_sprite_bgcolor(ISPRITESYS *iss, ISPID id, color_t col);
extern void isp_set_sprite_wmap_bgcolor(ISPRITESYS *iss, ISPID id, color_t col);
extern void isp_release_sprite_bgcolor(ISPRITESYS *iss, ISPID id);
extern bool_t isp_check_sprite_collision(ISPRITESYS *iss, ISPID id1, ISPID id2, bool_t overlap);
extern bool_t test_sprite_for_land_collision(ISPRITESYS *iss, ISPID id);
extern void isp_draw_all_sprites(ISPRITESYS *iss);
extern void isp_shutdown(ISPRITESYS *iss);
extern void sprite_tester(void);
extern bool_t is_valid_rect(RECT r);
extern RECT make_rect(ISPBUF ib);
extern WMAP *wm_make_wmap_size(coord_t width, coord_t height);
extern WMAP *wm_make_screensize_wmap(void);
extern void sprite_tester_mapaware(void);
extern bool_t wm_value(WMAP *w, coord_t x, coord_t y, uint8_t v);
extern WMAP *wm_build_land_map_from_screen(void);
extern bool_t wm_check_box_for_land_collision(WMAP *map, RECT r);
extern pixel_t *boxmaker(coord_t x, coord_t y, color_t col);
extern int isp_load_image_from_file(ISPRITESYS *iss, ISPID id, char *name);
extern bool_t isp_set_sprite_from_spholder(ISPRITESYS *iss, ISPID id, ISPHOLDER *sph);
extern ISPHOLDER *isp_get_spholder_from_sprite(ISPRITESYS *iss, ISPID id);
extern ISPHOLDER *isp_get_spholder_from_file(char *name);
extern void isp_destroy_spholder(ISPHOLDER *sph);
#endif
