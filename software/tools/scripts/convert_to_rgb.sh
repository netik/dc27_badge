#!/bin/sh

# SPQR/Ides of March Asset Build System
# 1/2017
#
# John Adams <jna@retina.net> @netik
# Bill Paul <wpaul@windriver.com>
#
# convert_to_rgb.sh
#
# Convert any readable image into a rgb565be image with proper header.
# Requires ffmpeg, imagemagick, and rgbhdr
#
# Run this from chibios-orchard/sd_card only, or tools will not be
# located.
#

MYDIR=`dirname "$0"`

FFMPEG=/usr/local/bin/ffmpeg
IDENTIFY=/usr/local/bin/identify
RGBHDR=${MYDIR}/../bin/rgbhdr

if [ "$1" == "" ]; then
    echo "Usage: $0 filename"
    exit
fi

filename=$1
newfilename=${filename%.*}.rgb
width=`${IDENTIFY} -format %w $filename`
height=`${IDENTIFY} -format %h $filename`

# create the rgb header
${RGBHDR} ${width} ${height} /tmp/hdr$$.rgb
CODEC=jpegls
extension=.tif

if [ "$filename" != "${filename%"$extension"*}" ];
then
    CODEC=tiff
fi

${FFMPEG} -loglevel panic -vcodec ${CODEC} -i $filename -vcodec rawvideo -f rawvideo -pix_fmt rgb565be /tmp/out$$.raw 

if [ $? != 0 ]; then
    echo "FFMPEG failed to decode ${filename} !"
    exit 1
fi


cat /tmp/hdr$$.rgb /tmp/out$$.raw > ${newfilename}

rm -f /tmp/hdr$$.rgb
rm -f /tmp/out$$.raw

