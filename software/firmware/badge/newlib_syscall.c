/*-
 * Copyright (c) 2019
 *      Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This module provides some OS-specific shims for the newlib libc
 * implementation so that we can use printf() and friends as well as
 * malloc() and free(). ChibiOS has its own heap management routines
 * (chHeapAlloc()/chHeapFree()) which must be disabled for us to use
 * the ones in newlib.
 *
 * Technically we can get away with the default _sbrk() shim in newlib,
 * but the one here uses __heap_base__ instead of "end" as the heap
 * start pointer which is slightly more correct.
 *
 * We need to add the "used" attribute tag to these functions to avoid a
 * potential problem with LTO. If newlib is built with link-time optimization
 * then there's no problem, but if it's not, the LTO optimizer can
 * mistakenly think that these functions are not needed (possibly because
 * they also appear in libnosys.a) and it will optimize them out. Using
 * the tag defeats this. (It's redundant but harmless when not using LTO.)
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <stdio.h>

#include "ch.h"
#include "hal.h"
#include "osal.h"

#include "ff.h"

extern char   __heap_base__; /* Set by linker */
extern char   __heap_end__; /* Set by linker */

static mutex_t malloc_mutex;

#define MAX_FILES 5
static FIL file_handles[MAX_FILES];
static int file_used[MAX_FILES];

void
newlibStart (void)
{
	osalMutexObjectInit (&malloc_mutex);
}


__attribute__((used))
int
_read (int file, char * ptr, int len)
{
	int i;
	FIL * f;
	UINT br;

	/* descriptor 0 is always stdin */

	if (file == 0) {
		ptr[0] = sdGet (&SD1);
		return (1);
	}

	/* Can't read from stdout or stderr */

	if (file == 1 || file == 2 || file > MAX_FILES) {
		errno = EINVAL;
		return (-1);
	}

	i = file - 3;
	if (file_used[i] == 0) {
		errno = EINVAL;
		return (-1);
	}

	f = &file_handles [i];

	if (f_read (f, ptr, len, &br) != FR_OK) {
		errno = EIO;
		return (-1);
	}

	return (br);
}

__attribute__((used))
int
_write (int file, char * ptr, int len)
{
	int i;
	UINT br;
	FIL * f;

	if (file == 1 || file == 2) {
		for (i = 0; i < len; i++) {
			if (ptr[i] == '\n')
				sdPut (&SD1, '\r');
			sdPut (&SD1, ptr[i]);
		}
		return (len);
	}

	if (file == 0 || file > MAX_FILES) {
		errno = EINVAL;
		return (-1);
	}

	i = file - 3;

	if (file_used[i] == 0) {
		errno = EINVAL;
		return (-1);
	}

	f = &file_handles [i];

	if (f_write (f, ptr, len, &br) != FR_OK) {
		errno = EIO;
		return (-1);
	}

	return (br);
}

__attribute__((used))
int
_open (const char * file, int flags, int mode)
{
	FIL * f;
	FRESULT r;
	BYTE fmode = 0;
	int i;

	for (i = 0; i < MAX_FILES; i++) {
		if (file_used[i] == 0)
			break;
	}

	if (i == MAX_FILES) {
		errno = ENOSPC;
		return (-1);
	}

	f = &file_handles[i];

	fmode = FA_READ;

	if (flags & O_WRONLY)
		fmode = FA_WRITE;
	if (flags & O_RDWR)
		fmode = FA_READ|FA_WRITE;

	if (flags & O_CREAT)
		fmode |= FA_CREATE_ALWAYS;
	if (flags & O_APPEND)
		fmode |= FA_OPEN_APPEND;

	r = f_open (f, file, fmode);

	if (r != FR_OK) {
		if (r == FR_NO_FILE)
			errno = ENOENT;
		else if (r == FR_DENIED)
			errno = EPERM;
		else if (r == FR_EXIST)
			errno = EEXIST;
		else
			errno = EIO;
		return (-1);
	}

	file_used[i] = 1;

	/*
	 * stdin, stdout and stderr are descriptors
	 * 0, 1 and 2, so start at 3
	 */

	i += 3;

	return (i);
}

__attribute__((used))
int
_close (int file)
{
	int i;
	FIL * f;

	if (file < 3 || file > MAX_FILES) {
		errno = EINVAL;
		return (-1);
	}

	i = file - 3;

	if (file_used[i] == 0) {
		errno = EINVAL;
		return (-1);
	}

	f = &file_handles [i];
	file_used[i] = 0;
	f_close (f);

	return (0);
}

__attribute__((used))
int
_lseek (int file, int ptr, int dir)
{
	int i;
	FIL * f;
	FSIZE_t offset = 0;

	/* Not sure how to handle SEEK_END */

	if (dir == SEEK_END) {
		errno = EINVAL;
		return (-1);
	}

	if (file < 3 || file > MAX_FILES) {
		errno = EINVAL;
		return (-1);
	}

	i = file - 3;

	if (file_used[i] == 0) {
		errno = EINVAL;
		return (-1);
	}

	f = &file_handles [i];

	if (dir == SEEK_SET)
		offset = ptr;

	if (dir == SEEK_CUR)
		offset = f_tell (f) + ptr;

	if (f_lseek (f, offset) != FR_OK) {
		errno = EIO;
		return (-1);
	}

	return (offset);
}

__attribute__((used))
int
_stat (const char * file, struct stat * st)
{
	FILINFO f;
	FRESULT r;

	r = f_stat (file, &f);

	if (r != FR_OK) {
		if (r == FR_NO_FILE)
			errno = ENOENT;
		else if (r == FR_DENIED)
			errno = EPERM;
		else
			errno = EIO;
		return (-1);

	}

	st->st_size = f.fsize;
	st->st_blksize = 512;
	st->st_blocks = f.fsize / 512;
	if (st->st_blocks == 0)
		st->st_blocks++;

	return (0);
}

__attribute__((used))
int
_fstat (int desc, struct stat * st)
{
	FIL * f;
	int i;

	st->st_blksize = 512;
	st->st_size = 0;
	st->st_blocks = 0;

	if (desc < 3)
		return (0);

	if (desc > MAX_FILES) {
		errno = EINVAL;
		return (-1);
	}

	i = desc - 3;
	if (file_used[i] == 0) {
		errno = EINVAL;
		return (-1);
	}

	f = &file_handles [i];

	st->st_size = f_size (f);
	st->st_blocks = f_size (f) / 512;
	if (st->st_blocks == 0)
		st->st_blocks++;

	return (0);
}

__attribute__((used))
void
__malloc_lock (struct _reent * ptr)
{
	osalMutexLock (&malloc_mutex);
	return;
}

__attribute__((used))
void
__malloc_unlock (struct _reent * ptr)
{
	osalMutexUnlock (&malloc_mutex);
	return;
}

__attribute__((used))
void *
_sbrk (int incr)
{
	static char * heap_end;
	char *        prev_heap_end;

	if (heap_end == NULL)
		heap_end = &__heap_base__;
    
	if ((heap_end + incr) > &__heap_end__) {
		errno = ENOMEM;
		return ((void *)-1);
	}

	prev_heap_end = heap_end;
	heap_end += incr;

	return (void *) prev_heap_end;
}
