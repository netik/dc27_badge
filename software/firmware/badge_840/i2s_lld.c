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

uint16_t * i2sBuf;
uint8_t i2sEnabled = TRUE;

static char * fname;
static thread_t * pThread = NULL;
static volatile uint8_t play;
static uint8_t i2sloop;

static THD_WORKING_AREA(waI2sThread, 1024);
static thread_reference_t i2sThreadReference;
static int i2sState;
static int i2sRunning;

static
THD_FUNCTION(i2sThread, arg)
{
        FIL f;
        UINT br;
        uint16_t * p;
        thread_t * th;
        char * file = NULL;

	chRegSetThreadName ("I2S");

	while (1) {
		if (play == 0) {
			th = chMsgWait ();
			file = fname;
			play = 1;
			chMsgRelease (th, MSG_OK);
		}

		if (i2sEnabled == FALSE) {
			play = 0;
			file = NULL;
			continue;
		}

		/*
		 * If the video app is running, then it's already using
		 * the I2S channel, and we shouldn't try to use it again
		 * here because we can't play from two sources at once.
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
		if (f_read (&f, p, I2S_BYTES, &br) != FR_OK) {
			f_close (&f);
			chHeapFree (i2sBuf);
			i2sBuf = NULL;
			play = 0;
			continue;
		}

		/* Power up the audio amp */

		palClearPad (IOPORT1, IOPORT1_I2S_AMPSD);

		while (1) {

			/* Start the samples playing */

			i2sSamplesPlay (p, br >> 1);

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

		i2sSamplesStop ();
		if (br == 0 && i2sloop == I2S_PLAY_ONCE) {
			file = NULL;
			play = 0;
		}

		/* Power down the audio amp */

		palSetPad (IOPORT1, IOPORT1_I2S_AMPSD);

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

	/*
	 * Select master clock and Left/Right clock frequencies.
	 * The I2S peripheral is fed with a 32MHz bus clock. We can
	 * choose various divisors for it. From that, we can then
	 * derive the Left/Right clock by selecting a further divisor
	 * ratio.
	 *
	 * We want a sample rate of approximately 16KHz. Using the 32MDIV8
	 * gives us a master clock of 4MHz. And a L/R divisor of 256
	 * gives us 15.625MHz. This is close enough.
	 */

	NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV8 <<
	    I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
	NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_256X <<
	    I2S_CONFIG_RATIO_RATIO_Pos;

	/* Select master mode. */

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

	/* John wants stereo so we do stereo. :) */

	NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Stereo <<
	    I2S_CONFIG_CHANNELS_CHANNELS_Pos;

	/* Configure pins */

	NRF_I2S->PSEL.SDOUT = 0x20 | IOPORT2_I2S_SDOUT;
	NRF_I2S->PSEL.LRCK = 0x20 | IOPORT2_I2S_LRCK;
	NRF_I2S->PSEL.MCK = 0x20 | IOPORT2_I2S_MCK;

	/* Configure NVIC to enable the I2S interrupt vector. */

	nvicEnableVector (I2S_IRQn, NRF5_I2S_IRQ_PRIORITY);

	/* Also enable one of the software interrupts */

	nvicEnableVector (SWI0_EGU0_IRQn, NRF5_I2S_IRQ_PRIORITY);

	/*
	 * We would like to be able to have the i2sSamplesWait()
	 * routine put the thread to sleep and only wake up when
	 * the current batch of samples finishes playing. For that
	 * we need to have an interrupt triggered when the I2S
	 * controller sends the last sample out onto the wire to
	 * the codec. Unfortunately it doesn't do that. The
	 * EVENTS_TXPTRUPD register does change state when the
	 * transmission of samples is done, but the I2S interrupt
	 * occurs before that.
	 *
	 * We could just poll the EVENTS_TXPTRUPD register in a
	 * loop, but we'd need to sleep between checks in order not
	 * to hog the CPU, and we are limited by the system tick
	 * granularity in how long we can sleep. If the system tick
	 * frequency is too low, we'll sleep too long, and if it's
	 * too high, we'll generate too many extra interrupts.
	 *
	 * To get around this problem, we take advantage of the
	 * Nordic PPI block and the software event generator unit.
	 * We program one of the PPI blocks to tie the
	 * EVENTS_TXPTRUPD event to one of the software interrupt
	 * channels. When the EVENTS_TXPTRUPD register changes state,
	 * the PPI will trigger a software interrupt. We can then
	 * use that to wake the thread.
	 */

	/* Enable one of the event generator channels */

	NRF_EGU0->INTENSET = 1 << I2S_EGU_TASK;

	/* Enable one of the PPI channels */

	NRF_PPI->CH[I2S_PPI_CHAN].EEP =
	    (uint32_t)&NRF_I2S->EVENTS_TXPTRUPD;
	NRF_PPI->CH[I2S_PPI_CHAN].TEP =
	    (uint32_t)&NRF_EGU0->TASKS_TRIGGER[I2S_EGU_TASK];

	/* Turn on the I2S peripheral and start the clocks . */

	NRF_I2S->ENABLE = 1;
	NRF_I2S->TASKS_START = 1;

	/* Launch the player thread. */

	pThread = chThdCreateStatic (waI2sThread, sizeof(waI2sThread),
	    I2S_THREAD_PRIO, i2sThread, NULL);

	/* Put audio amp in shutdown mode */

	palSetPad (IOPORT1, IOPORT1_I2S_AMPSD);

	/*
	 * According to the CS4344 manual, we need to have MCK
	 * running for a certain period of time before the chip
	 * will start decoding audio samples. We want samples to
	 * play right away though, so we need to keep the MCK and
	 * LRCK going and gate the SCK to start/stop the playing
	 * of audio. When we first start the I2S controller, we
	 * start the MCK and LRCK up and keep the SCK pin isolated
	 * so that we can give the CS4344 some cycles to allow it
	 * to ramp up to the point where it's ready to start
	 * playing.
	 */

	i2sRunning = 1;
	i2sSamplesPlay (NULL, 32768);
	i2sSamplesWait ();
	i2sSamplesStop ();

	return;
}

OSAL_IRQ_HANDLER(Vector90)
{
	OSAL_IRQ_PROLOGUE();
	if (NRF_EGU0->EVENTS_TRIGGERED[I2S_EGU_TASK]) {
		osalSysLockFromISR ();

		/* Turn off the PPI channel and ack the interrupt */

		NRF_PPI->CHENCLR = 1 << I2S_PPI_CHAN;
		NRF_EGU0->EVENTS_TRIGGERED[I2S_EGU_TASK] = 0;
		(void)NRF_EGU0->EVENTS_TRIGGERED[I2S_EGU_TASK];

		/* Wake the sleeping thread */

		i2sState = I2S_STATE_IDLE;
		osalThreadResumeI (&i2sThreadReference, MSG_OK);
		osalSysUnlockFromISR (); 
	}
	OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(VectorD4)
{
	OSAL_IRQ_PROLOGUE();
	NRF_I2S->INTENCLR = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;
	NRF_I2S->RXTXD.MAXCNT = 0;
	/*
	 * Enable PPI channel -- should trigger when TXPTRUPD
	 * event changes state.
	 */
	NRF_PPI->CHENSET = 1 << I2S_PPI_CHAN;
	OSAL_IRQ_EPILOGUE();
	return;
}

/******************************************************************************
*
* i2sSamplesPlay - play specific sample buffer
*
* This function can be used to play an arbitrary buffer of samples provided
* by the caller via the pointer argument <p>. The <cnt> argument indicates
* the number of samples (which should be equal to the total number of bytes
* in the buffer divided by 2, since samples are stored as 16-bit quantities).
*
* This function is used primarily by the video player module.
*
* RETURNS: N/A
*/

void
i2sSamplesPlay (void * buf, int cnt)
{
	osalSysLock ();
	i2sState = I2S_STATE_BUSY;

	if (i2sRunning == 0) {
		NRF_I2S->PSEL.SCK = 0x20 | IOPORT2_I2S_SCK;
		i2sRunning = 1;
	}

	NRF_I2S->TXD.PTR = (uint32_t)buf;
	NRF_I2S->RXTXD.MAXCNT = cnt >> 1;
	NRF_I2S->INTENSET = I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk;
	osalSysUnlock ();

	return;
}

/******************************************************************************
*
* i2sSamplesWait - wait for audio sample buffer to finish playing
*
* This function can be used to test when the current block of samples has
* finished playing. It is used primarily by callers of i2sSamplesPlay() to
* keep pace with the audio playback. The function sleeps until an interrupt
* occurs indicating that the current batch of samples has been sent to
* the codec chip.
*
* RETURNS: N/A
*/

void
i2sSamplesWait (void)
{
	osalSysLock ();

	/* If we've finished playing samples already, just return. */

	if (i2sState == I2S_STATE_IDLE) {
		osalSysUnlock ();
		return;
	}

	/* Sleep until the current batch of samples has been sent. */

	osalThreadSuspendS (&i2sThreadReference);
	osalSysUnlock ();

	return;
}

/******************************************************************************
*
* i2sSamplesStop - stop sample playback
*
* This function must be called after samples are sent to the codec using
* i2sSamplesPlay() and there are no more samples left to send. The nature
* of the CS4344 codec is such that we can't just stop the I2S controller
* because that cuts off the clock signals to it, and it will take time for
* it to re-synchronize when the clocks are started again. Instead we gate
* the SCK signal which stops it from trying to decode any audio data.
*
* RETURNS: N/A
*/

void
i2sSamplesStop (void)
{
	osalSysLock ();
	i2sRunning = 0;
	NRF_I2S->PSEL.SCK = 0xFFFFFFFF;
	osalSysUnlock ();
	return;
}

/******************************************************************************
*
* i2sWait - wait for current audio file to finish playing
*
* This function can be used to test when the current audio sample file has
* finished playing. It can be used to pause the current thread until a
* sound effect finishes playing.
*
* RETURNS: The number of ticks we had to wait until the playback finished.
*/

int
i2sWait (void)
{
	int waits = 0;

	while (play != 0) {
		chThdSleep (1);
		waits++;
	}

	return (waits);
}

/******************************************************************************
*
* i2sPlay - play an audio file
*
* This is a shortcut version of i2sLoopPlay() that always plays a file just
* once. As with i2sLoopPlay(), playback can be halted by calling i2sPlay()
* with <file> set to NULL.
*
* RETURNS: N/A
*/

void
i2sPlay (char * file)
{
	i2sLoopPlay (file, I2S_PLAY_ONCE);
	return;
}

/******************************************************************************
*
* i2sLoopPlay - play an audio file
*
* This function cues up an audio file to be played by the background I2S
* thread. The file name is specified by <file>. If <loop> is I2S_PLAY_ONCE,
* the sample file is played once and then the player thread will go idle
* again. If <loop> is I2S_PLAY_LOOP, the same file will be played over and
* over again.
*
* Playback of any sample file (including one that is looping) can be halted
* at any time by calling i2sLoopPlay() with <file> set to NULL.
*
* RETURNS: N/A
*/

void
i2sLoopPlay (char * file, uint8_t loop)
{
	play = 0;
	i2sloop = loop;
	fname = file;
	chMsgSend (pThread, MSG_OK);

	return;
}

void
i2sAudioAmpCtl (uint8_t ctl)
{
	switch (ctl) {
		case I2S_AMP_OFF:
			palSetPad (IOPORT1, IOPORT1_I2S_AMPSD);
			break;
		case I2S_AMP_ON:
			palClearPad (IOPORT1, IOPORT1_I2S_AMPSD);
			break;
		default:
			break;
	}

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

	if (f_open (&f, "0:DRWHO.SND", FA_READ) != FR_OK)
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

	i2sSamplesStop ();
	f_close (&f);
}
#endif
