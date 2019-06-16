ZMACHINESRC +=				\
	$(ZMACHINE)/acursesio.c		\
	$(ZMACHINE)/control.c		\
	$(ZMACHINE)/extern.c		\
	$(ZMACHINE)/fileio.c		\
	$(ZMACHINE)/getopt.c		\
	$(ZMACHINE)/input.c		\
	$(ZMACHINE)/interpre.c		\
	$(ZMACHINE)/jzip.c		\
	$(ZMACHINE)/license.c		\
	$(ZMACHINE)/math.c		\
	$(ZMACHINE)/mcurses.c		\
	$(ZMACHINE)/memory.c		\
	$(ZMACHINE)/object.c		\
	$(ZMACHINE)/operand.c		\
	$(ZMACHINE)/osdepend.c		\
	$(ZMACHINE)/property.c		\
	$(ZMACHINE)/quetzal.c		\
	$(ZMACHINE)/screen.c		\
	$(ZMACHINE)/text.c		\
	$(ZMACHINE)/variable.c

ZMACHINEINC += $(ZMACHINE)

ZMACHINEDEFS += -DHARD_COLORS -DZ_FILENAME_MAX=12 -DZ_PATHNAME_MAX=50
USE_COPT += $(ZMACHINEDEFS)
