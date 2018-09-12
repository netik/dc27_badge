FATFSSRC+=					\
	$(FATFS)/source/ff.c			\
	$(FATFS)/source/diskio.c		\
	$(FATFS)/source/syscall.c

FATFSINC+= $(FATFS)/source

FATFSDEFS+= -DDRV_MMC=0 -UDRV_CFC
USE_COPT+= $(FATFSDEFS)
