// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_game_of_life.h"
#include "addr_led_driver.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include "logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static double hIncrement = -2;
static double sIncrement = -0;
static double vIncrement = -0.01;

static GoLCellStatus_e cellStatusCurrent[NUM_LEDS];
static GoLCellStatus_e cellStatusNext[NUM_LEDS];
static GoLCellStatus_e *currentFramePointer = &cellStatusCurrent;

static uint16_t totalAlive = 0;
static uint16_t skipFrames = 100; //TODO maybe find a better solution for this?

static uint16_t skipFrameCounter = 0;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hIncrement", .valPtr = (union EightByteData_u *) &hIncrement, .type = DOUBLE, .ll.d = -360.00, .ul.d = 360.00},
	(EditableValue_t) {.name = "sIncrement", .valPtr = (union EightByteData_u *) &sIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vIncrement", .valPtr = (union EightByteData_u *) &vIncrement, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "skipFrames", .valPtr = (union EightByteData_u *) &skipFrames, .type = UINT16_T, .ll.u16 = 0, .ul.u16 = 100},
};
static EditableValueList_t editableValuesList = {.name = "gameoflife", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void printCurrentFrame(void)
{
	for (uint16_t i = 0; i < NUM_LEDS; i++)
	{
		logprint("%d ", cellStatusCurrent[i]);
	}
	logprint("\n");
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

static void getCellStatusByIdx(uint16_t idx)
{
	if (idx >= NUM_LEDS)
	{
		logprint("%s bad idx %d\n", __FUNCTION__, idx);
	}
	return cellStatusCurrent[idx];
}

static void setCellByIdx(uint16_t idx, GoLCellStatus_e status)
{
	if (idx >= NUM_LEDS)
	{
		logprint("%s bad idx %d\n", __FUNCTION__, idx);
	}
	cellStatusCurrent[idx] = status;
}

static void setCellByCoordinate(Direction_e pos, uint8_t x, uint8_t y, GoLCellStatus_e status)
{
	Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, x, y);
	if (p == NULL)
	{
		logprint("%s bad pixel! p%d x%d y%d\n", __FUNCTION__, pos, x, y);
		return;
	}
	setCellByIdx(p->stripIdx, status);
}

static void drawFrame(void)
{
	// Can draw now
	for (uint16_t pIdx = 0; pIdx < NUM_LEDS; pIdx++)
	{
		Pixel_t *p = AddrLedDriver_GetPixelByIdx(pIdx);	
		GoLCellStatus_e status = cellStatusCurrent[pIdx];
		if (status == ALIVE)
		{
			AddrLedDriver_SetPixelRgb(p, 100, 0, 0);
		}
	}
	Visual_IncrementAllByHSV(hIncrement, sIncrement ,vIncrement);
}

static void RunningAction(void)
{
	// TODO. eventually find a better way to slow down this animation
	skipFrameCounter++;
	if (skipFrameCounter < skipFrames)
	{
		drawFrame();
		return;
	}
	skipFrameCounter = 0;

	// logprint("FRAME\n");
	
	// For every cell, go thru each one of their neighbors of current 
	for (uint16_t pIdx = 0; pIdx < NUM_LEDS; pIdx++)
	{
		Pixel_t *p = AddrLedDriver_GetPixelByIdx(pIdx);
		uint16_t numAliveNeighbors = 0;

		// Check all neighbors
		for (uint8_t i = 0; i < NUM_DIRECTIONS; i++)
		{
			if (p->neighborPixels[i]) // If this neighbor exists
			{
				uint16_t neighborIdx = ((Pixel_t *) p->neighborPixels[i])->stripIdx;
				numAliveNeighbors += cellStatusCurrent[neighborIdx];
			}
		}

		// By now we know the number of alive neighbors. set the status in the next frame buffer
		GoLCellStatus_e myCurrentStatus = cellStatusCurrent[pIdx];
		GoLCellStatus_e myNextStatus = DEAD;
		if (myCurrentStatus == ALIVE && numAliveNeighbors < 2)
		{
			myNextStatus = DEAD; 
		}
		if (myCurrentStatus == ALIVE && (numAliveNeighbors == 2 || numAliveNeighbors == 3))
		{
			myNextStatus = ALIVE; 
		}
		if (myCurrentStatus == ALIVE && numAliveNeighbors >= 4)
		{
			myNextStatus = DEAD; 
		}
		if (myCurrentStatus == DEAD && numAliveNeighbors == 3)
		{
			myNextStatus = ALIVE; 
		}

		cellStatusNext[pIdx] = myNextStatus;
	}
	
	// Move over the next frame buffer to current
	// TODO could do a pointer switching mechanism to avoid doing this copy. for now this is easier
	memcpy(&cellStatusCurrent, &cellStatusNext, sizeof(GoLCellStatus_e) * NUM_LEDS);
	
	drawFrame();
}

bool AnimationGameOfLife_Init(void *arg)
{
	state = ANIMATION_STATE_RUNNING;
	memset(&cellStatusCurrent, 0, sizeof(uint8_t) * NUM_LEDS);
	memset(&cellStatusNext, 0, sizeof(uint8_t) * NUM_LEDS);
	printCurrentFrame();
	return true;
}

void AnimationGameOfLife_Deinit(void)
{
}

void AnimationGameOfLife_Start(void)
{
}

void AnimationGameOfLife_Stop(void)
{
}

void AnimationGameOfLife_Update(void)
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

void AnimationGameOfLife_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationGameOfLife_UsrInput(int argc, char **argv)
{
	// ASSERT_ARGS(1);
	// logprint("GameOfLife received usr input: \n");
	// for (int i = 0; i < argc; i++)
	// {
	// 	logprint(" %s", argv[i]);
	// }
	// logprint("\n");
	// AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
	ASSERT_ARGS(1);
	logprint("GameOfLife received usr input: \n");
	for (int i = 0; i < argc; i++)
	{
		logprint(" %s", argv[i]);
	}
	logprint("\n");

	if (strcmp(argv[0], "alive") == 0)
	{
		// anim alive <pos> <x> <y>	
		ASSERT_ARGS(4);
		Position_e pos = UserCommand_PositionStringToVal(argv[1]);
		uint8_t x = atoi(argv[2]);
		uint8_t y = atoi(argv[3]);
		logprint("Setting %s %d %d alive\n", AddrLedDriver_GetPositionString(pos), x, y);
		setCellByCoordinate(pos, x, y, ALIVE);
		printCurrentFrame();
		return;
	}

	if (strcmp(argv[0], "print") == 0)
	{
		// anim print	
		ASSERT_ARGS(1);
		printCurrentFrame();
		return;
	}

	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);

}

void AnimationGameOfLife_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationGameOfLife_GetState(void)
{
	return state;
}
