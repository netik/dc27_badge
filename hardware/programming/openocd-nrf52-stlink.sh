#!/bin/sh

# This script is used to debug/program a Nordic NRF52 board using an
# ST-LINK V2 USB dongle (and clones). Clone devices are available
# for a little as $7.
#
# NOTE: The ST-LINK V2 is a high-level debugger and is not able to
# perform a recovery if you set the APPPROTECT bit in the NVMC block,
# as this disables DAP access. To reset the bit, you need to perform
# a mass erase via the CTRL-AP port, which is not possible with the
# ST-LINK. You'll need to use something like the Olimex debugger
# instead.
#
# Note that you need to set your PATH environment variable to include the
# location where your openocd binary resides before running this script.

openocd -f interface/stlink.cfg				\
	-f target/nrf52.cfg				\
	-c "gdb_flash_program enable"			\
	-c "gdb_breakpoint_override hard"	"$@"
