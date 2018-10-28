#!/bin/sh
#
# SPQR/Ides of March Asset Build System
# 1/2017
#
# John Adams <jna@retina.net> @netik
# Bill Paul <wpaul@windriver.com>
#
# Script to convert a video file into the format used by the
# the video player on the Ides of DEF CON badge. The format isn't
# very complicated: a .VID file contains raw RGB565 pixel data
# and raw 16-bit audio sample data. The player just loads the data
# and transfers it to the screen and DAC as fast as it can.
#
# Usage is:
# encode_video.sh file.mp4 destination/directory/path
#
# Required utilities:
# ffmpeg
# sox
#
# The destination directory must already exist
#
# The video will have an absolute resolution of 160x120 pixels and
# a frame rate of 13 frames per second. These values have been chosen
# to coincide with the audio sample rate of 15625Hz. The video
# and audio will be synchronized during playback.
#

rm -f $2/video.bin

# Convert video to raw rgb565 pixel frames at 8 frames/sec
ffmpeg -i "$1" -r 13 -s 160x120 -f rawvideo -pix_fmt rgb565be $2/video.bin

# Extract audio in stereo
ffmpeg -i "$1" -ac 2 -ar 15625 $2/sample.wav

# Convert WAV to 2's complement signed 16 bit samples, boost gain a little
sox --encoding signed-integer $2/sample.wav $2/sample.raw channels 2

# Skip one out of every 625 samples
./bin/sndskip $2/sample.raw $2/sample.skip

# Now merge the video and audio into a single file
./bin/videomerge $2/video.bin $2/sample.skip $2/video.vid

rm -f $2/video.bin $2/sample.wav $2/sample.u16 $2/sample.raw $2/sample.skip
