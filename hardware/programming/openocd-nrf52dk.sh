#!/bin/sh

# This script is used to debug/program a Nordic NRF52 DK reference board,
# with an integrated Segger J-Link interface MCU. This same command works
# for both the nRF52832 and nRF52840 devices.
#
# Note that you need to set your PATH environment variable to include the
# location where your openocd binary resides before running this script.

openocd -f board/nordic_nrf52_dk.cfg		\
	-c "gdb_flash_program enable"		\
	-c "gdb_breakpoint_override hard"	"$@"
