V2600SRC+=					\
	$(V2200)/vmachine.c			\
	$(V2600)/cpu.c				\
	$(V2600)/memory.c			\
	$(V2600)/raster.c			\
	$(V2600)/collision.c			\
	$(V2600)/limiter.c			\
	$(V2600)/keyboard.c			\
	$(V2600)/options.c			\
	$(V2600)/misc.c				\
	$(V2600)/no_ui.c			\
	$(V2600)/no_sound.c			\
	$(V2600)/no_mouse.c			\
	$(V2600)/no_joy.c			\
	$(V2600)/exmacro.c


V2600INC+= $(V2600)

V2600DEFS+=
USE_COPT+= $(V2600DEFS)
