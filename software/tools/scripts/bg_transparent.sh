#!/bin/sh
# Find the color at pixel 0,0 and make it transparent.
# This assumes the pixel at 0,0 is part of the background.
# Note: this only works with certain kinds of image formats.
# (JPEGs don't support transparencies)

BG=`convert $1 -format '%[pixel:p{0,0}]' info:-`
convert $1 -fuzz 7% -transparent $BG $2
