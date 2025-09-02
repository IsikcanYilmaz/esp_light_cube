#ifndef ANIMATION_MANAGER_H_
#define ANIMATION_MANAGER_H_
#include <stdbool.h>
#include "button.h"
#include "editable_value.h"

#define ANIMATION_DEFAULT (ANIMATION_WALKER) //(ANIMATION_CANVAS)
#define ANIMATION_MANAGER_DEFAULT_FPS (60)

typedef enum AnimationIdx_e_
{
	ANIMATION_SCROLLER,
	ANIMATION_SPARKLES,
	ANIMATION_LINES,
	ANIMATION_OSCILLATOR,
	ANIMATION_SNAKES,
	ANIMATION_GAME_OF_LIFE,
	ANIMATION_WALKER,
	ANIMATION_CANVAS,
	ANIMATION_MAX
} AnimationIdx_e;

typedef enum AnimationManState_e_
{
	ANIMATION_MAN_STATE_RUNNING,
	ANIMATION_MAN_STATE_STOPPED,
	ANIMATION_MAN_STATE_SWITCHING,
	ANIMATION_MAN_STATE_MAX
} AnimationManState_e;

typedef enum AnimationState_e_
{
	ANIMATION_STATE_UNINITIALIZED,
	ANIMATION_STATE_STOPPED,
	ANIMATION_STATE_STOPPING,
	ANIMATION_STATE_STARTING,
	ANIMATION_STATE_RUNNING,
	ANIMATION_STATE_MAX
} AnimationState_e;

typedef enum AnimationSignal_e_
{
	ANIMATION_SIGNAL_START,
	ANIMATION_SIGNAL_STOP,
} AnimationSignal_e;

typedef struct AnimationSetting_s_
{
	uint8_t idx;
	char *name;
	uint8_t value[sizeof(double)];
} AnimationSetting_s;

typedef struct Animation_s_
{
	char *name;
	bool (*init)(void *arg);
	void (*deinit)(void);
	void (*start)(void);
	void (*stop)(void);
	void (*update)(void);
	void (*buttonInput)(Button_e b, ButtonGesture_e g);
	uint8_t (*usrInput)(int argc, char **argv);
	void (*signal)(AnimationSignal_e s);
	AnimationState_e (*getState)(void);
} Animation_s;

void AnimationMan_Init(void);
void AnimationMan_StartPollTimer(void);
void AnimationMan_StopPollTimer(void);
void AnimationMan_SetAnimation(AnimationIdx_e anim, bool immediately);
int AnimationMan_TakeUsrCommand(int argc, char **argv);
uint8_t AnimationMan_GenericGetSetValPath(EditableValueList_t *l, int argc, char **argv);
void *AnimationMan_ThreadHandler(void *arg);
#endif
