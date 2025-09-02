#include "button.h"
#include "animation_manager.h"

typedef enum GoLCellStatus_s {
	DEAD = 0,
	ALIVE = 1,
	GOL_CELL_STATUS_MAX
} GoLCellStatus_e;

bool AnimationGameOfLife_Init(void *arg);
void AnimationGameOfLife_Deinit(void);
void AnimationGameOfLife_Start(void);
void AnimationGameOfLife_Stop(void);
void AnimationGameOfLife_Update(void);
void AnimationGameOfLife_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationGameOfLife_UsrInput(int argc, char **argv);
void AnimationGameOfLife_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationGameOfLife_GetState(void);
