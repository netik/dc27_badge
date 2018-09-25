FATFSSRC+=					\
	$(FATFS)/source/ff.c			\
	$(FATFS)/source/diskio.c		\
	$(FATFS)/source/syscall.c

FATFSINC+= $(FATFS)/source

FATFSDEFS+= -UDRV_CFC -DDRV_MMC=0 -DDRV_QSPI=1
USE_COPT+= $(FATFSDEFS)
