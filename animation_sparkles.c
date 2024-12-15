#include "animation_sparkles.h"
#include "addr_led_driver.h"
#include "color.h"
#include "colorspace_interface.h"
// #include "pico/rand.h"
#include "visual_util.h"
#include "editable_value.h"
#include "usr_commands.h"
// #include "hardware/timer.h"
#include "logger.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define DEFAULT_H 0.00
#define DEFAULT_S 0.80
#define DEFAULT_V 0.80

#define DEFAULT_RANDOM_UPPER_LIM_H 360.0 // 345.0
#define DEFAULT_RANDOM_UPPER_LIM_S 1.0
#define DEFAULT_RANDOM_UPPER_LIM_V 1.0

#define DEFAULT_RANDOM_LOWER_LIM_H 0.0 // 250.0
#define DEFAULT_RANDOM_LOWER_LIM_S 0.8
#define DEFAULT_RANDOM_LOWER_LIM_V 0.8

#define DEFAULT_SPARK_MODE SPARKLES_MODE_DROPS
#define DEFAULT_COLOR_MODE SPARKLES_COLOR_RANDOM
#define DEFAULT_BURST_MODE SPARKLES_BURST_RANDOM

#define DEFAULT_SPARKLE_CHANCE_PERCENT 15 
#define DEFAULT_NUM_SPARKLES_PER_BURST 1
#define DEFAULT_BURST_PERIOD 100 // 1000ms

#define DEFAULT_ITER_UNTIL_HUE_CHANGE 300

#define DEFAULT_PIXEL_H_CHANGE_PER_ITER 0
#define DEFAULT_PIXEL_S_CHANGE_PER_ITER 0.0
#define DEFAULT_PIXEL_V_CHANGE_PER_ITER -0.01

#define DEFAULT_NUM_COLORS 2
#define MAX_COLORS 30

#define DEFAULT_FADE_DIFF 0.01

// 
static uint16_t iter = 0;
static uint64_t lastBurstTimestampMs = 0;

static uint8_t numColors = DEFAULT_NUM_COLORS; // editable
static Color_t colorArr[MAX_COLORS];
static Color_t *currColor = &colorArr[0];
static uint8_t colorIdx = 0;

static SparklesSparkMode_e sparkMode = DEFAULT_SPARK_MODE;
static SparklesColorMode_e colorMode = DEFAULT_COLOR_MODE;
static SparklesBurstMode_e burstMode = DEFAULT_BURST_MODE;

static double randomLowerLimH = DEFAULT_RANDOM_LOWER_LIM_H;
static double randomUpperLimH = DEFAULT_RANDOM_UPPER_LIM_H;

static double randomLowerLimS = DEFAULT_RANDOM_LOWER_LIM_S;
static double randomUpperLimS = DEFAULT_RANDOM_UPPER_LIM_S;

static double randomLowerLimV = DEFAULT_RANDOM_LOWER_LIM_V;
static double randomUpperLimV = DEFAULT_RANDOM_UPPER_LIM_V;

static uint16_t iterUntilChange = DEFAULT_ITER_UNTIL_HUE_CHANGE;

static double hChange = DEFAULT_PIXEL_H_CHANGE_PER_ITER;
static double sChange = DEFAULT_PIXEL_S_CHANGE_PER_ITER;
static double vChange = DEFAULT_PIXEL_V_CHANGE_PER_ITER;

static uint8_t burstChance = DEFAULT_SPARKLE_CHANCE_PERCENT;
static uint8_t burstSize = DEFAULT_NUM_SPARKLES_PER_BURST;
static uint32_t burstPeriod = DEFAULT_BURST_PERIOD;
                  
