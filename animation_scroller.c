#include "animation_scroller.h"
#include "addr_led_driver.h"
#include "colorspace_interface.h"
#include "visual_util.h"
#include "editable_value.h"
#include "usr_commands.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_H 350.0
#define DEFAULT_S 0.80
#define DEFAULT_V 0.00

#define DEFAULT_UL_H 360.0
#define DEFAULT_UL_S 0.80
#define DEFAULT_UL_V 0.15

#define DEFAULT_LL_H 0.0
#define DEFAULT_LL_S 0.50
#define DEFAULT_LL_V 0.10

#define DEFAULT_ROW_H_DIFF 10
#define DEFAULT_INCREMENT_H 1.0
#define DEFAULT_PHASE_H 0.0

#define DEFAULT_FADE_DIFF 0.01

static AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static double currH = DEFAULT_H;
static double currS = DEFAULT_S;
static double currV = DEFAULT_V;

static double ulH = DEFAULT_UL_H;
static double ulS = DEFAULT_UL_S;
static double ulV = DEFAULT_UL_V;

static double llH = DEFAULT_LL_H;
static double llS = DEFAULT_LL_S;
static double llV = DEFAULT_LL_V;

static double rowHDiff = DEFAULT_ROW_H_DIFF;
static double incrementH = DEFAULT_INCREMENT_H;
static double phaseH = DEFAULT_PHASE_H;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "currH", .valPtr = (union EightByteData_u *) &currH, .type = DOUBLE, .ll.d = 0, .ul.d = 360.0},
	(EditableValue_t) {.name = "currS", .valPtr = (union EightByteData_u *) &currS, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "currV", .valPtr = (union EightByteData_u *) &currV, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "ulH", .valPtr = (union EightByteData_u *) &ulH, .type = DOUBLE, .ll.d = 0, .ul.d = 360.0},
	(EditableValue_t) {.name = "ulS", .valPtr = (union EightByteData_u *) &ulS, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "ulV", .valPtr = (union EightByteData_u *) &ulV, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "llH", .valPtr = (union EightByteData_u *) &llH, .type = DOUBLE, .ll.d = 0, .ul.d = 360.0},
	(EditableValue_t) {.name = "llS", .valPtr = (union EightByteData_u *) &llS, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "llV", .valPtr = (union EightByteData_u *) &llV, .type = DOUBLE, .ll.d = 0, .ul.d = 1.0},
	(EditableValue_t) {.name = "rowHDiff", .valPtr = (union EightByteData_u *) &rowHDiff, .type = DOUBLE, .ll.d = 0, .ul.d = 360.0},
	(EditableValue_t) {.name = "incrementH", .valPtr = (union EightByteData_u *) &incrementH, .type = DOUBLE, .ll.d = 0, .ul.d = 100.0},
	(EditableValue_t) {.name = "phaseH", .valPtr = (union EightByteData_u *) &phaseH, .type = DOUBLE, .ll.d = 0, .ul.d = 360.0},
};
static EditableValueList_t editableValuesList = {.name = "scroller", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static void RunningAction(void)
{
	phaseH += incrementH;
	phaseH = remainder(phaseH, ulH);

	// set pixel init colors of all panels except top
	for (uint8_t pos = 0; pos < TOP; pos++)
	{
		for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
		{
			for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
			{
				Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, col, row);
				Color_t c = Color_CreateFromHsv(phaseH + row * rowHDiff, currS, currV);
				AddrLedDriver_SetPixelRgb(p, c.red, c.green, c.blue);
			}
		}
	}

	// do top seperately	
	for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
	{
		for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
		{
			Pixel_t *p = AddrLedDriver_GetPixelInPanel(TOP, col, row);
			bool midSection = ((row == 1 && col == 1) || (row == 2	&& col == 1) || (row == 1 && col == 2) || (row == 2 && col == 2));
			Color_t c = Color_CreateFromHsv(phaseH + (midSection ? 5 * rowHDiff : 4 * rowHDiff), currS, currV);
			// if (midSection)
			// {
			// 	printf("%d %d ", row, col);
			// 	Color_PrintColor(c);
			// }
			AddrLedDriver_SetPixelRgb(p, c.red, c.green, c.blue);
		}
	}
}

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	currV -= DEFAULT_FADE_DIFF;
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		printf("Fade off done state %d\n", state);
	}
}

static void FadeUpAction(void)
{
	// Visual_IncrementAllByHSV(0,0,0.01);
	// // Check on any pixel if we've received our V value
	// Pixel_t *p = &(AddrLedDriver_GetStrip()->pixels[0]);
	// Color_t c = Color_CreateFromRgb(p->red, p->green, p->blue);
	currV += DEFAULT_FADE_DIFF;
	if (currV >= ulV)
	{
		state = ANIMATION_STATE_RUNNING;
		printf("Fade up done state %d\n", state);
	}
}

bool AnimationScroller_Init(void *arg)
{
	// set pixel init colors of all panels except top
	for (uint8_t pos = 0; pos < TOP; pos++)
	{
		for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
		{
			for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
			{
				Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, col, row);
				Color_t c = Color_CreateFromHsv(row * 10, DEFAULT_S, DEFAULT_V);
				AddrLedDriver_SetPixelRgb(p, c.red, c.green, c.blue);
			}
		}
	}

	// do top seperately	
	for (uint8_t row = 0; row < NUM_LEDS_PER_PANEL_SIDE; row++)
	{
		for (uint8_t col = 0; col < NUM_LEDS_PER_PANEL_SIDE; col++)
		{
			Pixel_t *p = AddrLedDriver_GetPixelInPanel(TOP, col, row);
			bool midSection = ((row == 1 && col == 1) || (row == 2	&& col == 1) || (row == 1 && col == 2) || (row == 2 && col == 2));
			Color_t c = Color_CreateFromHsv(midSection ? 5 * 10 : 4 * 10 , DEFAULT_S, DEFAULT_V);
			AddrLedDriver_SetPixelRgb(p, c.red, c.green, c.blue);
		}
	}

	state = ANIMATION_STATE_STARTING;
	printf("%s\n", __FUNCTION__);
}

void AnimationScroller_Deinit(void)
{
	// TODO when/if i end up using a dynamic allocator i'll do freeing here
	state = ANIMATION_STATE_UNINITIALIZED;
}

void AnimationScroller_Start(void)
{
}

void AnimationScroller_Stop(void)
{
}

void AnimationScroller_Update(void)
{
	switch(state)
	{
		case ANIMATION_STATE_STARTING:
		{
			RunningAction();
			FadeUpAction();
			break;
		}
		case ANIMATION_STATE_RUNNING:
		{
			RunningAction();
			break;
		}
		case ANIMATION_STATE_STOPPING:
		{
			RunningAction();
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

void AnimationScroller_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationScroller_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	printf("Scroller received usr input:");
	for (int i = 0; i < argc; i++)
	{
		printf(" %s", argv[i]);
	}
	printf("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
}

void AnimationScroller_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationScroller_GetState(void)
{
	return state;
}
