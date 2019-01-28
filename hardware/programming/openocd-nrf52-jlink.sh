#!/bin/sh

# This script is used to debug/program a Nordic NRF52 board using an
# Segger J-Link debugger. This script works both with the nRF528xx DK
# boards and custom boards using an off-board Segger J-Link debugger.

openocd -f interface/jlink.cfg				\
	-c "transport select swd"			\
	-f target/nrf52.cfg				\
	-c "gdb_flash_program enable"			\
	-c "gdb_breakpoint_override hard"	$*
