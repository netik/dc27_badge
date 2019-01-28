#!/bin/sh

# This script is used to debug/program a Nordic NRF52 DK reference board,
# with an integrated Segger J-Link interface MCU. This same command works
# for both the nRF52832 and nRF52840 devices.
#
# This script also enables RTOS support for ChibiOS, which is used on
# the DC27 IDES badge. This will allow GDB to identify individual ChibiOS
# thread contexts with the "info threads" command.

openocd -f board/nordic_nrf52_dk.cfg		\
	-c "gdb_flash_program enable"		\
	-c "gdb_breakpoint_override hard"	\
	-c "nrf52.cpu configure -rtos ChibiOS"
