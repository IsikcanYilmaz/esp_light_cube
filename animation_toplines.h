#include "button.h"
#include "animation_manager.h"

bool AnimationToplines_Init(void *arg);
void AnimationToplines_Deinit(void);
void AnimationToplines_Start(void);
void AnimationToplines_Stop(void);
void AnimationToplines_Update(void);
void AnimationToplines_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationToplines_UsrInput(int argc, char **argv);
void AnimationToplines_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationToplines_GetState(void);
