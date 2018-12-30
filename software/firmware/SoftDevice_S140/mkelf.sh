#!/bin/sh

# This script converts the softdevice.hex file into a softdevice.o ELF
# object that we can link with ChibiOS to produce a complete OS image.
# Note: the entry point address 0xA81 is the address of the reset
# handler in the SoftDevice module. This address was obtained by
# doing a hex dump of the first few bytes of the object file.
#
# Note: The s140_nrf52_6.1.1_softdevice.hex file actually contains
# two things: the master boot record at address 0x0 and the SoftDevice
# code at address 0x1000. There is a gap between the two. When we
# convert the .hex file to a binary image, we need to fill the gap
# with 0xFF instead of the default of 0x00. Here's why:
#
# The MBR allows for a bootloader to be installed in the flash which
# can be used for firmware updates. Nordic uses two special locations
# in the UICR region of flash to specify the bootloader address and
# MBR parameter page:
#
# #define MBR_UICR_BOOTLOADER_ADDR (&(NRF_UICR->NRFFW[0]))
# #define MBR_UICR_PARAM_PAGE_ADDR (&(NRF_UICR->NRFFW[1]))
#
# The expectation is that by default these locations will be set
# to 0xFFFFFFFF, which is the factory default. This also happens to
# be the default value of flash locations which have been erased
# and not yet programmed.
#
# Version 2.4.1 of the MBR introduces two new locations:
#
# #define MBR_BOOTLOADER_ADDR (0xFF8)
# #define MBR_PARAM_PAGE_ADDR (0xFFC)
#
# These are located within the MBR region itself. The start-up code
# checks these new locations first to see if they too are 0xFFFFFFFF.
# If so, it then checks the UICR locations as well. If those are
# also 0xFFFFFFFF, then the MBR launches the user application code
# (which for this release is at 0x26000). The locations from the
# end of the MBR code (0xb00) to the start of the SoftDevice (0x1000)
# would end up being 0xFFFFFFFF because normally a programmer reading
# directly from the .hex file would leave that location unset, so it
# would retain the default erase value of 0xffffffff.
#
# Here's the problem: when objcopy converts from hex to bin, it will
# fill the gap from 0xB00 to 0xFFF with 0 instead of 0xFF. When the MBR
# code sees this, it will get stuck in an infinite loop and never
# launch the application code. So we must use the --gap-fill option
# to preserve the expected default behavior.

arm-none-eabi-objcopy -I ihex -O binary --gap-fill 0xFF s140_nrf52_6.1.1_softdevice.hex softdevice.bin
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm softdevice.bin softdevice.o
rm -f softdevice.bin
arm-none-eabi-objcopy -I elf32-littlearm --rename-section .data=.softdevice softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --set-section-flags .softdevice=CONTENTS,ALLOC,LOAD,CODE softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --add-symbol softdeviceEntry=.softdevice:0x00000A81,global,function softdevice.o
arm-none-eabi-ar rcs libsoftdevice.a softdevice.o
rm -f softdevice.o
