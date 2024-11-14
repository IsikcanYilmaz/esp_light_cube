// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_oscillator.h"
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
#include <stdint.h>
#include <stdio.h>
#include <math.h>

static float freq = 1;
static Color_t currColor;

static double hIncrement = -3;
static double sIncrement = -0.005;
static double vIncrement = -0.01;

static EditableValue_t editableValues[] = 
{
};
static EditableValueList_t editableValuesList = {.name = "oscillator", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	Visual_IncrementAllByHSV(0,0,-0.01);
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		printf("Fade off done state %d\n", state);
	}
}

static void RunningAction(void)
{
	static uint8_t yvals[16];
	float now = (float) ztimer_now(ZTIMER_USEC) / 1000000.0;
	now = fmodf(now, 100.0);
	freq = 0.5;


	for (uint8_t i = 0; i < 16; i++)
	{
		// Red
		float phase = i * 30.0 * M_PI / 180.0;
		float sinout = 2 + 2 * sin(2.0 * M_PI * freq * now + phase);
		yvals[i] = (int)sinout;
		Position_e pos = i/4;
		AddrLedDriver_SetPixelRgbInPanel(pos, i%4, yvals[i], 100, 0, 0);
	}

	Visual_IncrementAllByHSV(hIncrement,sIncrement,vIncrement);

}

bool AnimationOscillator_Init(void *arg)
{
	currColor = Color_CreateFromRgb(50, 0, 0);
	state = ANIMATION_STATE_RUNNING;
	return true;
}

void AnimationOscillator_Deinit(void)
{
}

void AnimationOscillator_Start(void)
{
}

void AnimationOscillator_Stop(void)
{
}

void AnimationOscillator_Update(void)
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

void AnimationOscillator_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationOscillator_UsrInput(int argc, char **argv)
{
	// ASSERT_ARGS(1);
	// printf("Oscillator received usr input: \n");
	// for (int i = 0; i < argc; i++)
	// {
	// 	printf(" %s", argv[i]);
	// }
	// printf("\n");
	// AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
}

void AnimationOscillator_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationOscillator_GetState(void)
{
	return state;
}
