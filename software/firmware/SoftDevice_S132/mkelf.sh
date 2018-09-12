#!/bin/sh

# This script converts the softdevice.hex file into a softdevice.o ELF
# object that we can link with ChibiOS to produce a complete OS image.
# Note: the entry point address 0x8e9 is the address of the reset
# handler in the SoftDevice module. This address was obtained by
# doing a hex dump of the first few bytes of the object file.

arm-none-eabi-objcopy -I ihex -O binary s132_nrf52_5.1.0_softdevice.hex softdevice.bin
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm softdevice.bin softdevice.o
rm -f softdevice.bin
arm-none-eabi-objcopy -I elf32-littlearm --rename-section .data=.softdevice softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --set-section-flags .softdevice=CONTENTS,ALLOC,LOAD,CODE softdevice.o
arm-none-eabi-objcopy -I elf32-littlearm --add-symbol softdeviceEntry=.softdevice:0x000008e9,global,function softdevice.o
arm-none-eabi-ar rcs libsoftdevice.a softdevice.o
rm -f softdevice.o
