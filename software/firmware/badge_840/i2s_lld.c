/*-
 * Copyright (c) 2018
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

#include "hal.h"
#include "hal_i2s.h"
#include "i2s_lld.h"
#include "ff.h"

#include "badge.h"

volatile int i2sCnt;

uint16_t * i2sBuf;

static char * fname;
static thread_t * pThread = NULL;
static uint8_t play;
static uint8_t i2sloop;

static THD_WORKING_AREA(waI2sThread, 768);

static
THD_FUNCTION(i2sThread, arg)
{
        FIL f;
        UINT br;
        uint16_t * p;
        thread_t * th;
        char * file = NULL;
	int start;

	chRegSetThreadName ("I2S");

	while (1) {
		if (play == 0) {
			th = chMsgWait ();
			file = fname;
			play = 1;
			chMsgRelease (th, MSG_OK);
		}

		/*
		 * If the video app is running, then it's already using
		 * the DAC, and we shouldn't try to use it again here
		 * because can't play from two sources at once.
		 */
        
		if (i2sBuf != NULL) {
			play = 0;
			continue;
		}

		if (f_open (&f, file, FA_READ) != FR_OK) {
			play = 0;
			continue;
		}

		i2sBuf = chHeapAlloc (NULL, I2S_BYTES * 2);

		/* Load the first block of samples. */

		p = i2sBuf;
		start = 0;
		if (f_read (&f, p, I2S_BYTES, &br) != FR_OK) {
			f_close (&f);
			chHeapFree (i2sBuf);
			i2sBuf = NULL;
			play = 0;
			continue;
		}

		while (1) {

			/* Start the samples playing */

			if (start == 0) {
				i2sSamplesSend (p, I2S_SAMPLES / 2);
				start = 1;
			} else
				i2sSamplesNext (p, I2S_SAMPLES / 2);

			/* Swap buffers and load the next block of samples */

			if (p == i2sBuf)
        			p += I2S_SAMPLES;
			else
        			p = i2sBuf;

			if (f_read (&f, p, I2S_BYTES, &br) != FR_OK)
        			break;
			/*
			 * Wait until the current block of samples
			 * finishes playing before playing the next
			 * block.
			 */

			i2sSamplesWait ();

			/* If we read 0 bytes, we reached end of file. */

			if (br == 0)
				break;

			/* If told to stop, exit the loop. */

			if (play == 0)
        			break;
                }

                /* We're done, close the file. */

		i2sStop ();
                if (br == 0 && i2sloop == I2S_PLAY_ONCE) {
			file = NULL;
			play = 0;
                }
                chHeapFree (i2sBuf);
                i2sBuf = NULL;
                f_close (&f);
	}

	/* NOTREACHED */
	return;
}

void
i2sStart (void)
{
	NRF_I2S->CONFIG.TXEN = I2S_CONFIG_TXEN_TXEN_Msk;

	NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV10 <<
	    I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
	NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_256X <<
	    I2S_CONFIG_RATIO_RATIO_Pos;

	NRF_I2S->CONFIG.MODE = 0;

	/* 16bit sample width */

	NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16Bit <<
	    I2S_CONFIG_SWIDTH_SWIDTH_Pos;

	/* Alignment = Left */

	NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Left <<
	    I2S_CONFIG_ALIGN_ALIGN_Pos;

	/* Format = I2S */

	NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S <<
	    I2S_CONFIG_FORMAT_FORMAT_Pos;

	/* Use stereo */

	NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Left <<
	    I2S_CONFIG_CHANNELS_CHANNELS_Pos;

	/* Configure pins */

	NRF_I2S->PSEL.SDOUT = 0x20 | IOPORT2_I2S_SDOUT;
	NRF_I2S->PSEL.SCK = 0x20 | IOPORT2_I2S_SCK;
	NRF_I2S->PSEL.LRCK = 0x20 | IOPORT2_I2S_LRCK;
	NRF_I2S->PSEL.MCK = 0x20 | IOPORT2_I2S_MCK;

	nvicEnableVector (I2S_IRQn, 5);

	NRF_I2S->ENABLE = 1;

	pThread = chThdCreateStatic (waI2sThread, sizeof(waI2sThread),
	    I2S_THREAD_PRIO, i2sThread, NULL);

	/*
	 * According to the CS4344 manual, we need to have MCK
	 * running for a certain period of time before the
	 * chip will start decoding audio samples. We want samples
	 * to play right away, so we need to keep the MCK and
	 * LRCK going and gate the SCK to start/stop the playing
	 * of audio. When we first start the I2S controller, we
	 * start the MCK and LRCK up and keep the SCK pin isolated
	 * so that we can give the CS4344 some cycles to allow it
	 * to ramp up to the point where it's ready to start
	 * playing.
	 */

	NRF_I2S->INTENSET = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;
	NRF_I2S->EVENTS_TXPTRUPD = 0;
	NRF_I2S->TXD.PTR = 0x0;
	NRF_I2S->RXTXD.MAXCNT = 32768;
	NRF_I2S->TASKS_START = 1;
	i2sSamplesWait ();

	return;
}

OSAL_IRQ_HANDLER(VectorD4)
{
	OSAL_IRQ_PROLOGUE();
	NRF_I2S->INTENCLR = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;
	NRF_I2S->EVENTS_TXPTRUPD = 0;
	NRF_I2S->RXTXD.MAXCNT = 0;
	NRF_I2S->TXD.PTR = 0;
	OSAL_IRQ_EPILOGUE();
	return;
}

void
i2sSamplesSend (void * buf, int cnt)
{
	NRF_I2S->PSEL.SCK = 0x20 | IOPORT2_I2S_SCK;
	NRF_I2S->TXD.PTR = (uint32_t)buf;
	NRF_I2S->RXTXD.MAXCNT = cnt;
	NRF_I2S->EVENTS_TXPTRUPD = 0;
	NRF_I2S->INTENSET = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;

	return;
}

void
i2sSamplesNext (void * buf, int cnt)
{
	NRF_I2S->TXD.PTR = (uint32_t)buf;
	NRF_I2S->RXTXD.MAXCNT = cnt;
	NRF_I2S->INTENSET = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;

	return;
}

void
i2sSamplesWait (void)
{
	while (NRF_I2S->EVENTS_TXPTRUPD == 0)
		chThdSleep (1);
	return;
}

void
i2sStop (void)
{
	NRF_I2S->PSEL.SCK = 0xFFFFFFFF;
	return;
}

void
i2sPlay (char * filename)
{
	play = 0;
	i2sloop = I2S_PLAY_ONCE;
	fname = filename;
	chMsgSend (pThread, MSG_OK);

	return;
}

#ifdef notdef
uint16_t sbuf1[128];
uint16_t sbuf2[128];

void
i2sTest (void)
{
	FIL f;
	UINT br;
	uint16_t * p;
	int i;
	uint32_t * blah;
	int start = 0;

	if (f_open (&f, "0:DRWHO.RAW", FA_READ) != FR_OK)
		return;

	p = sbuf1;
	f_read (&f, p, sizeof(sbuf1), &br);
	i2sSamplesSend (p, 64);

	while (1) {
		if (p == sbuf1)
			p = sbuf2;
		else
			p = sbuf1;
		f_read (&f, p, sizeof(sbuf1), &br);
		if (br == 0)
			break;
		i2sSamplesWait ();
		i2sSamplesNext (p, 64);
	}

	i2sStop ();
	f_close (&f);
}
#endif
