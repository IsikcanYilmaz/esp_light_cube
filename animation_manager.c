#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "animation_manager.h"
#include "animation_scroller.h"
#include "animation_oscillator.h"
#include "animation_canvas.h"
#include "animation_lines.h"
#include "animation_snakes.h"
#include "addr_led_driver.h"
#include "animation_sparkles.h"
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

#include "colorspace_interface.h"

char animationMan_threadStack[THREAD_STACKSIZE_DEFAULT];
Animation_s *currentAnimation;
AnimationManState_e animationManState = ANIMATION_MAN_STATE_RUNNING;
AnimationIdx_e currentAnimationIdx;
AnimationIdx_e targetAnimationIdx; // for when we're waiting for the switching of another animation
// TODO could have animations time out as a failsafe?
static bool animationManInitialized = false;

static uint16_t autoAnimationSwitchMs = 30*60*1000;
static uint32_t lastAutoAnimationSwitchTimestamp = 0;
static bool autoAnimationSwitchEnabled = true;

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
	}
};

static Animation_s * AnimationMan_GetAnimationByIdx(AnimationIdx_e idx)
{
	if (idx >= ANIMATION_MAX)
	{
		printf("Bad anim idx %d at %s\n", idx, __FUNCTION__);
		return NULL;
	}
	return &animations[idx];
}

static void AnimationMan_PlayNextAnimation(void)
{
	AnimationMan_SetAnimation((currentAnimationIdx + 1) % ANIMATION_CANVAS, false);
}

static void AnimationMan_HandleAutoSwitch(void)
{
	if (!autoAnimationSwitchEnabled)
	{
		return;
	}
	else
	{
		uint32_t now = ztimer_now(ZTIMER_USEC)/1000;
		if (ztimer_now(ZTIMER_USEC)/1000 - lastAutoAnimationSwitchTimestamp > autoAnimationSwitchMs) // Time to switch animations
		{
			printf("Auto anim switch at %d. %d %d\n", now, lastAutoAnimationSwitchTimestamp, autoAnimationSwitchMs);
			AnimationMan_PlayNextAnimation();
			lastAutoAnimationSwitchTimestamp = now;
		}
	}
}

void AnimationMan_ThreadHandler(void *arg)
{
	(void) arg;
	thread_t *thisThread = thread_get_active();
	const char *thisThreadName = thread_get_name(thisThread);
	// Main thread for animations. What it should be doing is:
	// Go through list of interruptions
	// Interact with the currently running animation
	// Display current animation
	while (true)
	{
		// Handle auto switching first
		AnimationMan_HandleAutoSwitch();

		// Main animation switch/case
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
						printf("Animation faded off. Starting next animation\n");
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
					printf("%s state invalid or not implemented yet %d\n", __FUNCTION__, animationManState);
					animationManState = ANIMATION_MAN_STATE_RUNNING; // TODO placeholder. eventually implement the stopped state. will need for temperature or deep sleep reasons? 
					break;
			}
		}
		ztimer_sleep(ZTIMER_USEC, 0.01 * US_PER_SEC);
	}
}

void AnimationMan_Init(void)
{
	currentAnimation = AnimationMan_GetAnimationByIdx(ANIMATION_DEFAULT);
	currentAnimationIdx = ANIMATION_DEFAULT;
	currentAnimation->init(NULL);

	kernel_pid_t animationMan_threadId = thread_create(
		animationMan_threadStack,
		sizeof(animationMan_threadStack),
		THREAD_PRIORITY_MAIN - 1,
		THREAD_CREATE_STACKTEST,
		AnimationMan_ThreadHandler,
		NULL,
		"animationman_thread"
	);

	animationManInitialized = true;
}

void AnimationMan_StartPollTimer(void)
{
	printf("%s\n", __FUNCTION__);
	// add_repeating_timer_ms(ANIMATION_UPDATE_PERIOD_MS, AnimationMan_PollCallback, NULL, &(animationManUpdateTimer));
}

void AnimationMan_StopPollTimer(void)
{
	printf("%s\n", __FUNCTION__);
	// cancel_repeating_timer(&(animationManUpdateTimer));
}

void AnimationMan_SetAnimation(AnimationIdx_e anim, bool immediately)
{
	if (anim >= ANIMATION_MAX)
	{
		printf("Bad anim idx %d to %s\n", anim, __FUNCTION__);
		return;
	}

	// TODO make this so that this sends a signal to the running animation which then does its cleanup and fades off
	// for now, it abruptly changes
	targetAnimationIdx = anim;
	if (immediately)
	{
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

	printf("%s Setting animation to %s. %s\n", __FUNCTION__, animations[anim].name, immediately ? "Immediately" : "Signal sent");
}

void AnimationMan_TakeUsrCommand(int argc, char **argv)
{
	// if (argc < 3) {
	// 	printf("usage: %s <id> <on|off|toggle>\n", argv[0]);
	// 	return -1;
	// }

	if (!animationManInitialized)
	{
		return;
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
				return;
			}
		}
	}
	else if (strcmp(argv[1], "next") == 0)
	{
		AnimationMan_PlayNextAnimation();
	}
	else if (strcmp(argv[1], "auto") == 0)
	{
		autoAnimationSwitchEnabled = !autoAnimationSwitchEnabled;
		printf("Auto switch toggled to %d\n", autoAnimationSwitchEnabled);
	}
	else
	{
		currentAnimation->usrInput(argc-1, &argv[1]);
	}
}

void AnimationMan_GenericGetSetValPath(EditableValueList_t *l, int argc, char **argv)
{
	if (strcmp(argv[0], "setval") == 0)
	{
		ASSERT_ARGS(3);
		bool ret = EditableValue_FindAndSetValueFromString(l, argv[1], argv[2]);
		printf("%s set to %s %s\n", argv[1], argv[2], (ret) ? "SUCCESS" : "FAIL");
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
					printf("%s bad val idx %d!\n", __FUNCTION__, valIdx);
					return;
				}
				printf("%d ", valIdx);
				EditableValue_PrintValue(&(l->values[valIdx]));
			}
			else
			{
				EditableValue_t *ev = EditableValue_FindValueFromString(l, argv[1]);
				if (ev)
				{
					printf("%d ", EditableValue_GetValueIdxFromString(l, argv[1]));
					EditableValue_PrintValue(ev);
				}
			}
		}
	}
}
