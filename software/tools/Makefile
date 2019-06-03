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

PROG=rgbhdr videomerge sndskip cp2102
LIST=$(addprefix $(BIN)/, $(PROG))
CFLAGS= -I$(SOURCE)

detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')

LIBS=-lusb

ifeq ($(detected_OS),Darwin)        # Mac OS X
  CFLAGS +=  -I/usr/local/include/libusb-1.0
  LIBS=-L/usr/local/lib -lusb-1.0
endif

all: dirs $(LIST)

dirs: 
	-[ -d bin/ ] || mkdir -p bin/

$(BIN)/%:  $(SOURCE)%.c
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)

clean:
	rm -f $(LIST)

