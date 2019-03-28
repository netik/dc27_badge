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

#include "ch.h"
#include "hal.h"

extern char   __heap_base__; /* Set by linker */

__attribute__((used))
int
_write (int file, char * ptr, int len)
{
	int i;
	if (file != 1 && file != 2) {
		errno = ENOSYS;
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (ptr[i] == '\n')
			sdPut (&SD1, '\r');
		sdPut (&SD1, ptr[i]);
	}

	return (len);
}

__attribute__((used))
void *
_sbrk (int incr)
{
	static char * heap_end;
	char *        prev_heap_end;

	if (heap_end == NULL)
		heap_end = &__heap_base__;
    
	prev_heap_end = heap_end;
	heap_end += incr;
    
	return (void *) prev_heap_end;
}
