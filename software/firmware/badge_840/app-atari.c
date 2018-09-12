#include "orchard-app.h"
#include "orchard-ui.h"

#include "ble_lld.h"

extern void atariRun (void);

static uint32_t
atari_init (OrchardAppContext *context)
{
	(void)context;

	/*
	 * We don't want any extra stack space allocated for us.
	 * We'll use the heap.
	 */

	return (0);
}

static void
atari_start (OrchardAppContext *context)
{
	gdispClear (Black);
	bleDisable ();
	atariRun ();
	return;
}

static void
atari_event (OrchardAppContext *context,
	const OrchardAppEvent *event)
{
	return;
}

static void
atari_exit (OrchardAppContext *context)
{
	(void)context;

	bleEnable ();
	return;
}

orchard_app("Atari", "setup.rgb",
    0, atari_init, atari_start, atari_event, atari_exit, 1);
