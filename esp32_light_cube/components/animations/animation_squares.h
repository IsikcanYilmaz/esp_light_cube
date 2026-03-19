// #include "button.h"
#include "animation_manager.h"
#include "addr_led_driver.h"
#include "colorspace_interface.h"

#define SQUARE_MAX_SIDE_LENGTH (6*NUM_LEDS_PER_PANEL_SIDE)

typedef struct Square_s 
{
  Pixel_t *origin;
  Pixel_t *toLightUpNext[SQUARE_MAX_SIDE_LENGTH];
  Color_t color;
  uint8_t currSize;
  uint8_t currIteration;
  uint8_t maxIteration;
} Square_t;

bool AnimationSquares_Init(void *arg);
void AnimationSquares_Deinit(void);
void AnimationSquares_Start(void);
void AnimationSquares_Stop(void);
void AnimationSquares_Update(void);
// void AnimationSquares_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationSquares_UsrInput(int argc, char **argv);
void AnimationSquares_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationSquares_GetState(void);
