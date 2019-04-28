# daBomb Asset build system
#
# John Adams 
# jna@retina.net
# 4/2019 (originally from our Ides of DEF CON badge)
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

# where we will stage our build before sdcard copy
STAGING_DIR = build

# The name of the sdcard to copy to (OSX only)
VOLNAME= /Volumes/NO\ NAME

.PHONY: tools_dep dirs

all: dirs tools_dep images video sounds photos sdcard

tools_dep:
	$(MAKE) -C $(TOOLS_DIR)

# These rules will build the standard images only if the jpgs have
# changed.

rgb/font/led/%.rgb : jpg/font/led/%.jpg 
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst .jpg,.rgb,$<) $(@)

rgb/icons/%.rgb : tif/icons/%.tif
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,tif/,$(@)) rgb/icons/

rgb/game/%.rgb : tif/game/%.tif
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,tif/,$(@)) rgb/game/

rgb/flags/%.rgb : tif/flags/%.tif
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,tif/,$(@)) rgb/flags/

rgb/%.rgb : jpg/%.jpg 
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,jpg/,$(@)) rgb/images

rgb/images/%.rgb : tif/images/%.tif
	$(TOOLS_DIR)/scripts/convert_to_rgb.sh $<
	mv $(subst rgb/,tif/,$(@)) rgb/images

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
#
# images used by the system, app-fight, and the "LED" app.
#
# if you add subfolders in tif/ you must add a rule here or the
# Maakefile will not build the subfolder.
images: dirs tools_dep photos \
        $(addprefix rgb/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard tif/*.tif))))) \
        $(addprefix rgb/images/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard tif/images/*.tif))))) \
        $(addprefix rgb/icons/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard tif/icons/*.tif))))) \
        $(addprefix rgb/game/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard tif/game/*.tif))))) \
        $(addprefix rgb/font/led/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard jpg/font/led/*.jpg))))) \
        $(addprefix rgb/, $(notdir  $(addsuffix .rgb, $(basename $(wildcard jpg/*.jpg)))))

# When possible please use .wav files as source material. MP3s already
# have enough compression and loss.
sounds: tools_dep $(addprefix dac/, $(notdir  $(addsuffix .snd, $(basename $(wildcard dac/*.wav dac/*.mp3))))) \
	          $(addprefix dac/fight/, $(notdir  $(addsuffix .snd, $(basename $(wildcard dac/fight/*.mp3)))))

video: tools_dep $(addprefix video/, $(notdir  $(addsuffix .vid, $(basename $(wildcard video_src/*.mp4)))))

sdcard: dirs tools_dep images sounds video
	-cp sdcard-readme.txt $(STAGING_DIR)/README.txt
	-cp rgb/images/*.rgb $(STAGING_DIR)/images
	-cp rgb/icons/*.rgb $(STAGING_DIR)/icons
	-cp rgb/font/led/*.rgb $(STAGING_DIR)/font/led
	-cp ../firmware/updater/updater.bin $(STAGING_DIR)
	-cp ../../software/firmware/badge/build/badge.bin $(STAGING_DIR)
	-cp dac/*.snd $(STAGING_DIR)/sound
	-cp photos/*.rgb $(STAGING_DIR)/photos
	-cp rgb/game/*.rgb $(STAGING_DIR)/game
	-cp dac/game/*.snd $(STAGING_DIR)/game
	-cp video/*.vid $(STAGING_DIR)/videos
	@echo
	@echo 'sdcard built in $(STAGING_DIR)'
	@echo
	@echo 'build #'
	@strings $(STAGING_DIR)/badge.bin  | grep -1 gitrev | head -1
	du -kh $(STAGING_DIR)

# copy the card to the right folder on my osx box
osxsd:	sdcard
	@echo "Copying to $(VOLNAME)"
	time cp -R $(STAGING_DIR)/* $(VOLNAME)/
	@sync

# copy only binaries and the updater to the card (reduces dev time
# when needed)
osxsdfw:	sdcard
	@echo "Updating binaries on /Volumes/SPQR_DC25"
	@cp ../updater/updater.bin $(VOLNAME)/
	@cp $(STAGING_DIR)/badge.bin $(VOLNAME)/
	@sync

# cleanups
clean_images:
	-rm -fr rgb/*

dirs:
	-[ -d bin/ ] || mkdir -p bin/
	-[ -d photos/ ] || mkdir -p photos/
	-[ -d $(STAGING_DIR)/game ] || mkdir -p $(STAGING_DIR)/game
	-[ -d $(STAGING_DIR)/icons ] || mkdir -p $(STAGING_DIR)/icons
	-[ -d $(STAGING_DIR)/images ] || mkdir -p $(STAGING_DIR)/images
	-[ -d $(STAGING_DIR)/sound ] || mkdir -p $(STAGING_DIR)/sound
	-[ -d $(STAGING_DIR)/videos ] || mkdir -p $(STAGING_DIR)/videos
	-[ -d $(STAGING_DIR)/font/led ] || mkdir -p $(STAGING_DIR)/font/led
	-[ -d rgb/icons ] || mkdir -p rgb/images
	-[ -d rgb/icons ] || mkdir -p rgb/icons
	-[ -d rgb/flags ] || mkdir -p rgb/flags
	-[ -d rgb/game ] || mkdir -p rgb/game
	-[ -d rgb/font/led ] || mkdir -p rgb/font/led
	-[ -d video ] || mkdir -p video

cleansd:
	@if [ -d "$(STAGING_DIR)" ]; then rm -rf $(STAGING_DIR); fi; 
	@if [ -d "rgb" ]; then rm -rf rgb; fi; 

# remove tools, sounds, images, and video
clean: clean_images
	$(MAKE) -C $(TOOLS_DIR) clean
	-rm -rf bin/
	-rm -f dac/*.snd
	-rm -f dac/fight/*.snd
	-rm -f video/*
	-rm -fr $(STAGING_DIR)/*
	-rm -f photos/*
	-rm -f tif/mirrored/*
	-rm -fr sdcard/*
