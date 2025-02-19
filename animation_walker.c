// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_walker.h"
#include "addr_led_driver.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "colorspace_interface.h"
#include "usr_commands.h"
#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define WALKER_MAX_LENGTH 10 // TODO Implement long walkers? if you want?
#define DEFAULT_NUM_WALKERS 2
#define MAX_WALKERS 10

typedef struct Walker_s
{
	Pixel_t* pix[WALKER_MAX_LENGTH];
	Pixel_t* prevPix;
	uint8_t length;
	Color_t color;
} Walker_t;

static double hChange = 4.0;
static double sChange = 0.0;
static double vChange = -0.1;

static uint8_t numWalkers = DEFAULT_NUM_WALKERS;

static Walker_t walkers[MAX_WALKERS];

static uint8_t randomBreak = 10;
static uint8_t skipFrames = 3; // TODO find a better solution to this before more animation does this 

static uint8_t skipFrameCtr = 0;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hChange", .valPtr = (union EightByteData_u *) &hChange, .type = DOUBLE, .ll.d = -20.00, .ul.d = 20.00},
	(EditableValue_t) {.name = "sChange", .valPtr = (union EightByteData_u *) &sChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vChange", .valPtr = (union EightByteData_u *) &vChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 0.10},
	(EditableValue_t) {.name = "skipFrames", .valPtr = (union EightByteData_u *) &skipFrames, .type = UINT16_T, .ll.u16 = 0, .ul.u16 = 100},
	(EditableValue_t) {.name = "numWalkers", .valPtr = (union EightByteData_u *) &numWalkers, .type = UINT8_T, .ll.u16 = 1, .ul.u16 = MAX_WALKERS},
};
static EditableValueList_t editableValuesList = {.name = "walker", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void walkerMoveRandomly(Walker_t *w)
{
	// Walker should move to one of the neighbors but not where we just came from
	Direction_e dir;
	Pixel_t *nextPix = NULL;

	uint8_t ctr = 0; // TODO hack, to not get stuck. but really this could use a better random walk algo
	// logprint("w curr pix %x\n", w->pix[0]);
	do
	{
		dir = (Direction_e) rand() % NUM_DIRECTIONS; // TODO this can get stuck 
		nextPix = w->pix[0]->neighborPixels[dir];
		// logprint("%d %x %d\n", dir, w->pix[0]->neighborPixels[dir], ctr);
		ctr++;
		if (ctr == randomBreak)
		{
			break;
		}
	} while(nextPix == NULL || nextPix == w->prevPix);

	if (nextPix == NULL) // TODO make sure this doesnt happen and this check not needed
	{
		logprint("NULL PIXEL IN %s\n", __FUNCTION__);
		return;
	}

	if (ctr == 4)
	{
		logprint("%s ctr == %d!\n", __FUNCTION__, randomBreak);
	}
	else
	{
		w->prevPix = w->pix[0];
		w->pix[0] = nextPix;
		// logprint("Setting next pixel to %x x%d y%d\n", nextPix, nextPix->x, nextPix->y);
	}

	if (w->pix[0] == NULL)
	{
		logprint("NULL PIXEL IN %s\n", __FUNCTION__);
	}
}

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
	// skip frames if needed
	skipFrameCtr++;
	if (skipFrameCtr >= skipFrames)
	{
		skipFrameCtr = 0;
	}
	else
	{
		return;
	}

	// Update and draw our walkers
	for (int walkerId = 0; walkerId < numWalkers; walkerId++)
	{
		Walker_t *walker = &walkers[walkerId];
		walkerMoveRandomly(walker);

		// Draw our walker
		for (int i = 0; i < walker->length; i++)
		{
			AddrLedDriver_SetPixelRgb(walker->pix[i], walker->color.red, walker->color.green, walker->color.blue);
		}
	}

	if (vChange <= 0)
	{
		Visual_IncrementAllByHSV(hChange,sChange,vChange);
	}
}

bool AnimationWalker_Init(void *arg)
{
	state = ANIMATION_STATE_RUNNING;

	// Init our walker(s)
	for (int i = 0; i < MAX_WALKERS; i++)
	{
		Walker_t *walker = &walkers[i];
		walker->length = 1;
		walker->color = Color_CreateFromHsv(i * 50.0, 1.0, 0.9);
		walker->pix[0] = AddrLedDriver_GetPixelByIdx(0);
		walker->prevPix = walker->pix[0];
	}
	return true;
}

void AnimationWalker_Deinit(void)
{
}

void AnimationWalker_Start(void)
{
}

void AnimationWalker_Stop(void)
{
}

void AnimationWalker_Update(void)
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

void AnimationWalker_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationWalker_UsrInput(int argc, char **argv)
{
	// ASSERT_ARGS(1);
	// logprint("Walker received usr input: \n");
	// for (int i = 0; i < argc; i++)
	// {
	// 	logprint(" %s", argv[i]);
	// }
	// logprint("\n");
	// AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
	ASSERT_ARGS(1);
	logprint("Walker received usr input: \n");
	for (int i = 0; i < argc; i++)
	{
		logprint(" %s", argv[i]);
	}
	logprint("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);

}

void AnimationWalker_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationWalker_GetState(void)
{
	return state;
}
