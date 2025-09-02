#include "button.h"
#include "animation_manager.h"

typedef enum SparklesColorMode_e_
{
	SPARKLES_COLOR_RANDOM,
	SPARKLES_COLOR_FIXED, // TODO
	SPARKLES_COLOR_MAX
} SparklesColorMode_e;

typedef enum SparklesBurstMode_e_
{
	SPARKLES_BURST_RANDOM,
	SPARKLES_BURST_TIMED,
	SPARKLES_BURST_MAX
} SparklesBurstMode_e;

typedef enum SparklesSparkMode_e_
{
	SPARKLES_MODE_NORMAL,
	SPARKLES_MODE_DROPS,
	SPARKLES_MODE_SQUARES,
	SPARKLES_MODE_MAX
} SparklesSparkMode_e;

bool AnimationSparkles_Init(void *arg);
void AnimationSparkles_Deinit(void);
void AnimationSparkles_Start(void);
void AnimationSparkles_Stop(void);
void AnimationSparkles_Update(void);
void AnimationSparkles_Draw(void);
void AnimationSparkles_ButtonInput(Button_e b, ButtonGesture_e g);
uint8_t AnimationSparkles_UsrInput(int argc, char **argv);
void AnimationSparkles_ReceiveSignal(AnimationSignal_e s);
AnimationState_e AnimationSparkles_GetState(void);
