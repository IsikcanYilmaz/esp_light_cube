#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "animation_manager.h"
#include "animation_scroller.h"
#include "animation_oscillator.h"
#include "animation_canvas.h"
#include "animation_lines.h"
#include "animation_snakes.h"
#include "animation_sparkles.h"
#include "animation_game_of_life.h"
#include "animation_walker.h"
#include "addr_led_driver.h"
#include "editable_value.h"
#include "logger.h"
#include <string.h>
#include "usr_commands.h"

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "periph_cpu.h"
#include "timex.h"
#include "ztimer.h"
#include "xtimer.h"
#include "periph/pwm.h"
#include "ws281x.h"
#include "thread.h"

// #include "colorspace_interface.h"

char animationMan_threadStack[THREAD_STACKSIZE_DEFAULT];
Animation_s *currentAnimation;
AnimationManState_e animationManState = ANIMATION_MAN_STATE_RUNNING;
AnimationIdx_e currentAnimationIdx;
AnimationIdx_e targetAnimationIdx; // for when we're waiting for the switching of another animation
// TODO could have animations time out as a failsafe?
static bool animationManInitialized = false;

static uint32_t autoSwitchTimestampMs = 0;

static uint32_t framePerSecond = ANIMATION_MANAGER_DEFAULT_FPS;
static uint32_t framePeriodUs = (US_PER_SEC/ANIMATION_MANAGER_DEFAULT_FPS);

static uint32_t autoSwitchMs = 1*60*1000;
static bool autoSwitchEnabled = false;

static double fadeOutV = 1.0;

static EditableValue_t editableValues[] =
{
	(EditableValue_t) {.name = "autoSwitchMs", .valPtr = (union EightByteData_u *) &autoSwitchMs, .type = UINT32_T, .ll.b = 1000, .ul.b = 0xffffffff},
	(EditableValue_t) {.name = "autoSwitchEnabled", .valPtr = (union EightByteData_u *) &autoSwitchEnabled, .type = BOOLEAN, .ll.b = false, .ul.b = true},
};
static EditableValueList_t editableValueList = {.name = "animationmanager", .values = &editableValues[0], .len=sizeof(editableValues)/sizeof(EditableValue_t)};

Animation_s animations[ANIMATION_MAX] = {
	[ANIMATION_SCROLLER] = {
		.name = "scroller",
		.init = AnimationScroller_Init,
		.deinit = AnimationScroller_Deinit,
		.start = AnimationScroller_Start,
		.stop = AnimationScroller_Stop,
		.update = AnimationScroller_Update,
		.buttonInput = AnimationScroller_ButtonInput,
		.usrInput = AnimationScroller_UsrInput,
		.signal = AnimationScroller_ReceiveSignal,
		.getState = AnimationScroller_GetState
	},
	[ANIMATION_SPARKLES] = {
		.name = "sparkles",
		.init = AnimationSparkles_Init,
		.deinit = AnimationSparkles_Deinit,
		.start = AnimationSparkles_Start,
		.stop = AnimationSparkles_Stop,
		.update = AnimationSparkles_Update,
		.buttonInput = AnimationSparkles_ButtonInput,
		.usrInput = AnimationSparkles_UsrInput,
		.signal = AnimationSparkles_ReceiveSignal,
		.getState = AnimationSparkles_GetState
	},
	[ANIMATION_CANVAS] = {
		.name = "canvas",
		.init = AnimationCanvas_Init,
		.deinit = AnimationCanvas_Deinit,
		.start = AnimationCanvas_Start,
		.stop = AnimationCanvas_Stop,
		.update = AnimationCanvas_Update,
		.buttonInput = AnimationCanvas_ButtonInput,
		.usrInput = AnimationCanvas_UsrInput,
		.signal = AnimationCanvas_ReceiveSignal,
		.getState = AnimationCanvas_GetState
	},
	[ANIMATION_LINES] = {
		.name = "lines",
		.init = AnimationLines_Init,
		.deinit = AnimationLines_Deinit,
		.start = AnimationLines_Start,
		.stop = AnimationLines_Stop,
		.update = AnimationLines_Update,
		.buttonInput = AnimationLines_ButtonInput,
		.usrInput = AnimationLines_UsrInput,
		.signal = AnimationLines_ReceiveSignal,
		.getState = AnimationLines_GetState
	},
	[ANIMATION_OSCILLATOR] = {
		.name = "oscillator",
		.init = AnimationOscillator_Init,
		.deinit = AnimationOscillator_Deinit,
		.start = AnimationOscillator_Start,
		.stop = AnimationOscillator_Stop,
		.update = AnimationOscillator_Update,
		.buttonInput = AnimationOscillator_ButtonInput,
		.usrInput = AnimationOscillator_UsrInput,
		.signal = AnimationOscillator_ReceiveSignal,
		.getState = AnimationOscillator_GetState
	},
	[ANIMATION_SNAKES] = {
		.name = "snakes",
		.init = AnimationSnakes_Init,
		.deinit = AnimationSnakes_Deinit,
		.start = AnimationSnakes_Start,
		.stop = AnimationSnakes_Stop,
		.update = AnimationSnakes_Update,
		.buttonInput = AnimationSnakes_ButtonInput,
		.usrInput = AnimationSnakes_UsrInput,
		.signal = AnimationSnakes_ReceiveSignal,
		.getState = AnimationSnakes_GetState
	},
	[ANIMATION_GAME_OF_LIFE] = {
		.name = "gameoflife",
		.init = AnimationGameOfLife_Init,
		.deinit = AnimationGameOfLife_Deinit,
		.start = AnimationGameOfLife_Start,
		.stop = AnimationGameOfLife_Stop,
		.update = AnimationGameOfLife_Update,
		.buttonInput = AnimationGameOfLife_ButtonInput,
		.usrInput = AnimationGameOfLife_UsrInput,
		.signal = AnimationGameOfLife_ReceiveSignal,
		.getState = AnimationGameOfLife_GetState
	},
	[ANIMATION_WALKER] = {
		.name = "walker",
		.init = AnimationWalker_Init,
		.deinit = AnimationWalker_Deinit,
		.start = AnimationWalker_Start,
		.stop = AnimationWalker_Stop,
		.update = AnimationWalker_Update,
		.buttonInput = AnimationWalker_ButtonInput,
		.usrInput = AnimationWalker_UsrInput,
		.signal = AnimationWalker_ReceiveSignal,
		.getState = AnimationWalker_GetState
	},
};

