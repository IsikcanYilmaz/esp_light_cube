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
#include "ztimer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define WALKER_MAX_LENGTH 1 // TODO Implement long walkers? if you want?
#define DEFAULT_NUM_WALKERS 2
#define MAX_WALKERS 10

typedef struct Walker_s
{
	Pixel_t* pix[WALKER_MAX_LENGTH];
	Pixel_t* prevPix;
	uint8_t length;
	Color_t color;
  bool alive;
} Walker_t;

static Walker_t walkers[MAX_WALKERS];

// HSV change per frame
static double hChange = -0.44;
static double sChange = 0.0;
static double vChange = -0.017; // -0.042

// Colors of the walkers
static double hBase = 0.0; // 297.8
static double sBase = 0.95; 
static double vBase = 0.6;

// HSV change per walker. i.e. for each walker change hsv by these much
static double hDiff = -21.0; // -42.9
static double sDiff = -0.05; // -0.1
static double vDiff = 0.0;

static uint8_t numSameWalkers = 1;

static bool diagonalMoveAllowed = false;
static bool sideMoveAllowed = true;
static bool collisionDetection = true;
static bool teleportWhenStuck = false;
static bool dyingAllowed = false;
static bool randomizeAfterDeath = false;
static bool randomizeAfterSeconds = true;
static bool randomizeHbase = true;
static bool randomizeSbase = true;
static bool randomizeHdiff = true;
static bool randomizeSdiff = true;
static uint8_t cyclesUntilRandomize = 5;
static uint8_t secondsUntilRandomize = 10;

static uint8_t moveChancePercent = 100;

static uint8_t numWalkers = DEFAULT_NUM_WALKERS;

static uint8_t skipFrames = 3; // TODO find a better solution to this before more animation does this 
static uint8_t skipSteps = 0;

static uint8_t skipFrameCtr = 0;
static uint8_t skipStepsCtr = 0;

