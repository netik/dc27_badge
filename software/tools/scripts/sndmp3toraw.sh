#!/bin/sh

# SPQR/Ides of March Asset Build System
# 1/2017
#
# John Adams <jna@retina.net> @netik
# Bill Paul <wpaul@windriver.com>
#
# sndmp3toraw.sh
#
# Convert mp3 to 12.5Khz audio
#

prefix="`echo $1 | cut -d . -f 1`"

echo "$prefix"
sox --encoding signed-integer $1 $prefix.raw channels 1 rate 12500 contrast 80