static Animation_s * AnimationMan_GetAnimationByIdx(AnimationIdx_e idx)
{
	if (idx >= ANIMATION_MAX)
	{
		logprint("Bad anim idx %d at %s\n", idx, __FUNCTION__);
		return NULL;
	}
	return &animations[idx];
  // return NULL;
}

static void AnimationMan_PlayNextAnimation(void)
{
	AnimationMan_SetAnimation((currentAnimationIdx + 1) % ANIMATION_CANVAS, false);
}

static void AnimationMan_HandleAutoSwitch(void)
{
	if (!autoSwitchEnabled)
	{
		return;
	}
	else if (currentAnimationIdx != ANIMATION_CANVAS)
	{
		uint32_t now = ztimer_now(ZTIMER_USEC)/1000;
		if (ztimer_now(ZTIMER_USEC)/1000 - autoSwitchTimestampMs > autoSwitchMs) // Time to switch animations
		{
			logprint("Auto anim switch at %d. %d %d\n", now, autoSwitchTimestampMs, autoSwitchMs);
			AnimationMan_PlayNextAnimation();
			autoSwitchTimestampMs = now;
		}
	}
}

static void AnimationMan_SetFps(uint32_t fps)
{
	framePerSecond = fps;
	framePeriodUs = US_PER_SEC/fps;
	logprint("New fps %d, period %d\n", fps, framePeriodUs);
}

static uint32_t AnimationMan_GetFps(void)
{
	return framePerSecond;
}

void *AnimationMan_ThreadHandler(void *arg)
{
	(void) arg;
	thread_t *thisThread = thread_get_active();
	const char *thisThreadName = thread_get_name(thisThread);
	// Main thread for animations. What it should be doing is:
	// Go through list of interruptions
	// Interact with the currently running animation
	// Let currentlyl running animation do its pixel settings
	// Display current state of the strip/s 
	while (true)
	{
		// Handle auto switching first
		AnimationMan_HandleAutoSwitch();

		// Main animation switch/case
		uint32_t t0 = ztimer_now(ZTIMER_USEC);
		switch(animationManState)
		{
			case ANIMATION_MAN_STATE_RUNNING:
				{
					currentAnimation->update();
					if (AddrLedDriver_ShouldRedraw()) 
					{
						AddrLedDriver_DisplayCube();
					}
					break;
			}
			case ANIMATION_MAN_STATE_SWITCHING:
				{
					if (currentAnimation->getState() == ANIMATION_STATE_STOPPED)
					{
						logprint("Animation faded off. Starting next animation\n");
						AnimationMan_SetAnimation(targetAnimationIdx, true);
					}
					else
					{
						currentAnimation->update();
						if (AddrLedDriver_ShouldRedraw()) 
						{
							AddrLedDriver_DisplayCube();
						}
					}
					break;
			}
			default:
				{
					logprint("%s state invalid or not implemented yet %d\n", __FUNCTION__, animationManState);
					animationManState = ANIMATION_MAN_STATE_RUNNING; // TODO placeholder. eventually implement the stopped state. will need for temperature or deep sleep reasons? 
					break;
			}
		}
		uint32_t t1 = ztimer_now(ZTIMER_USEC);
		uint32_t processingLatencyUs = (t1-t0);
		uint32_t sleepUntilNextFrames = framePeriodUs - processingLatencyUs;
		// logprint("proclat %d, sleep %d, calcperiod %d, netcalcperiod %d\n", processingLatencyUs, (uint32_t)(0.05 * US_PER_SEC), framePeriodUs, sleepUntilNextFrames);
		ztimer_sleep(ZTIMER_USEC, sleepUntilNextFrames);
	}
}

