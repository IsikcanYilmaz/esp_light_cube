#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "animation_manager.h"
#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "periph_cpu.h"
#include "timex.h"
#include "ztimer.h"
#include "xtimer.h"
#include "shell.h"

#include "addr_led_driver.h"
#include "usr_commands.h"

#include "periph/pwm.h"

#include "ws281x.h"

#include "thread.h"

#include "dma_rmt_test.h"

#include "mic.h"
#include "logger.h"

extern char line_buf[SHELL_BUFFER_SIZE];

char blink_threadStack[THREAD_STACKSIZE_DEFAULT];
char mictest_threadStack[THREAD_STACKSIZE_DEFAULT];

bool on = false;

void *blink_threadHandler(void *arg)
{
	(void) arg;
	thread_t *thisThread = thread_get_active();
	const char *thisThreadName = thread_get_name(thisThread);
	while (1) {
		if (on)
		{
			LED0_ON;
		}
		else
		{
			LED0_OFF;
		}
		on = !on;
		ztimer_sleep(ZTIMER_USEC, 1 * US_PER_SEC);
	}
}

void *mictest_threadHandler(void *arg)
{
	(void) arg;
	Mic_Init();
	while(true)
	{
		Mic_Test();
		ztimer_sleep(ZTIMER_MSEC, 200);
	}
}

int main(void)
{
	// ztimer_sleep(ZTIMER_USEC, 5 * US_PER_SEC);
	logprint("Light Cube RIOT\n");

	ztimer_sleep(ZTIMER_USEC, 4 * US_PER_SEC); 
	srand(time(NULL));
	// dma_rmt_test();
	AddrLedDriver_Init();
	AnimationMan_Init();
	// AddrLedDriver_Test();

	kernel_pid_t blink_threadId = thread_create(
		blink_threadStack,
		sizeof(blink_threadStack),
		THREAD_PRIORITY_MAIN - 1,
		THREAD_CREATE_STACKTEST,
		blink_threadHandler,
		NULL,
		"blink_thread"
	);

	// kernel_pid_t mictest_threadId = thread_create(
	// 	mictest_threadStack,
	// 	sizeof(mictest_threadStack),
	// 	THREAD_PRIORITY_MAIN - 1,
	// 	THREAD_CREATE_STACKTEST,
	// 	mictest_threadHandler,
	// 	NULL,
	// 	"mictest_thread"
	// );
	
	UserCommand_Init(); // inf loop

	for (;;) 
	{
	}

	return 0;
}
