// #include "button.h"
#include "animation_manager.h"

bool AnimationSnakes_Init(void *arg);
void AnimationSnakes_Deinit(void);
void AnimationSnakes_Start(void);
void AnimationSnakes_Stop(void);
void AnimationSnakes_Update(void);
// void AnimationSnakes_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationSnakes_UsrInput(int argc, char **argv);
void AnimationSnakes_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationSnakes_GetState(void);
