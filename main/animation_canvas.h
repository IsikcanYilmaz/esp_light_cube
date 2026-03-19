// #include "button.h"
#include "animation_manager.h"

bool AnimationCanvas_Init(void *arg);
void AnimationCanvas_Deinit(void);
void AnimationCanvas_Start(void);
void AnimationCanvas_Stop(void);
void AnimationCanvas_Update(void);
// void AnimationCanvas_ButtonInput(Button_e b, ButtonGesture_e g);
// uint8_t AnimationCanvas_UsrInput(int argc, char **argv);
void AnimationCanvas_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationCanvas_GetState(void);
