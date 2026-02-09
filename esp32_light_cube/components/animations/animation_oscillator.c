// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_oscillator.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include "control_signals.h"
// #include "timex.h"
// #include "ztimer.h"
// #include "xtimer.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "colorspace_interface.h"
#include "addr_led_driver.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

static const char* TAG = "ANIM_OSCILLATOR";

static Color_t currColor;
static double freq = 1.3;
static double currH = 255.0;
static double currS = 1.0;
static double currV = 0.15;
static double hIncrement = -0;
static double sIncrement = -0;
static double vIncrement = -0.06;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hIncrement", .valPtr = (union EightByteData_u *) &hIncrement, .type = DOUBLE, .ll.d = -360.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "sIncrement", .valPtr = (union EightByteData_u *) &sIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vIncrement", .valPtr = (union EightByteData_u *) &vIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "currH", .valPtr = (union EightByteData_u *) &currH, .type = DOUBLE, .ll.d = 0.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "currS", .valPtr = (union EightByteData_u *) &currS, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "currV", .valPtr = (union EightByteData_u *) &currV, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "freq", .valPtr = (union EightByteData_u *) &freq, .type = DOUBLE, .ll.d = 0.00, .ul.d = 2.00},
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
		ESP_LOGI(TAG, "Fade off done state %d", state);
	}
}

static void RunningAction(void)
{
	static uint8_t yvals[16];
	float now = (float) esp_timer_get_time() / 1000000.0;
	now = fmodf(now, 100.0);
	// freq = 1;

	Visual_IncrementAllByHSV(hIncrement, sIncrement * CtrlSig_Sin(0.01, 0),vIncrement);
  Color_t c = Color_CreateFromHsv(currH, currS, currV);
	for (uint8_t i = 0; i < 16; i++)
	{
		// Red
		float phaseDiffRadian = i * 90.0 * CtrlSig_Sin(0.01, 0) * M_PI / 180.0;
		float sinout = 2 + 2 * CtrlSig_Sin(freq, phaseDiffRadian);
		yvals[i] = (int)sinout;
		Position_e pos = i/4;
		yvals[i] = (yvals[i] > 3) ? 3 : yvals[i];
		AddrLedDriver_SetPixelRgbInPanel(pos, i%4, yvals[i]%4, c.red, c.green, c.blue);
	}
}

bool AnimationOscillator_Init(void *arg)
{
	currColor = Color_CreateFromHsv(0.0, 1.0, 0.8); // TODO depricated
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

// void AnimationOscillator_ButtonInput(Button_e b, ButtonGesture_e g)
// {
// }

uint8_t AnimationOscillator_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	ESP_LOGI(TAG, "Oscillator received usr input:");
	for (int i = 0; i < argc; i++)
	{
		printf(" %s", argv[i]);
	}
	printf("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
  return 0;
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
