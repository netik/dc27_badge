# Copyright (c) 2016-2017
#      Bill Paul <wpaul@windriver.com>.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by Bill Paul.
# 4. Neither the name of the author nor the names of any co-contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


VPATH= ../ChibiOS-Contrib/os/hal/ports/NRF5/NRF52840 			\
	../ChibiOS-Contrib/os/hal/ports/NRF5/LLD			\
	../ChibiOS/os/hal/ports/common/ARMCMx				\
	../ChibiOS/os/hal/src ../FatFs/source ../badge_840		\
	../ChibiOS-Contrib/os/hal/boards/NONSTANDARD_NRF52840_IDES_OF_DEFCON			\

OBJS=	entry.o updater.o nvic.o ff.o diskio.o mmc_spi_lld.o	\
	flash.o hal_gpt_lld.o hal_spi_lld.o hal_gpt.o hal_spi.o \
	hal_pal_lld.o board.o

CC=	arm-none-eabi-gcc
LD=	arm-none-eabi-ld
OBJCOPY= arm-none-eabi-objcopy

CFLAGS+=  -mcpu=cortex-m4 -Os -ggdb -fomit-frame-pointer		\
	-mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant	\
	-falign-functions=4 -nostdlib -DBIG_ENDIAN=0 -DLITTLE_ENDIAN=1	\
	-Dasm=__asm__ -ansi -ffunction-sections				\
	-DUPDATER -fdata-sections -fno-common -std=c99			\
	-Wall -Wextra -Wstrict-prototypes				\
	-DCORTEX_USE_FPU=TRUE -DTHUMB_PRESENT -mno-thumb-interwork	\
	-D__VTOR_PRESENT=1						\
	-DTHUMB_NO_INTERWORKING -mthumb -DTHUMB -I.			\
	-I../badge_840 -I../ChibiOS/os/license				\
	-I../ChibiOS-Contrib/os/hal/ports/NRF5/NRF52840			\
	-I../ChibiOS-Contrib/os/hal/ports/NRF5/LLD			\
	-I../ChibiOS-Contrib/os/hal/include				\
	-I../ChibiOS/os/common/ext/ARM/CMSIS/Core/Include 		\
	-I../ChibiOS-Contrib/os/common/startup/ARMCMx/devices/NRF52840	\
	-I../ChibiOS/os/rt/include -I../ChibiOS/os/common/ports/ARMCMx	\
	-I../ChibiOS/os/hal/ports/common/ARMCMx				\
	-I../ChibiOS/os/common/ports/ARMCMx/compilers/GCC		\
	-I../ChibiOS/os/hal/osal/rt  					\
	-I../ChibiOS/os/common/oslib/include  				\
	-I../ChibiOS/os/hal/include 					\
	-I../ChibiOS-Contrib/os/hal/boards/NONSTANDARD_NRF52840_IDES_OF_DEFCON \
	-I../FatFs/source -DDRV_MMC=0 -UDRV_CFC -UDRV_QSPI -flto

ASFLAGS= $(CFLAGS)

updater: $(OBJS)
	$(CC) $(OBJS) -ggdb -nostdlib -nostartfiles -mfloat-abi=hard	\
	-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -flto			\
	-Wl,--no-warn-mismatch,-Tlink.lds -lc -lgcc -o $@
	$(OBJCOPY) -O binary $@ updater.bin

clean:
	rm -f updater *.o *.bin

all: updater
