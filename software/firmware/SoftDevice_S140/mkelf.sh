#!/bin/sh

# This script converts the softdevice.hex file into a softdevice.o ELF
# object that we can link with ChibiOS to produce a complete OS image.
# Note: the entry point address 0xA81 is the address of the reset
# handler in the SoftDevice module. This address was obtained by
# doing a hex dump of the first few bytes of the object file.
#
# Note: Softdevice S140 version 6.1.1 (released Novemvber 2018) was
# found to fail bootstrap on an nRF52840 board. It's not exactly clear
# why. Version 6.1.0 seemed to examine the UICR (at address 0x10001000)
# in order to determine part of the bootstrap path. Version 6.1.1
# seems to look at values stored in the flash at offset 0xffc and
# 0xff8 instead. These locations contain 0 by default. The end result
# is that the bootstrap code gets stuck in an endless loop, constantly
# branching back to the entry point (0xA81). Since the code
# always seems to examine these locations in flash and they can
# never change, it's not clear exactly how this is ever supposed to
# work. It's possible that the proprietary Nordic flashing tool
# modifies these locations when flashing the SoftDevice.
#
# To get around this, we patch the binary image at address 0xff8
# so that it contains the value 0xFFFFFFFF instead of 0. This seems
# to allow the SoftDevice to boot into the application code at
# 0x26000 as expected. We go to some trouble to do this using only
# standard UNIX utilities so that this script remains relatively
# portable.

arm-none-eabi-objcopy -I ihex -O binary s140_nrf52_6.1.1_softdevice.hex softdevice.bin
awk 'BEGIN{ printf "%c%c%c%c",  255, 255, 255, 255 }' > patch.txt
dd if=patch.txt of=softdevice.bin conv=notrunc obs=1 seek=4088
dd if=patch.txt of=softdevice.bin conv=notrunc obs=1 seek=4092
rm patch.txt
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm softdevice.bin softdevice.o
rm -f softdevice.bin
arm-none-eabi-objcopy -I elf32-littlearm --rename-section .data=.softdevice softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --set-section-flags .softdevice=CONTENTS,ALLOC,LOAD,CODE softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --add-symbol softdeviceEntry=.softdevice:0x00000A81,global,function softdevice.o
arm-none-eabi-ar rcs libsoftdevice.a softdevice.o
rm -f softdevice.o
