#!/bin/sh

# This script is used to debug/program a Nordic NRF52 board using an
# Olimex ARM-USB-OCD-H debugger. This will work with either an nRF528xx DK
# board or the custom DC27 badge board. For the nRF528xx DK boards, you
# must connect the debugger to the Debug In plug next to the CPU.
#
# This script also enables ChibiOS RTOS support in OpenOCD.
#
# Note that you need to set your PATH environment variable to include the
# location where your openocd binary resides before running this script.

openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg	\
	-f interface/ftdi/olimex-arm-jtag-swd.cfg	\
	-f target/nrf52.cfg				\
	-c "gdb_flash_program enable"			\
	-c "gdb_breakpoint_override hard"		\
	-c "nrf52.cpu configure -rtos ChibiOS"	$*
