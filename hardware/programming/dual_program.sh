#!/bin/sh

#
# this script will provision two boards at same time.
# it does not configure UICR. 
#


SERIALS="OLAC6A5A OLHLB2K"
serialarr=($SERIALS)

for SERIAL in $SERIALS;
do
  openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg	\
	-f interface/ftdi/olimex-arm-jtag-swd.cfg	\
	-f target/nrf52.cfg				\
        -c "ftdi_serial ${SERIAL}"                      \
	-c "init"					\
	-c "reset halt"					\
        -c "nrf5 mass_erase" \
	-c "flash banks"				\
	-c "flash write_bank 0 ../../software/firmware/badge/build/badge.bin" \
       	-c "mww 0x4001E504 0x00000001"			\
	-c "mdw 0x4001E504"				\
	-c "mww 0x10001200 0x00000012"			\
	-c "mdw 0x10001200"				\
	-c "mww 0x10001204 0x00000012"			\
	-c "mdw 0x10001204"				\
	-c "mww 0x10001080 0x00112233"			\
	-c "mdw 0x10001080"				\
	-c "mww 0x10001084 0x44556677"			\
	-c "mdw 0x10001084"				\
	-c "mww 0x10001088 0x8899AABB"			\
	-c "mdw 0x10001088"				\
	-c "mww 0x1000108C 0xCCDDEEFF"			\
	-c "mdw 0x1000108C"				\
	-c "mww 0x10001090 0x6672616E"			\
	-c "mdw 0x10001090"				\
	-c "mww 0x10001094 0x00000000"			\
	-c "mdw 0x10001094"				\
        -c "mww 0x100010d0 0x0001deaf"                  \
        -c "mdw 0x100010d0"                             \
        -c "mww 0x100010d4 0x000f10a7"                  \
        -c "mdw 0x100010d4"                             \
        -c "mww 0x100010d8 0x000fa4a1"                  \
        -c "mdw 0x100010d8"                             \
        -c "mww 0x100010dc 0x000c0571"                  \
        -c "mdw 0x100010dc"                             \
        -c "mww 0x100010e0 0x000d2d2d"                  \
        -c "mdw 0x100010e0"                             \
        -c "mww 0x100010e4 0x00031337"                  \
        -c "mdw 0x100010e4"                             \
        -c "mww 0x100010e8 0x000feed5"                  \
        -c "mdw 0x100010e8"                             \
        -c "mww 0x100010ec 0x0000c01d"                  \
        -c "mdw 0x100010ec"                             \
        -c "mww 0x100010f0 0x000a55e5"                  \
        -c "mdw 0x100010f0"                             \
        -c "mww 0x100010f4 0x000b00b5"                  \
        -c "mdw 0x100010f4"                             \
        -c "mww 0x100010f8 0x000c0ded"                  \
        -c "mdw 0x100010f8"                             \
	-c "mww 0x4001E504 0x00000000"			\
	-c "mdw 0x4001E400"				\
        -c "program ../../software/firmware/badge/build/badge.elf" \
	-c "reset init"					\
	-c "resume"					\
	-c "exit" &
done

