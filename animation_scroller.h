#include "button.h"
#include <stdbool.h>
#include "animation_manager.h"

// typedef enum ScrollerSetting_e_
// {
//
// } ScrollerSetting_e;


bool AnimationScroller_Init(void *arg);
void AnimationScroller_Deinit(void);
void AnimationScroller_Start(void);
void AnimationScroller_Stop(void);
void AnimationScroller_Update(void);
void AnimationScroller_ButtonInput(Button_e b, ButtonGesture_e g);
void AnimationScroller_UsrInput(int argc, char **argv);
void AnimationScroller_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationScroller_GetState(void);
