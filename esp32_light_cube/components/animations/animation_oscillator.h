// #include "button.h"
#include "animation_manager.h"

bool AnimationOscillator_Init(void *arg);
void AnimationOscillator_Deinit(void);
void AnimationOscillator_Start(void);
void AnimationOscillator_Stop(void);
void AnimationOscillator_Update(void);
// void AnimationOscillator_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationOscillator_UsrInput(int argc, char **argv);
void AnimationOscillator_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationOscillator_GetState(void);
