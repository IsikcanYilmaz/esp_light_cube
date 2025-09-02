#include "button.h"
#include "animation_manager.h"

// typedef enum SparklesColorMode_e_
// {
// 	SPARKLES_COLOR_RANDOM,
// 	SPARKLES_COLOR_FIXED, // TODO
// 	SPARKLES_COLOR_MAX
// } SparklesColorMode_e;
//
// typedef enum SparklesBurstMode_e_
// {
// 	SPARKLES_BURST_RANDOM,
// 	SPARKLES_BURST_TIMED,
// 	SPARKLES_BURST_MAX
// } SparklesBurstMode_e;
//
// typedef enum SparklesSparkMode_e_
// {
// 	SPARKLES_MODE_DROPS,
// 	SPARKLES_MODE_WAVE,
// 	SPARKLES_MODE_MAX
// } SparklesSparkMode_e;

bool AnimationLines_Init(void *arg);
void AnimationLines_Deinit(void);
void AnimationLines_Start(void);
void AnimationLines_Stop(void);
void AnimationLines_Update(void);
void AnimationLines_Draw(void);
void AnimationLines_ButtonInput(Button_e b, ButtonGesture_e g);
void AnimationLines_ReceiveSignal(AnimationSignal_e s);
uint8_t AnimationLines_UsrInput(int argc, char **argv);
AnimationState_e AnimationLines_GetState(void);
