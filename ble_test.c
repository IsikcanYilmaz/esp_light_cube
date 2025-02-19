#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ble_test.h"
#include "logger.h"

#include "timex.h"
#include "ztimer.h"
#include "shell.h"

#include "nimble_scanner.h"
#include "nimble_scanlist.h"

// #include "nimble_riot.h"
// #include "nimble_autoadv.h"
// #include "host/ble_hs.h"
// #include "host/util/util.h"
// #include "host/ble_gatt.h"
// #include "services/gap/ble_svc_gap.h"
// #include "services/gatt/ble_svc_gatt.h"

#include "usr_commands.h"

#include <stdbool.h>

/* default scan interval */
#define DEFAULT_SCAN_INTERVAL_MS    30

/* default scan duration (1s) */
#define DEFAULT_DURATION_MS        (1 * MS_PER_SEC)

bool bleInitialized = false;

int Ble_Scan(uint32_t timeout)
{
	nimble_scanlist_clear();
	printf("Scanning for %"PRIu32" ms now ...\n", timeout);
	nimble_scanner_start();
	ztimer_sleep(ZTIMER_MSEC, timeout);
	nimble_scanner_stop();
	puts("Done, results:");
	nimble_scanlist_print();
	puts("");

	return 0;
}

int Ble_TakeUsrCommand(int argc, char **argv)
{
	if (!bleInitialized)
	{
		logprint("Nimble not initialized!\n");
		return 1;
	}
	ASSERT_ARGS(2);

	if (strcmp(argv[1], "scan") == 0)
	{
		uint32_t scanTime = DEFAULT_SCAN_INTERVAL_MS;
		if (argc == 3)
		{
			scanTime = atoi(argv[2]);
		}
		Ble_Scan(scanTime);
	}
	return 0;
}

int Ble_Init(void)
{
	logprint("NimBLE Scanner Example Application\n");
	logprint("Type `scan help` for more information\n");

	/* in this example, we want Nimble to scan 'full time', so we set the
		 * window equal the interval */
	nimble_scanner_cfg_t params = {
		.itvl_ms = DEFAULT_SCAN_INTERVAL_MS,
		.win_ms = DEFAULT_SCAN_INTERVAL_MS,
		#if IS_USED(MODULE_NIMBLE_PHY_CODED)
		.flags = NIMBLE_SCANNER_PHY_1M | NIMBLE_SCANNER_PHY_CODED,
		#else
		.flags = NIMBLE_SCANNER_PHY_1M,
		#endif
	};

	/* initialize the nimble scanner */
	nimble_scanlist_init();
	nimble_scanner_init(&params, nimble_scanlist_update);

	bleInitialized = true;

	return 0;
}