void AnimationMan_Init(void)
{
	if (!AddrLedDriver_IsInitialized())
	{
		logprint("%s addr led driver not initialized!\n", __FUNCTION__);
		return;
	}

	currentAnimation = AnimationMan_GetAnimationByIdx(ANIMATION_DEFAULT);
	currentAnimationIdx = ANIMATION_DEFAULT;
	currentAnimation->init(NULL);

	kernel_pid_t animationMan_threadId = thread_create(
		animationMan_threadStack,
		sizeof(animationMan_threadStack),
		0,
		THREAD_CREATE_STACKTEST,
		AnimationMan_ThreadHandler,
		NULL,
		"animationman_thread"
	);

	animationManInitialized = true;
}

void AnimationMan_SetAnimation(AnimationIdx_e anim, bool immediately)
{
	if (anim >= ANIMATION_MAX)
	{
		logprint("Bad anim idx %d to %s\n", anim, __FUNCTION__);
		return;
	}

	// TODO make this so that this sends a signal to the running animation which then does its cleanup and fades off
	// for now, it abruptly changes
	targetAnimationIdx = anim;
	if (immediately)
	{
		// Special case; if CANVAS was just on, wait full wait time to do a auto switch once we get out of it
		if (currentAnimationIdx == ANIMATION_CANVAS && autoSwitchEnabled)
		{
			autoSwitchTimestampMs = ztimer_now(ZTIMER_MSEC);
		}

		currentAnimation->deinit();
		AddrLedDriver_Clear();
		currentAnimation = &animations[targetAnimationIdx];
		currentAnimationIdx = anim;
		currentAnimation->init(NULL);
		animationManState = ANIMATION_MAN_STATE_RUNNING;
	}
	else
	{
		currentAnimation->signal(ANIMATION_SIGNAL_STOP);
		animationManState = ANIMATION_MAN_STATE_SWITCHING;
	}

	logprint("%s Setting animation to %s. %s\n", __FUNCTION__, animations[anim].name, immediately ? "Immediately" : "Signal sent");
}

int AnimationMan_TakeUsrCommand(int argc, char **argv)
{
	// if (argc < 3) {
	// 	logprint("usage: %s <id> <on|off|toggle>\n", argv[0]);
	// 	return -1;
	// }

	if (!animationManInitialized)
	{
		return 1;
	}
	ASSERT_ARGS(2);
	if (strcmp(argv[1], "set") == 0)
	{
		ASSERT_ARGS(3);
		for (int i = 0; i < ANIMATION_MAX; i++)
		{
			if (strcmp(argv[2], animations[i].name) == 0)
			{
				AnimationMan_SetAnimation(i, false);
				return 0;
			}
		}
	}
	else if (strcmp(argv[1], "conf") == 0)
	{
		AnimationMan_GenericGetSetValPath(&editableValueList, argc-2, &argv[2]);
	}
	else if (strcmp(argv[1], "next") == 0)
	{
		AnimationMan_PlayNextAnimation();
	}
	else if (strcmp(argv[1], "auto") == 0)
	{
		autoSwitchEnabled = !autoSwitchEnabled;
		logprint("Auto switch toggled to %d\n", autoSwitchEnabled);
	}
	else
	{
		currentAnimation->usrInput(argc-1, &argv[1]);
	}
	return 0;
}

// This function doesnt take the entire command line, it takes a line starting with getval or setval
uint8_t AnimationMan_GenericGetSetValPath(EditableValueList_t *l, int argc, char **argv)
{
	ASSERT_ARGS(1);
	if (strcmp(argv[0], "setval") == 0)
	{
		ASSERT_ARGS(3);
		bool ret = EditableValue_FindAndSetValueFromString(l, argv[1], argv[2]);
		logprint("%s set to %s %s\n", argv[1], argv[2], (ret) ? "SUCCESS" : "FAIL");
	}
	else if (strcmp(argv[0], "getval") == 0)
	{
		// ASSERT_ARGS(1);
		if (argc == 1)
		{
			EditableValue_PrintList(l);
		}
		else if (argc == 2)
		{
			bool isNumber = (argv[1][0] >= '0' && argv[1][0] <= '9');
			if (isNumber)
			{
				uint16_t valIdx = atoi(argv[1]);
				if (valIdx >= l->len)
				{
					logprint("%s bad val idx %d!\n", __FUNCTION__, valIdx);
					return 1;
				}
				logprint("%d ", valIdx);
				EditableValue_PrintValue(&(l->values[valIdx]));
			}
			else
			{
				EditableValue_t *ev = EditableValue_FindValueFromString(l, argv[1]);
				if (ev)
				{
					logprint("%d ", EditableValue_GetValueIdxFromString(l, argv[1]));
					EditableValue_PrintValue(ev);
				}
			}
		}
	}
  return 0;
}
