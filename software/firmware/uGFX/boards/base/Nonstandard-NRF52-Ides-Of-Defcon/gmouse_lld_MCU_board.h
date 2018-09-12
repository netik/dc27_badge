/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _LLD_GMOUSE_MCU_BOARD_H
#define _LLD_GMOUSE_MCU_BOARD_H

#include "xpt2046_lld.h"
#include "xpt2046_reg.h"

#ifdef notyet
#include "userconfig.h"
#endif

#include <string.h>

#define EASTRISING

#ifdef MCUFRIEND
static const float calibrationData[] = {
	-0.06661,		// ax
	0.00031,		// bx
	260.26651,		// cx
	-0.00079,		// ay
	0.08950,		// by
	-13.79760		// cy
};
#endif

#ifdef EASTRISING
static const float calibrationData[] = {
	0.06396,		// ax
	-0.00003,		// bx
	-9.35300,		// cx
	0.00024,		// ay
	0.09009,		// by
	-13.67999		// cy
};
#endif
 
bool_t LoadMouseCalibration(unsigned instance, void *data, size_t sz)
{
#ifdef notyet
	const userconfig * config;
#endif
	(void)data;

#ifdef notdef
	config = getConfig();
#endif

	if (sz != sizeof(calibrationData) || instance != 0) {
		return FALSE;
	}

#ifdef notyet
	if (config->touch_data_present)
		memcpy (data, (void*)&config->touch_data, sz);
	else
#endif
		memcpy (data, (void*)&calibrationData, sz);

	return (TRUE);
}

// Resolution and Accuracy Settings
#define GMOUSE_MCU_PEN_CALIBRATE_ERROR		14
#define GMOUSE_MCU_PEN_CLICK_ERROR		1
#define GMOUSE_MCU_PEN_MOVE_ERROR		1
#define GMOUSE_MCU_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_MCU_FINGER_CLICK_ERROR		18
#define GMOUSE_MCU_FINGER_MOVE_ERROR		14
#define GMOUSE_MCU_Z_MIN		0	/* The minimum Z reading */
#define GMOUSE_MCU_Z_MAX		4096	/* The maximum Z reading */

#define GMOUSE_MCU_Z_TOUCHON		300	/*
						 * Values between this and
						 * Z_MAX are definitely pressed
						 */

#define GMOUSE_MCU_Z_TOUCHOFF		100	/*
						 * Values between this and
						 * Z_MIN are definitely not
						 * pressed
						 */

/*
 * How much extra data to allocate at the end of the GMouse
 * structure for the board's use
 */

#define GMOUSE_MCU_BOARD_DATA_SIZE	0

static bool_t init_board(GMouse *m, unsigned driverinstance) {
	(void)m;
	(void)driverinstance;
	return (TRUE);
}

static bool_t read_xyz(GMouse *m, GMouseReading *prd) {
	uint16_t z1;
	uint16_t z2;

	(void)m;

	prd->x = xptGet (XPT_CHAN_X);
	prd->y = xptGet (XPT_CHAN_Y);

	z1 = xptGet (XPT_CHAN_Z1);
	z2 = xptGet (XPT_CHAN_Z2);

	if ((z2 - z1) < XPT_TOUCH_THRESHOLD)
		prd->z = 4095 - (z2 - z1);
	else
		prd->z = 0;

	return (TRUE);
}

#endif /* _LLD_GMOUSE_MCU_BOARD_H */
