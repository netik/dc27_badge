/*-
 * Copyright (c) 2017
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

#ifndef _VIDEO_LLD_H_
#define _VIDEO_LLD_H_

#define VID_FRAMES_PER_SEC		12

#define VID_PIXELS_PER_LINE		160
#define VID_LINES_PER_FRAME		120

#define VID_CHUNK_LINES			8

#define VID_AUDIO_SAMPLES_PER_SCANLINE	8

#define VID_AUDIO_CHANNELS		1

#define VID_AUDIO_SAMPLE_RATE		12500


#define VID_AUDIO_SAMPLES_PER_CHUNK			\
	(VID_AUDIO_SAMPLES_PER_SCANLINE * VID_CHUNK_LINES)

#define VID_AUDIO_BYTES_PER_CHUNK			\
	(VID_AUDIO_SAMPLES_PER_CHUNK * 2)

#define VID_CHUNK_BYTES					\
	((VID_PIXELS_PER_LINE * VID_CHUNK_LINES * 2) +	\
	VID_AUDIO_BYTES_PER_CHUNK)

#define VID_CHUNK_PIXELS				\
	((VID_PIXELS_PER_LINE * VID_CHUNK_LINES) +	\
	VID_AUDIO_SAMPLES_PER_CHUNK)

#define VID_TIMER_RESOLUTION	NRF5_GPT_FREQ_16MHZ

#define VID_TIMER_INTERVAL				\
	((NRF5_GPT_FREQ_16MHZ * VID_CHUNK_LINES) /	\
	(VID_LINES_PER_FRAME * VID_FRAMES_PER_SEC))

extern int videoPlay (char *);

#endif /* _VIDEO_LLD_H_ */
