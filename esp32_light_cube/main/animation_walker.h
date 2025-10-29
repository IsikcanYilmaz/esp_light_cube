// #include "button.h"
#include "animation_manager.h"

bool AnimationWalker_Init(void *arg);
void AnimationWalker_Deinit(void);
void AnimationWalker_Start(void);
void AnimationWalker_Stop(void);
void AnimationWalker_Update(void);
// void AnimationWalker_ButtonInput(Button_e b, ButtonGesture_e g);
// uint8_t AnimationWalker_UsrInput(int argc, char **argv);
void AnimationWalker_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationWalker_GetState(void);
