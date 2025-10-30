// #include "button.h"
#include <stdbool.h>
#include "animation_manager.h"

bool AnimationScroller_Init(void *arg);
void AnimationScroller_Deinit(void);
void AnimationScroller_Start(void);
void AnimationScroller_Stop(void);
void AnimationScroller_Update(void);
// void AnimationScroller_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationScroller_UsrInput(int argc, char **argv);
void AnimationScroller_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationScroller_GetState(void);
