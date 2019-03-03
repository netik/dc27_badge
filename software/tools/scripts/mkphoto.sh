#!/bin/sh

# This script formats images to turn them into photos for the photo
# app.
#
# The screen on the Ides of Defcon badge is 320x240 pixels so photos
# must be this size to fill the display. Also, because of the way the
# scrolling works, we need to do the following two things to prepare
# the image so that it diplays correctly:
#
# 1) Rotate the image left by 90 degrees
# 2) "Flop" the image to reverse it
#
# This script depends on the following tools to work correctly:
# - ImageMagick identify(1) utility
# - ImageMagick convert(1) utility
# - ffmpeg
# - the rgbhdr tool from this distribution
#
# Image files will be forcibly resized to 320x240 pixels, then processed
# into the RGB565 native format of the display. Once the files are generated,
# they can be copied to the "photos" subdirectory on the badge's SD card.
#

MYDIR=`dirname "$0"`

if [ "$1" == "" ]; then
    echo "Usage: $0 filename"
    exit
fi

convert $1 -resize !320x240 -rotate 90 -flop photo.jpg
${MYDIR}/convert_to_rgb.sh photo.jpg photo.rgb
