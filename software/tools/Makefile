#
# SPQR / Ides of March asset build system
#
# John Adams 
# jna@retina.net
# 1/2017
#

# This makefile will build the tools for manipulating media. 
#
# rgbhdr, ImageMagick, and FFMPeg are required to convert images. 
#

CC=cc
BIN=./bin
SOURCE=./src/

PROG=rgbhdr videomerge
LIST=$(addprefix $(BIN)/, $(PROG))

all: $(LIST)

$(BIN)/%:  $(SOURCE)%.c
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)

clean:
	rm -f $(LIST)
