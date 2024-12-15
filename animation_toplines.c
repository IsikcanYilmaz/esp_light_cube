#include "animation_toplines.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include "control_signals.h"
#include "timex.h"
#include "ztimer.h"
#include "xtimer.h"
#include "colorspace_interface.h"
#include "addr_led_driver.h"
#include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

static Color_t currColor;
static float freq = 0.5;
static double hIncrement = -3;
static double sIncrement = -0.05;
static double vIncrement = -0.03;

typedef struct 
{
	Position_e startingPosition;
	uint8_t startingX;
	uint8_t startingY;
	

} Line_t;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hIncrement", .valPtr = (union EightByteData_u *) &hIncrement, .type = DOUBLE, .ll.d = -360.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "sIncrement", .valPtr = (union EightByteData_u *) &sIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vIncrement", .valPtr = (union EightByteData_u *) &vIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
};
static EditableValueList_t editableValuesList = {.name = "toplines", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	Visual_IncrementAllByHSV(0,0,-0.01);
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		logprint("Fade off done state %d\n", state);
	}
}

static void RunningAction(void)
{
	
}

bool AnimationToplines_Init(void *arg)
{
	currColor = Color_CreateFromHsv(0.0, 1.0, 0.4);
	state = ANIMATION_STATE_RUNNING;
	return true;
}

void AnimationToplines_Deinit(void)
{
}

void AnimationToplines_Start(void)
{
}

void AnimationToplines_Stop(void)
{
}

void AnimationToplines_Update(void)
{
	switch(state)
	{
		case ANIMATION_STATE_STARTING:
		{
			state = ANIMATION_STATE_RUNNING; // TODO populate this area
		}
		case ANIMATION_STATE_RUNNING:
		{
			RunningAction();
			break;
		}
		case ANIMATION_STATE_STOPPING:
		{
			FadeOffAction();
			break;
		}
		case ANIMATION_STATE_STOPPED:
		{
			// NOP
			break;
		}
		default:
		{
			break;
		}
	}
}

void AnimationToplines_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationToplines_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	logprint("Toplines received usr input: \n");
	for (int i = 0; i < argc; i++)
	{
		logprint(" %s", argv[i]);
	}
	logprint("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
}

void AnimationToplines_ReceiveSignal(AnimationSignal_e s)
{
	switch(s)
	{
		case ANIMATION_SIGNAL_START:
		{
			state = ANIMATION_STATE_STARTING;
			break;
		}
		case ANIMATION_SIGNAL_STOP:
		{
			state = ANIMATION_STATE_STOPPING;
			break;
		}
		default:
		{

			break;
		}
	}
}

AnimationState_e AnimationToplines_GetState(void)
{
	return state;
}
