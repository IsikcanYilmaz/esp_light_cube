// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_squares.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include "esp_log.h"
#include "colorspace_interface.h"
#include <stdio.h>

static const char *TAG = "ANIM_SQUARES";

static double hBase = 0.0; // 297.8
static double sBase = 0.95; 
static double vBase = 0.6;

static double vChange = -0.25;
static double sChange = -0.00;
static double hChange = -1.0;

static uint8_t squareMaxIterations = 10;

static uint8_t skipFrames = 4;
static uint8_t skipSteps = 4;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "hBase", .valPtr = (union EightByteData_u *) &hBase, .type = DOUBLE, .ll.d = -360.0, .ul.d = 360.0},
	(EditableValue_t) {.name = "sBase", .valPtr = (union EightByteData_u *) &sBase, .type = DOUBLE, .ll.d = 0.0, .ul.d = 1.0},
	(EditableValue_t) {.name = "vBase", .valPtr = (union EightByteData_u *) &vBase, .type = DOUBLE, .ll.d = 0.0, .ul.d = 1.0},
	(EditableValue_t) {.name = "hChange", .valPtr = (union EightByteData_u *) &hChange, .type = DOUBLE, .ll.d = -20.00, .ul.d = 20.00},
	(EditableValue_t) {.name = "sChange", .valPtr = (union EightByteData_u *) &sChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 1.00},
	(EditableValue_t) {.name = "vChange", .valPtr = (union EightByteData_u *) &vChange, .type = DOUBLE, .ll.d = -1.00, .ul.d = 0.10},
	(EditableValue_t) {.name = "skipFrames", .valPtr = (union EightByteData_u *) &skipFrames, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
	(EditableValue_t) {.name = "skipSteps", .valPtr = (union EightByteData_u *) &skipSteps, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
};
static EditableValueList_t editableValuesList = {.name = "squares", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static uint8_t numAliveSquares = 0; 
static uint8_t skipFramesCounter = 0;

static Square_t square;

static Color_t currColor;

static bool checkIfPixelAlreadyNextUp(Pixel_t *p, Pixel_t **pixels)
{
  for (int i = 0; i < SQUARE_MAX_SIDE_LENGTH; i++)
  {
    if (pixels[i] == NULL)
    {
      break;
    }
    if (pixels[i] == p)
    {
      return true;
    }
  }
  return false;
}

static void createRandomSquare(void)
{
  uint8_t x = rand() % NUM_LEDS_PER_PANEL_SIDE;
  uint8_t y = rand() % NUM_LEDS_PER_PANEL_SIDE;
  Position_e pos = rand() % NUM_SIDES; 
  Pixel_t *randomOrigin = AddrLedDriver_GetPixelInPanel(pos, x, y);
  square.origin = randomOrigin;
  square.currSize = 1;
  square.currIteration = 0;
  square.toLightUpNext[0] = randomOrigin;
  square.maxIteration = squareMaxIterations;
  ESP_LOGI(TAG, "Created square at %s %d %d", AddrLedDriver_GetPositionString(pos), x, y);
}

static void drawAllSquares(void)
{
  // printf("Drawing (%d) : ", square.currSize);
  for (int i = 0; (square.toLightUpNext[i] != NULL && i < SQUARE_MAX_SIDE_LENGTH) ; i++)
  {
    Pixel_t *p = square.toLightUpNext[i];
    if (p == NULL)
    {
      // printf("x x, ");
      continue;
    }
    // printf("(%s) %d %d, ", AddrLedDriver_GetPositionString(p->pos), p->x, p->y);
    currColor = Color_CreateFromHsv(hBase, sBase, vBase); //Color_GenerateRandomColor(0.0, 360.0, 0.4, 0.9, 0.7, 0.9);
    AddrLedDriver_SetPixelRgb(p, currColor.red, currColor.green, currColor.blue);
  }
  // printf("\n");
}

static void updateAllSquares(void)
{ 
  // printf("%s ----- \n", __FUNCTION__);

  // We must be done with the drawing. Put all neighbors of the current lit up pixels to the array
  Pixel_t *currLitPixels[SQUARE_MAX_SIDE_LENGTH];

  // printf("square iter %d\n", square.currIteration);

  // first get currently lit up ones
  int numCurrLit;
  // printf("curr: ");
  for (numCurrLit = 0; numCurrLit < SQUARE_MAX_SIDE_LENGTH; numCurrLit++)
  {
    if (square.toLightUpNext[numCurrLit] == NULL)
    {
      break;
    }
    currLitPixels[numCurrLit] = (Pixel_t *) square.toLightUpNext[numCurrLit];
    // printf("%s %d %d, ", AddrLedDriver_GetPositionString(currLitPixels[numCurrLit]->pos), ((Pixel_t *) square.toLightUpNext[numCurrLit])->x, ((Pixel_t *) square.toLightUpNext[numCurrLit])->y);
  }
  // printf("\n");
  square.currSize = 0;
  memset(square.toLightUpNext, 0x00, sizeof(square.toLightUpNext));

  // then, update square.toLightUpNext with the unlit neighbors of currLitPixels
  int numTotalNewPixels = 0;
  for (int currLitIdx = 0; currLitIdx < numCurrLit; currLitIdx++)
  {
    Pixel_t *currLitPix = currLitPixels[currLitIdx];

    // printf("(%s) %d %d -> up next: ", AddrLedDriver_GetPositionString(currLitPix->pos), currLitPix->x, currLitPix->y); 
    for (int neigh = 0; neigh < NUM_NEIGHBORS; neigh++)
    {
      Pixel_t *neighborPixel = (Pixel_t *) currLitPix->neighborPixels[neigh];
      if (neighborPixel == NULL) // if neighbor doesnt exist we're done
      {
        break;
      }
      if (!Visual_IsDark(neighborPixel)) // if this neighbor is not dark, we dont want to light it up
      {
        // printf("%s %d %d not dark), ", AddrLedDriver_GetPositionString(neighborPixel->pos), neighborPixel->x, neighborPixel->y);
        continue;
      }
      if (checkIfPixelAlreadyNextUp(neighborPixel, square.toLightUpNext))
      {
        // printf("%s %d %d already added), ", AddrLedDriver_GetPositionString(neighborPixel->pos), neighborPixel->x, neighborPixel->y);
        continue;
      }
      square.toLightUpNext[numTotalNewPixels] = neighborPixel; 
      // printf("(%s) %d %d, ", AddrLedDriver_GetPositionString(neighborPixel->pos), neighborPixel->x, neighborPixel->y); 
      square.currSize++;
      numTotalNewPixels++;

    }
    // printf(" (%d) \n", square.currSize);
  }

  square.currIteration++;

  // printf("%s ----- DONE ---- \n", __FUNCTION__);
}

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
  // printf("running action\n");
  if (square.currIteration >= square.maxIteration)
  {
    memset(&square, 0x00, sizeof(Square_t));
    if (Visual_IsAllDark())
    {
      // currColor = Color_GenerateRandomColor(0.0, 360.0, 0.4, 0.9, 0.7, 0.9);
      createRandomSquare();
    }
    // return;
  }

  skipFramesCounter++;
  if (skipFramesCounter < skipFrames)
  {
    return;
  }
  skipFramesCounter = 0;
  
  // Algo
  
  drawAllSquares();
  updateAllSquares();

	if (vChange <= 0)
	{
		Visual_IncrementAllByHSV(hChange,sChange,vChange);
	}
}

bool AnimationSquares_Init(void *arg)
{
  currColor = Color_CreateFromHsv(hBase, sBase, vBase); //Color_GenerateRandomColor(0.0, 360.0, 0.4, 0.9, 0.7, 0.9);
	state = ANIMATION_STATE_RUNNING;
  memset(&square, 0x00, sizeof(Square_t));
	return true;
}

void AnimationSquares_Deinit(void)
{
}

void AnimationSquares_Start(void)
{
}

void AnimationSquares_Stop(void)
{
}

void AnimationSquares_Update(void)
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

// void AnimationSquares_ButtonInput(Button_e b, ButtonGesture_e g)
// {
// }

uint8_t AnimationSquares_UsrInput(int argc, char **argv)
{
	ASSERT_ARGS(1);
	ESP_LOGI(TAG, "Squares received usr input: ");
	for (int i = 0; i < argc; i++)
	{
		printf(" %s", argv[i]);
	}
	printf("\n");
	AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
  return 0;
}

void AnimationSquares_ReceiveSignal(AnimationSignal_e s)
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

AnimationState_e AnimationSquares_GetState(void)
{
	return state;
}