static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "iterUntilChange", .valPtr = (union EightByteData_u *) &iterUntilChange, .type = UINT16_T, .ll.u16 = 1, .ul.u16 = 0xffff},
	(EditableValue_t) {.name = "numColors", .valPtr = (union EightByteData_u *) &numColors, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = MAX_COLORS},
	(EditableValue_t) {.name = "randomLowerLimH", .valPtr = (union EightByteData_u *) &randomLowerLimH, .type = DOUBLE, .ll.d = 0.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "randomUpperLimH", .valPtr = (union EightByteData_u *) &randomUpperLimH, .type = DOUBLE, .ll.d = 0.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "randomLowerLimS", .valPtr = (union EightByteData_u *) &randomLowerLimS, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "randomUpperLimS", .valPtr = (union EightByteData_u *) &randomUpperLimS, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "randomLowerLimV", .valPtr = (union EightByteData_u *) &randomLowerLimV, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "randomUpperLimV", .valPtr = (union EightByteData_u *) &randomUpperLimV, .type = DOUBLE, .ll.d = 0.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "hChange", .valPtr = (union EightByteData_u *) &hChange, .type = DOUBLE, .ll.d = -360.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "sChange", .valPtr = (union EightByteData_u *) &sChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vChange", .valPtr = (union EightByteData_u *) &vChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "burstMode", .valPtr = (union EightByteData_u *) &burstMode, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = SPARKLES_BURST_MAX},
	(EditableValue_t) {.name = "burstPeriod", .valPtr = (union EightByteData_u*) &burstPeriod, .type = UINT32_T, .ll.u16 = 1, .ul.u16 = 0xffff},
	(EditableValue_t) {.name = "burstChance", .valPtr = (union EightByteData_u *) &burstChance, .type = UINT8_T,  .ll.u8 = 0, .ul.u8 = 100},
	(EditableValue_t) {.name = "burstSize", .valPtr = (union EightByteData_u *) &burstSize, .type = UINT8_T,  .ll.u8 = 0, .ul.u8 = NUM_LEDS},
};
static EditableValueList_t editableValuesList = {.name = "sparkles", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

uint8_t numVacancies = NUM_LEDS;
uint8_t vacancyIndexes[NUM_LEDS];

static Color_t GenerateRandomColor(void)
{
	double h = fmod(rand(), (randomUpperLimH - randomLowerLimH + 1)) + randomLowerLimH;
	double s = (double) randomLowerLimS + fmod((rand() % 1000) / 100, (randomUpperLimS - randomLowerLimS + 0.01));
	double v = (double) randomLowerLimV + fmod((rand() % 1000) / 100, (randomUpperLimV - randomLowerLimV + 0.01));
	// logprint("%f %f %f\n", h, s, v);
	Color_t c = Color_CreateFromHsv(h, s, v);
	// Color_PrintColor(c);
	return c;
}

static void SetCurrColorRandomly(void)
{
	if (numColors > 1)
	{
		currColor = &colorArr[rand() % numColors];
	}
}

static void CheckVacancies(void)
{
	AddrLedStrip_t *s = AddrLedDriver_GetStrip();
	numVacancies = 0;
	for (uint8_t i = 0; i < s->numLeds; i++)
	{
		Pixel_t *p = &(s->pixels[i]); 
		if (Visual_IsDark(p))
		{
			vacancyIndexes[numVacancies] = i;
			numVacancies++;
		}
	}
}

static bool CheckShouldBurst(void)
{
	switch(burstMode)
	{
		case SPARKLES_BURST_RANDOM:
		{
			return rand() % 100 < burstChance;
			break;
		}
		case SPARKLES_BURST_TIMED:
		{
			return (burstPeriod < ztimer_now(ZTIMER_USEC)/1000 - lastBurstTimestampMs);
			logprint("SPARKLES_BURST_TIMED not yet implemented\n");
			break;
		}
		default:
		{
			logprint("Bad burst mode!\n");
			return false;
		}
	}
	return true;
}

static void Burst(void)
{
	for (uint8_t i = 0; i < burstSize; i++)
	{
		if (numVacancies == 0)
		{
			break;
		}
		bool emptyCell = false;
		Pixel_t *p;
		AddrLedStrip_t *s = AddrLedDriver_GetStrip();
		uint8_t randTableIdx = (uint8_t) rand() % numVacancies;
		uint8_t randLedIdx = vacancyIndexes[randTableIdx];
		p = &(s->pixels[randLedIdx]); 
		if (Visual_IsDark(p))
		{
			AddrLedDriver_SetPixelRgb(p, currColor->red, currColor->green, currColor->blue);
		}
	}
	lastBurstTimestampMs = ztimer_now(ZTIMER_USEC)/1000;
}

static void RunningAction(void)
{
	CheckVacancies();
	if (iter >= iterUntilChange)
	{
		*currColor = GenerateRandomColor();
		colorIdx = colorIdx % numColors;
		iter = 0;
	}
	SetCurrColorRandomly();

	// Dice roll for sparkle. 
	if (CheckShouldBurst())
	{
		Burst();
	}
	Visual_IncrementAllByHSV(hChange,sChange,vChange);
	iter++;
}

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	Visual_IncrementAllByHSV(0,0,-DEFAULT_FADE_DIFF);
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		logprint("Fade off done state %d\n", state);
	}
}

static void InitColors(void)
{
	for (uint8_t i = 0; i < MAX_COLORS; i++)
	{
		colorArr[i] = GenerateRandomColor();
	}
}

static void IterThruColors(void)
{
	colorIdx = (colorIdx + 1) % numColors;
	colorArr[colorIdx] = GenerateRandomColor();
}

bool AnimationSparkles_Init(void *arg)
{
	InitColors();
	AddrLedDriver_Clear();
	state = ANIMATION_STATE_RUNNING;
	logprint("%s\n", __FUNCTION__);
	return true;
}

void AnimationSparkles_Deinit(void)
{
	// TODO when/if i end up using a dynamic allocator i'll do freeing here. for now, STOP basically means UNINITIALIZED
	state = ANIMATION_STATE_UNINITIALIZED;
}

void AnimationSparkles_Start(void)
{
}

void AnimationSparkles_Stop(void)
{
}

void AnimationSparkles_Update(void)
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

void AnimationSparkles_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationSparkles_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	logprint("Sparkles received usr input:");
	for (int i = 0; i < argc; i++)
	{
		logprint(" %s", argv[i]);
	}
	logprint("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
}

void AnimationSparkles_ReceiveSignal(AnimationSignal_e s)
{
	logprint("%s signal received %d\n", __FUNCTION__, s);
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
			logprint("%s bad signal %d\n", __FUNCTION__, s);
			break;
		}
	}
}

AnimationState_e AnimationSparkles_GetState(void)
{
	return state;
}
