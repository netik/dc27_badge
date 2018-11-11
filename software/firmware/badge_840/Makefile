##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

PLATFORM=NRF5/NRF52840
BOARD=NONSTANDARD_NRF52840_IDES_OF_DEFCON

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -Os -ggdb -fomit-frame-pointer -falign-functions=16
  USE_OPT += -DBIG_ENDIAN=0 -DLITTLE_ENDIAN=1
  USE_OPT+= -ansi -std=c99 -Dinline=__inline__ -Dasm=__asm__
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT =
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = 
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x400
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU on Cortex-M4 (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = hard
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = badge

# Imported source files and paths
CHIBIOS         = ../ChibiOS
CHIBIOS_CONTRIB = $(CHIBIOS)/../ChibiOS-Contrib
# Startup files.
include $(CHIBIOS_CONTRIB)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_nrf52840.mk
# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS_CONTRIB)/os/hal/hal.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/$(PLATFORM)/platform.mk
include $(CHIBIOS_CONTRIB)/os/hal/boards/$(BOARD)/board.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk
# Other files (optional).
include $(CHIBIOS)/os/various/shell/shell.mk
include $(CHIBIOS)/os/ex/Micron/m25q.mk

# uGFX includes
GFXLIB = $(CHIBIOS)/../uGFX
include $(GFXLIB)/gfx.mk
include $(GFXLIB)/boards/base/Nonstandard-NRF52-Ides-Of-Defcon/board.mk
include $(GFXLIB)/drivers/gdisp/ILI9341/driver.mk
include $(GFXLIB)/drivers/ginput/touch/MCU/driver.mk

# FatFS library
FATFS = $(CHIBIOS)/../FatFs
include $(FATFS)/build.mk

# Atari 2600 emulator
V2600 = $(CHIBIOS)/../v2600
include $(V2600)/build.mk

# Define linker script file here
LDSCRIPT= NRF52840_softdecvice.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(STARTUPSRC) \
       $(KERNSRC) \
       $(PORTSRC) \
       $(OSALSRC) \
       $(HALSRC) \
       $(M25QSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC) \
       $(TESTSRC) \
       $(SHELLSRC) \
       $(CHIBIOS)/os/hal/lib/streams/memstreams.c \
       $(CHIBIOS)/os/hal/lib/streams/chprintf.c

CSRC += main.c \
	cmd-audio.c \
	cmd-radio.c \
	cmd-reset.c \
	cmd-random.c \
	cmd-watchdog.c \
	orchard-app.c \
	orchard-ui.c \
	ui-keyboard.c \
	ui-list.c \
	app-launcher.c \
	app-test.c \
	app-video.c \
	app-chat.c \
	app-credits.c \
	app-update.c \
	app-email.c \
	app-ledsign.c \
	async_io_lld.c \
	scroll_lld.c \
	ble_gap_lld.c \
	ble_l2cap_lld.c \
	ble_lld.c \
	ble_peer.c \
	i2s_lld.c \
	mmc_spi_lld.c \
	nrf52flash_lld.c \
	flash_qspi_lld.c \
	joypad_lld.c \
	xpt2046_lld.c \
	video_lld.c \
	badge_vectors.c \
	ides_gfx.c \
	scanf.c \
	strtoq.c \
	strtouq.c \
        $(GFXSRC) \
	$(FATFSSRC) \
	$(V2600SRC)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C++ sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC =
ASMXSRC = $(STARTUPASM) $(PORTASM) $(OSALASM)


INCDIR = $(CHIBIOS)/os/license \
         $(STARTUPINC) $(KERNINC) $(PORTINC) $(OSALINC) \
         $(HALINC) $(M25QINC) $(PLATFORMINC) $(BOARDINC) $(TESTINC) \
	 $(SHELLINC) $(GFXINC) $(FATFSINC) $(V2600INC) \
	 $(CHIBIOS)/os/hal/lib/streams $(CHIBIOS)/os/various


INCDIR += $(CHIBIOS_CONTRIB)/os/various

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

MCU  = cortex-m4

TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary
SREC = $(CP) -O srec

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes  -Wno-unused-parameter 

# Define C++ warning options here
CPPWARN = -Wall -Wextra -Wundef

#
# Compiler settings
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = 

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR = $(CHIBIOS)/../SoftDevice_S140/include

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS = -L$(CHIBIOS)/../SoftDevice_S140/lib -lsoftdevice -lc_nano -lnosys

#
# End of user defines
##############################################################################

.PHONY: updatebuildtime

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/
include $(RULESPATH)/rules.mk

PRE_MAKE_ALL_RULE_HOOK: buildtime.h
 
buildtime.h: updatebuildtime
	echo "#define BUILDTIME \"Built: `/bin/date` (`/bin/hostname -s`)\"" > $@
	echo "#define BUILDVER `/bin/date +%s`" >> $@
	echo "#define BUILDMAGIC 0xCAFEBABE" >> $@

OHEX = $(BUILDDIR)/$(PROJECT).hex
OELF = $(BUILDDIR)/$(PROJECT).elf
OBIN = $(BUILDDIR)/$(PROJECT).bin



include $(CHIBIOS_CONTRIB)/os/various/jlink.mk
include $(CHIBIOS_CONTRIB)/os/various/gdb.mk


pin-reset: jlink-pin-reset
flash: all jlink-flash
debug: gdb-debug
erase-all: jlink-erase-all
debug-server: jlink-debug-server

