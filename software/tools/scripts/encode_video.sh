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
# a frame rate of 16.25 frames per second. These values have been
# chosen to coincide with the audio sample rate of 15625Hz. The video
# and audio will be synchronized during playback.
#
# You may be wondering why we chose a video rate of 16.25 frames/second
# and an audio sample rate of 15.6KHz here. (Or maybe not. But I'm going
# to tell you anyway.)
#
# In order to keep the audio and video in sync, we need the ratio of audio
# samples per video scanline to be a whole number. (That is, if you divide
# the number of audio samples per second by the number of video scanlines
# per second, you should end up with a whole number as a result.)
# 
# With the DC25 badge design, we used a simple DAC for audio output and
# controlled the sample rate using an interval timer. This allowed the
# audio sample rate to be set to any value that we wanted, which in turn
# allowed us to choose a sample rate that yielded a whole number of samples
# per scaline.
#
# For the DC27 design, we're using the I2S controller in the Nordic
# nRF52840 as the audio output device. For I2S, you control the sample
# rate by setting the left/right clock, which in turn is supposed to be
# some fraction of the master clock. The Nordic I2S design only supports
# a fixed set of possible master clock frequencies and LRCLK divisors,
# so our choices for the LRCLK clock are limited. We wanted a sample
# rate of around 16KHz. The closest you can is 15.625KHz.
#
# This means we can't adjust the audio sample rate arbitrarily. It
# turns out you can get a whole number ratio with a video rate of 13
# frames per second. However the hardware has enough bandwidth for
# about 16 frames per second, and a higher frame rate would be nice.
# But at 16 frames/second, the math doesn't work out.
#
# For some reason I had become fixated on the idea of having a whole
# number video frame rate, but then I remembered that fractional frame
# rates are a perfectly acceptable thing and ffmpeg supports them just
# fine. (The NTSC frame rate is 29.97 fps after all.)
#
# The problem though is that even with 16.25 frames/second, we get
# close to a whole number ratio, but not quite:
#
# 16.25 x 120 = 1950 scanlines/second
# 15625 / 1950 = 8.01282051282051282051 samples/scanline
#
# Again, we're stuck with the LRCLK frequency of 15.625KHz. Our only
# remaining recourse is to cheat a little, and encode our audio tracks
# at a rate of 15.6KHz:
#
# 15600 / 1950 = 8
#
# This is slightly slower than the rate expected by the I2S controller,
# and it effectively results in the audio playback being slowed down
# compared to the original. However it amounts to a difference of
# only 0.16%, which is so small that a human listener can't really
# notice it.
#

MYDIR=`dirname "$0"`

filename=`basename "$1"`
newfilename=${filename%.*}.vid

rm -f $2/video.bin

# Convert video to raw rgb565 pixel frames at 16.25 frames/sec
ffmpeg -i "$1" -r 16.25 -s 160x120 -f rawvideo -pix_fmt rgb565be $2/video.bin

# Extract audio in stereo
ffmpeg -i "$1" -ac 2 -ar 15600 $2/sample.wav

# Convert WAV to 2's complement signed 16 bit samples
sox $2/sample.wav $2/sample.s16 channels 2 rate 15600 loudness 12

# Now merge the video and audio into a single file
${MYDIR}/../bin/videomerge $2/video.bin $2/sample.s16 $2/${newfilename}

rm -f $2/video.bin $2/sample.wav $2/sample.s16
