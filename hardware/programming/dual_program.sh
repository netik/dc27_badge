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
	-c "reset init"					\
	-c "resume"					\
	-c "exit" &
done
