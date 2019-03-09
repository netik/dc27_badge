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

sox $1 $prefix.s16 channels 2 rate 15625 loudness 12
mv -f $prefix.s16 $prefix.snd
