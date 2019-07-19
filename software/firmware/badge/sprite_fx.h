#ifndef _SPRITE_FX_
#define _SPRITE_FX_


enum fx_type {
  FX_NONE,
  FX_SIZER,
  FX_BLINKER,
  FX_TRACKER
} ;

typedef struct _fx_object {
  coord_t x;
  coord_t y;
  enum fx_type type;
  int p1a; /* paramater 1a (start) */
  int p1b; /* parameter 1b (finish) */
  int p1z; /* value used on last poss */
  int p2a;
  int p2b;
  int p2z;
  int delay; /* this actually gets decremented until it's <= 0, then becomes active.*/
  int lifespan; /* lifespan is the time it's been active after the delay. */
  int age; /* age is time since it was initialized.  it's age will */
  int has_drawn;
  systime_t birth_time;
  ISPID master_sprite; /* for having the FX sprite follow an esiting sprite */
  ISPID sprite;
} FX_OBJ;

typedef struct _fx_list {
  FX_OBJ **list;
  int count;
} FXLIST;




extern void fx_init(ISPRITESYS *i);
extern void fx_shutdown(ISPRITESYS *i);
extern ISPID fx_make_sizer_box(coord_t x, coord_t y, coord_t sz_s, coord_t sz_e, color_t c_s, color_t c_e, int delay, int lifespan);
extern void fx_update(void);

#endif
