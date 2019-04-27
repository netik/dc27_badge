#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "orchard-app.h"
#include "orchard-ui.h"

#include "src/gdriver/gdriver.h"
#include "src/ginput/ginput_driver_mouse.h"

#include "userconfig.h"

static uint32_t
calibrate_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
calibrate_start (OrchardAppContext *context)
{
	GMouse * m;
	userconfig *config;

	(void)context;

	/* Run the calibration GUI */
	(void)ginputCalibrateMouse (0);

	/* Save the calibration data */
	m = (GMouse*)gdriverGetInstance (GDRIVER_TYPE_MOUSE, 0);

	config = getConfig();
	memcpy (&config->touch_data, &m->caldata, sizeof(config->touch_data));
	config->touch_data_present = 1;
	configSave (config);

	orchardAppExit ();

	return;
}

static void
calibrate_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	(void)context;
	(void)event;

	return;
}

static void
calibrate_exit (OrchardAppContext *context)
{
	(void)context;
	return;
}

orchard_app("Touch Cal", "icons/starfish.rgb",
    0, calibrate_init, calibrate_start, calibrate_event, calibrate_exit, 1);
