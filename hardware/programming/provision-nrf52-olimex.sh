#!/bin/sh

# This script is used to program a Nordic NRF52 board using an
# Olimex ARM-USB-OCD-H debugger. This will work with either an nRF528xx DK
# board or the custom DC27 badge board. For the nRF528xx DK boards, you
# must connect the debugger to the Debug In plug next to the CPU.
#
# Note that you need to set your PATH environment variable to include the
# location where your openocd binary resides before running this script.
#
# This is a provisioning script. It's designed to flash an ELF firmware
# image to a badge board and to program the customer information area,
# to do things like set the reset pin configuration, badge serial number,
# and the unlocks password.
#
# In addition to 1MB of flash at address 0, the nRF52840 chip has two
# additional banks of non-volatile memory:
#
# 0x10000000 - Factory Information Configuration Region (FICR)
# 0x10001000 - User Information Configuration Region (UICR)
#
# The FICR is permanently programmed by Nordic and is read-only. It contains
# the chip hardware info, unique device ID (serial number), and BLE station
# address.
#
# The UICR can be erased and programmed using the NVMC registers and is
# intended for Nordic customers to store their own application-specific
# info. The reset pin configuration is stored here since it's really up
# to the user to decide which pin to wire up as the reset pin. (We happen
# to use the same pin as the Nordic nRF52840 DK board.)
#
# Doing a mass erase will delete everything in both the flash and UICR
# regions, effectively putting the chip back to a factory default state.
# We can then flash our firmware image and set up the UICR region.
#
# There's one catch though. The nrf52 flash driver in OpenOCD has some
# funny ideas about how to treat the UICR region. It only lets you program
# it once, on the assumption that you have to erase it like a flash sector
# every time you want to reprogram it. This isn't true: once you erase it,
# you can enable writes to it and write to any region that hasn't been
# set yet without having to fully erase it each time.
#
# We would like to be able to program various words in the UICR individually,
# but because of the nrf52 flash driver's behavior, you can only use the
# flash command to write to the UICR once. After that, it won't let you
# write again until you do another mass erase. This is a bit silly.
#
# To get around this, we program the NVMC directly and just use memory
# write commands to update regions in the UICR. The NVMC register bank
# is at offset 0x4001E000 in the address space, and register 0x504 is
# the CONFIG register, which we can use to enable flash writes. Doing
# this also lets us write to the UICR.
#
# Offsets 0x10001200 and 0x10001204 correspond to UICR->PSELRESET[0]
# and UICR->PSELRESET[1] registers. We program this with 0x12, which
# corresponds to GPIO pin 18. You need to program both locations in
# order for the chip to recognize the setting. The setting is only latched
# into the chip after a reset, so the script resets the CPU right
# before exiting.
#
# Offsets 0x10001080 through 0x100010FC correspond to UICR->CUSTOMER[0]
# through UICR->CUSTOMER[31]. We can put anything we want there.
#
# Currently this script programs some test values to UICR->CUSTOMER[0]
# through UICR->CUSTOMER[3]. It then sets UICR->CUSTOMER[4] to a 4-byte
# string, which can act as the unlocks password. (The string can be longer,
# this is just an example.)
#
# The intent is that when we provision the badges, we can use this script
# to program the unlocks password on each badge rather than having the
# password compiled in the firmware, or present in the git repo, where
# people can easily see it.
#
# Of course, it's still possible for someone to extract the password by
# connecting a JTAG/SWD debugger to the badge and dumping the memory. We
# could also prevent this by setting the APPROTECT bit in the NVMC to
# lock the chip, but that wouldn't stop someone from using the recovery
# process to blank the chip and flash it with a new firmware image and
# a password of their own choosing.
#
# But this is not intended to be a foolproof strategy: we just want to
# keep people from cheating right away.

openocd -f interface/ftdi/olimex-arm-usb-ocd-h.cfg	\
	-f interface/ftdi/olimex-arm-jtag-swd.cfg	\
	-c "init"					\
	-c "reset halt"					\
	-c "nrf5 mass_erase"				\
	-c "program build/badge.elf"			\
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
	-c "mww 0x10001090 0x6E617266"			\
	-c "mdw 0x10001090"				\
	-c "mww 0x10001094 0x00000000"			\
	-c "mdw 0x10001094"				\
	-c "mdw 0x4001E400"				\
	-c "mww 0x4001E504 0x00000000"			\
	-c "reset init"					\
	-c "resume"					\
	-c "exit"
