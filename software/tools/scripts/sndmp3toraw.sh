#!/bin/sh

# SPQR/Ides of March Asset Build System
# 1/2017
#
# John Adams <jna@retina.net> @netik
# Bill Paul <wpaul@windriver.com>
#
# sndmp3toraw.sh
#
# Convert mp3 to 15.625Mhz audio in stereo
#

prefix="`echo $1 | cut -d . -f 1`"

sox --encoding signed-integer $1 $prefix.raw channels 2 rate 15625 contrast 80
mv -f $prefix.raw $prefix.snd