static bool mirror = false;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hChange", .valPtr = (union EightByteData_u *) &hChange, .type = DOUBLE, .ll.d = -20.00, .ul.d = 20.00},
	(EditableValue_t) {.name = "sChange", .valPtr = (union EightByteData_u *) &sChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vChange", .valPtr = (union EightByteData_u *) &vChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 0.10},

	(EditableValue_t) {.name = "hBase", .valPtr = (union EightByteData_u *) &hBase, .type = DOUBLE, .ll.d = -360.0, .ul.d = 360.0},
	(EditableValue_t) {.name = "sBase", .valPtr = (union EightByteData_u *) &sBase, .type = DOUBLE, .ll.d = 0.0, .ul.d = 1.0},
	(EditableValue_t) {.name = "vBase", .valPtr = (union EightByteData_u *) &vBase, .type = DOUBLE, .ll.d = 0.0, .ul.d = 1.0},

	(EditableValue_t) {.name = "hDiff", .valPtr = (union EightByteData_u *) &hDiff, .type = DOUBLE, .ll.d = -100.00, .ul.d = 100.00},
	(EditableValue_t) {.name = "sDiff", .valPtr = (union EightByteData_u *) &sDiff, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	/*(EditableValue_t) {.name = "vDiff", .valPtr = (union EightByteData_u *) &vDiff, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},*/

	(EditableValue_t) {.name = "numSameWalkers", .valPtr = (union EightByteData_u *) &numSameWalkers, .type = UINT8_T, .ll.u8 = 1, .ul.u8 = MAX_WALKERS},

	(EditableValue_t) {.name = "skipFrames", .valPtr = (union EightByteData_u *) &skipFrames, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
	(EditableValue_t) {.name = "skipSteps", .valPtr = (union EightByteData_u *) &skipSteps, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},

	(EditableValue_t) {.name = "moveChancePercent", .valPtr = (union EightByteData_u *) &moveChancePercent, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
	(EditableValue_t) {.name = "numWalkers", .valPtr = (union EightByteData_u *) &numWalkers, .type = UINT8_T, .ll.u8 = 1, .ul.u8 = MAX_WALKERS},
	(EditableValue_t) {.name = "diagonalMoveAllowed", .valPtr = (union EightByteData_u *) &diagonalMoveAllowed, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "sideMoveAllowed", .valPtr = (union EightByteData_u *) &sideMoveAllowed, .type = BOOLEAN, .ll.b = false, .ul.b = true},

	(EditableValue_t) {.name = "collisionDetection", .valPtr = (union EightByteData_u *) &collisionDetection, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "teleportWhenStuck", .valPtr = (union EightByteData_u *) &teleportWhenStuck, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "dyingAllowed", .valPtr = (union EightByteData_u *) &dyingAllowed, .type = BOOLEAN, .ll.b = false, .ul.b = true},

	(EditableValue_t) {.name = "randomizeAfterDeath", .valPtr = (union EightByteData_u *) &randomizeAfterDeath, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "randomizeAfterSeconds", .valPtr = (union EightByteData_u *) &randomizeAfterSeconds, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "randomizeHbase", .valPtr = (union EightByteData_u *) &randomizeHbase, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "randomizeSbase", .valPtr = (union EightByteData_u *) &randomizeSbase, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "randomizeHdiff", .valPtr = (union EightByteData_u *) &randomizeHdiff, .type = BOOLEAN, .ll.b = false, .ul.b = true},
	(EditableValue_t) {.name = "randomizeSdiff", .valPtr = (union EightByteData_u *) &randomizeSdiff, .type = BOOLEAN, .ll.b = false, .ul.b = true},
  (EditableValue_t) {.name = "cyclesUntilRandomize", .valPtr = (union EightByteData_u *) &cyclesUntilRandomize, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 255},
  (EditableValue_t) {.name = "secondsUntilRandomize", .valPtr = (union EightByteData_u *) &secondsUntilRandomize, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 255},

	(EditableValue_t) {.name = "mirror", .valPtr = (union EightByteData_u *) &mirror, .type = BOOLEAN, .ll.b = false, .ul.b = true},
};
static EditableValueList_t editableValuesList = {.name = "walker", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static uint32_t randomizeCycleCount = 0;
static uint32_t lastRandomizeTimestampMs = 0;

static void randomize(void)
{
  if (randomizeHbase)
  {
    hBase = ((double)rand()/(double)(RAND_MAX)) * 360.00;
  }
  if (randomizeHdiff)
  {
    hDiff = ((double)rand()/(double)(RAND_MAX)) * 360.00;
  }
  if (randomizeSbase)
  {
    sBase = 0.5 + ((double)rand()/(double)(RAND_MAX)) * 0.5;
  }
  if (randomizeSdiff)
  {
    sDiff = ((double)rand()/(double)(RAND_MAX)) * 0.4;
  }
}

static void walkerMoveRandomly(Walker_t *w)
{
	// Depending on chance
	if (rand() % 100 >= moveChancePercent)
	{
		return;
	}

	// Jot down possible directions. Impossible directions mean the direction we just came from, diagonals maybe, or NULL directions
	uint8_t numPossibleDirections = 0;
	Direction_e possibleDirections[NUM_DIRECTIONS];
	
	if (!diagonalMoveAllowed && !sideMoveAllowed)
	{
		return; 
	}

	for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
	{
    // Check our diagonal and side moving rules
		bool isDiagonal = ((dir % 2) != 0);
		if (!diagonalMoveAllowed && isDiagonal)
		{
			continue;
		}
		if (!sideMoveAllowed && !isDiagonal)
		{
			continue;
		} 

		Pixel_t *pix = w->pix[0]->neighborPixels[dir];

		if (pix == NULL || pix == w->prevPix)
		{
			continue;
		}

    // If collisionDetection is on, check if pixel is already on. if so, dont go there
    if (collisionDetection && !Visual_IsDark(pix))
    {
      continue;
    }

		possibleDirections[numPossibleDirections] = dir;
		numPossibleDirections++;
	}

  if (numPossibleDirections == 0)
  {
    // If setting is set, have our walker teleport some open location
    if (teleportWhenStuck)
    {
      Pixel_t *teleportPixel = Visual_GetRandomBlankPixel();
      w->pix[0] = (teleportPixel != NULL) ? teleportPixel : w->pix[0];
    }
    else if (dyingAllowed)
    {
      w->alive = false;
    }
    return;
  }

	uint8_t randomDirection = rand() % numPossibleDirections;
	Pixel_t *nextPix = w->pix[0]->neighborPixels[possibleDirections[randomDirection]];

	if (nextPix == NULL)
	{
		logprint("NULL PIXEL IN %s\n", __FUNCTION__);
		logprint("randomDirection %d\n", randomDirection);
		logprint("numPossibleDirections %d\n", numPossibleDirections);
		return;
	}
	w->prevPix = w->pix[0];
	w->pix[0] = nextPix;
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
  bool allDead = true;

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

  // Apply decay/rise changes
	if (vChange <= 0)
	{
		Visual_IncrementAllByHSV(hChange,sChange,vChange);
	}

  // skip walking if needed
  if (skipSteps > 0 && skipStepsCtr < skipSteps)
  {
    skipStepsCtr++;
  }
  else
  {
    skipStepsCtr = 0;

    // Update and draw our walkers
    for (int walkerId = 0; walkerId < numWalkers; walkerId++)
    {
      Walker_t *walker = &walkers[walkerId];
      if (!walker->alive)
      {
        continue;
      }
      allDead = false;
      walkerMoveRandomly(walker);
    }
  }

  // If all dead and cube is all dark revive our walkers
  if (allDead && Visual_IsAllDark())
  {
    randomizeCycleCount++;
    if (randomizeAfterDeath && randomizeCycleCount % cyclesUntilRandomize == 0)
    {
      randomize();
    }
    for (int walkerId = 0; walkerId < numWalkers; walkerId++)
    {
      walkers[walkerId].alive = true;
    }
  }

  // Handle time based randomization case
  uint32_t now = ztimer_now(ZTIMER_MSEC);
  if (randomizeAfterSeconds && now - lastRandomizeTimestampMs > 1000 * secondsUntilRandomize)
  {
    randomize();
    lastRandomizeTimestampMs = now;
  }

  // Draw our walker
  Color_t c;
  for (int walkerId = 0; walkerId < numWalkers; walkerId++)
  {
    Walker_t *walker = &walkers[walkerId];
    if (walkerId % numSameWalkers == 0)
    {
      c = Color_CreateFromHsv(fmod(hBase + ((walkerId/numSameWalkers) * hDiff), 360.0), (sBase + ((walkerId/numSameWalkers) * sDiff)) > 1.00 ? 1.00 : (sBase + ((walkerId/numSameWalkers) * sDiff)), vBase);
    }
    if (walker->alive)
    {
      AddrLedDriver_SetPixelRgb(walker->pix[0], c.red, c.green, c.blue);
      if (mirror)
      {
        // AddrLedDriver_SetPixelRgb(Visual_GetMirrorPixel(walker->pix[0]), c.red, c.green, c.blue);
      }
    }
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
    walker->alive = true;
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
