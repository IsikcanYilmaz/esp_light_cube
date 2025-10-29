// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_snakes.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include "control_signals.h"
// #include "timex.h"
// #include "ztimer.h"
// #include "xtimer.h"
#include "colorspace_interface.h"
#include "addr_led_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
// #include "logger.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

static const char* TAG = "ANIM_SNAKE";

static Color_t currColor[16];
static float freq = 0.5;
static double hIncrement = -2;
static double sIncrement = -0;
static double vIncrement = -0.01;

static uint8_t size = 1;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hIncrement", .valPtr = (union EightByteData_u *) &hIncrement, .type = DOUBLE, .ll.d = -360.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "sIncrement", .valPtr = (union EightByteData_u *) &sIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vIncrement", .valPtr = (union EightByteData_u *) &vIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "size", .valPtr = (union EightByteData_u *) &size, .type = UINT8_T, .ll.u8 = 1, .ul.u8 = 3},
};
static EditableValueList_t editableValuesList = {.name = "snakes", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	Visual_IncrementAllByHSV(0,0,-0.01);
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		ESP_LOGI(TAG, "Fade off done state %d\n", state);
	}
}

static void RunningAction(void)
{
	static uint8_t yvals[16];
	static uint8_t xOffset;
	float now = (float) esp_timer_get_time() / 1000000.0;
	now = fmodf(now, 100.0);

	xOffset = 2 + 2 * CtrlSig_Sin(1, 0);

	Visual_IncrementAllByHSV(hIncrement, sIncrement * CtrlSig_Sin(0.01, 0),vIncrement);

	for (uint8_t i = 0; i < 16; i+=12)
	{
		// Red
		// float phaseDiffRadian = i * 90.0 * CtrlSig_Sin(0.01, 0) * M_PI / 180.0;
		float phaseDiffRadian = (i > 0 ? 1 : 0) * 180.0 * M_PI / 180.0;
		float sinout = 6 + 6 * CtrlSig_Sin(freq, phaseDiffRadian);
		yvals[i] = (int)sinout;
		Position_e pos = i/4;
		Color_t *c = &currColor[i];
		Pixel_t *targetPixel;

		if (yvals[i] < 4)
		{
			targetPixel = AddrLedDriver_GetPixelInPanel(pos, (xOffset+i)%4, yvals[i]);
		}
		else if (yvals[i] < 8) // TOP PANEL
		{
			targetPixel = AddrLedDriver_GetPixelInPanelRelative(TOP, pos, (xOffset+i)%4, yvals[i]%4);
		}
		else // OPPOSITE PANEL
		{
			Position_e oppositePos = AddrLedDriver_GetOppositePanel(pos);
			targetPixel = AddrLedDriver_GetPixelInPanelRelative(oppositePos, TOP, (xOffset+i)%4, yvals[i]%4);
		}

		if (size == 1)
		{
			AddrLedDriver_SetPixelRgb(targetPixel, c->red, c->green, c->blue);
		}
		else if (size == 2)
		{
			AddrLedDriver_SetPixelRgb(targetPixel, c->red, c->green, c->blue);
			AddrLedDriver_SetPixelRgb((Pixel_t *) targetPixel->neighborPixels[UP], c->red, c->green, c->blue);
			AddrLedDriver_SetPixelRgb((Pixel_t *) targetPixel->neighborPixels[UP_RIGHT], c->red, c->green, c->blue);
			AddrLedDriver_SetPixelRgb((Pixel_t *) targetPixel->neighborPixels[RIGHT], c->red, c->green, c->blue);
		}
		else if (size == 3)
		{
			AddrLedDriver_SetPixelRgb(targetPixel, c->red, c->green, c->blue);
			for (uint8_t i = 0; i < NUM_DIRECTIONS; i++)
			{
				Pixel_t *p = (Pixel_t *) targetPixel->neighborPixels[i];
				if (p)
				{
					AddrLedDriver_SetPixelRgb(p, c->red, c->green, c->blue);
				}
			}
		}
		break;
	}
}

bool AnimationSnakes_Init(void *arg)
{
	for (int i = 0; i < 16; i++)
	{
		currColor[i] = Color_CreateFromHsv((i * 20) % 360, 1.0, 0.4);
	}
	state = ANIMATION_STATE_RUNNING;
	return true;
}

void AnimationSnakes_Deinit(void)
{
}

void AnimationSnakes_Start(void)
{
}

void AnimationSnakes_Stop(void)
{
}

void AnimationSnakes_Update(void)
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

// void AnimationSnakes_ButtonInput(Button_e b, ButtonGesture_e g)
// {
// }

uint8_t AnimationSnakes_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	ESP_LOGI(TAG, "Snakes received usr input: \n");
	for (int i = 0; i < argc; i++)
	{
		printf(" %s", argv[i]);
	}
	printf("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
  return 0;
}

void AnimationSnakes_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationSnakes_GetState(void)
{
	return state;
}
