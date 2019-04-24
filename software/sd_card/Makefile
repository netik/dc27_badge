#
# daBomb Asset build system
#
# John Adams 
# jna@retina.net
# 4/2019 (originally 1/2017 from Ides of DEF CON badge)
#
# Master makefile
#
# This makefile will build the tools and media that will be placed on
# the SD cards
#
# ImageMagick, and FFMPeg are required to convert images.
# the Makefile in the $(TOOLS_DIR)/ subdir will build our required tools (rgbheader, snd
#
# Videos will not be built by this script. We will only copy .vid
# files to the sdcard from the video/ subdir.
#

# The tools dir contains our conversion tools and scripts
TOOLS_DIR = ../tools
.PHONY: tools_dep 

tools_dep:
	$(MAKE) -C $(TOOLS_DIR)

all: dirs tools_dep images sounds video

# These rules will build the mirrored and standard images only if the
# jpgs have changed.

rgb/font/led/%.rgb : jpg/font/led/%.jpg 
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst .jpg,.rgb,$<) $(@)

rgb/%.rgb : jpg/%.jpg 
	convert -flop $< $(subst .jpg,r.jpg,$(subst jpg/,jpg/mirrored/,$<))
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,jpg/,$(@)) rgb/
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $(subst .jpg,r.jpg,$(subst jpg/,jpg/mirrored/,$<))
	mv $(subst .jpg,r.rgb,$(subst jpg/,jpg/mirrored/,$<)) rgb/

rgb/%.rgb : tif/%.tif
	convert -flop $< $(subst .tif,r.tif,$(subst tif/,tif/mirrored/,$<))
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,tif/,$(@)) rgb/
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $(subst .tif,r.tif,$(subst tif/,tif/mirrored/,$<))
	mv $(subst .tif,r.rgb,$(subst tif/,tif/mirrored/,$<)) rgb/

# User-supplied images
#
# put your photos in photos_src and then run 'make photos' to make the
# rgb files.  photos must be 320x240, To facilitate scrolling, we have
# to reverse and rotate the image 90 degress, just like we do for the
# credits app.
#
photos/%.rgb: photos_src/%.tif
	convert $< -resize \!320x240 -rotate 90 -flop $(subst photos_src/,photos/,$<)
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $(subst photos_src/,photos/,$<)
	rm $(subst photos_src/,photos/,$<)

# User supplied photos if any.
photos: tools_dep $(addprefix photos/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard photos_src/*.tif)))))

# Sounds
dac/%.snd: dac/%.mp3
	$(TOOLS_DIR)/scripts/sndmp3toraw.sh $<

dac/%.snd: dac/%.wav
	$(TOOLS_DIR)/scripts/sndmp3toraw.sh $<

dac/fight/%.snd: dac/fight/%.mp3
	$(TOOLS_DIR)/scripts/sndmp3toraw.sh $<

# Videos
video/%.vid: video_src/%.mp4
	$(TOOLS_DIR)/scripts/encode_video.sh $< video

# When possible, please use TIF images. Aliasing is a real issue on
# our display and higher quality input is better.
# images used by the system, app-fight, and the "LED" app.
images: dirs tools_dep photos \
        $(addprefix rgb/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard tif/*.tif))))) \
        $(addprefix rgb/font/led/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard jpg/font/led/*.jpg))))) \
        $(addprefix rgb/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard jpg/*.jpg)))))

# When possible please use .wav files as source material. MP3s already
# have enough compression and loss.
sounds: tools_dep $(addprefix dac/, $(notdir  $(addsuffix .snd, $(basename $(wildcard dac/*.wav dac/*.mp3))))) \
	          $(addprefix dac/fight/, $(notdir  $(addsuffix .snd, $(basename $(wildcard dac/fight/*.mp3)))))

video: tools_dep $(addprefix video/, $(notdir  $(addsuffix .vid, $(basename $(wildcard video_src/*.mp4)))))

sdcard: dirs tools_dep images sounds video
	-cp sdcard-readme.txt sdcard/README.txt
	-cp rgb/*.rgb sdcard
	-cp rgb/font/led/*.rgb sdcard/font/led
	-cp ../firmware/updater/updater.bin sdcard
	-cp ../firmware/badge/build/badge.bin sdcard
	-cp dac/*.snd sdcard
	-cp photos/*.rgb sdcard/photos
	-cp dac/fight/*.snd sdcard/fight
	-cp video/*.vid sdcard
	@echo
	@echo 'sdcard built!'
	@echo
	@echo 'build #'
	@strings sdcard/badge.bin  | grep -1 gitrev | head -1
	du -kh sdcard

# copy the card to the right folder on my osx box
osxsd:	sdcard
	@echo "Copying to /Volumes/SPQR_DC25/"
	time cp -R sdcard/* /Volumes/SPQR_DC25/
	@sync

# copy only binaries and the updater to the card (reduces dev time
# when needed)
osxsdfw:	sdcard
	@echo "Updating binaries on /Volumes/SPQR_DC25"
	@cp ../updater/updater.bin /Volumes/SPQR_DC25/
	@cp sdcard/badge.bin /Volumes/SPQR_DC25
	@sync

# cleanups
clean_images:
	-rm -f jpg/mirrored/*
	-rm -f tif/mirrored/*
	-rm -fr rgb/*

dirs:
	-[ -d bin/ ] || mkdir -p bin/
	-[ -d sdcard/fight ] || mkdir -p sdcard/fight
	-[ -d sdcard/photos ] || mkdir -p sdcard/photos
	-[ -d sdcard/font/led ] || mkdir -p sdcard/font/led
	-[ -d rgb/font/led ] || mkdir -p rgb/font/led
	-[ -d video ] || mkdir -p video
	-[ -d tif/mirrored ] || mkdir -p tif/mirrored
	-[ -d jpg/mirrored ] || mkdir -p jpg/mirrored

cleansd:
	@if [ -d "sdcard" ]; then rm -rf sdcard; fi; 
	@if [ -d "rgb" ]; then rm -rf rgb; fi; 

# remove tools, sounds, images, and video
clean: clean_images
	$(MAKE) -C $(TOOLS_DIR) clean
	-rm -rf bin/
	-rm -f dac/*.snd
	-rm -f dac/fight/*.snd
	-rm -f video/*
	-rm -f tif/mirrored/*
	-rm -fr sdcard/*
	-rmdir tif/mirrored
	-rmdir jpg/mirrored
	-rmdir video
