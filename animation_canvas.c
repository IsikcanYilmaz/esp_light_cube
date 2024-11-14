// #include "animation_scroller.h"
// #include "addr_led_driver.h"
#include "animation_canvas.h"
#include "animation_manager.h"
#include "editable_value.h"
#include "visual_util.h"
#include "usr_commands.h"
#include <stdio.h>

uint8_t testval1 = 50;
uint8_t testval2 = 20;

static EditableValue_t editableValues[] = 
{
	(EditableValue_t) {.name = "testval1", .valPtr = (union EightByteData_u *) &testval1, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
	(EditableValue_t) {.name = "testval2", .valPtr = (union EightByteData_u *) &testval2, .type = UINT8_T, .ll.u8 = 0, .ul.u8 = 100},
};
static EditableValueList_t editableValuesList = {.name = "canvas", .values = &editableValues[0], .len = sizeof(editableValues)/sizeof(EditableValue_t)};

static volatile AnimationState_e state = ANIMATION_STATE_UNINITIALIZED;

static void FadeOffAction(void)
{
	// If we're stopping, fade off all LEDs. Check everytime if all LEDs are off
	Visual_IncrementAllByHSV(0,0,-0.01);
	if (Visual_IsAllDark())
	{
		state = ANIMATION_STATE_STOPPED;
		printf("Fade off done state %d\n", state);
	}
}

static void RunningAction(void)
{

}

bool AnimationCanvas_Init(void *arg)
{
	state = ANIMATION_STATE_RUNNING;
	return true;
}

void AnimationCanvas_Deinit(void)
{
}

void AnimationCanvas_Start(void)
{
}

void AnimationCanvas_Stop(void)
{
}

void AnimationCanvas_Update(void)
{
	switch(state)
	{
		case ANIMATION_STATE_STARTING:
		{
			state = ANIMATION_STATE_RUNNING; // TODO populate this area
		}
		case ANIMATION_STATE_RUNNING:
		{
			break;
		}
		case ANIMATION_STATE_STOPPING:
		{
			FadeOffAction();
			break;
		}
		case ANIMATION_STATE_STOPPED:
		{
			// NOP
			break;
		}
		default:
		{
			break;
		}
	}
}

void AnimationCanvas_ButtonInput(Button_e b, ButtonGesture_e g)
{
}

void AnimationCanvas_UsrInput(int argc, char **argv)
{
	// ASSERT_ARGS(1);
	// printf("Canvas received usr input: \n");
	// for (int i = 0; i < argc; i++)
	// {
	// 	printf(" %s", argv[i]);
	// }
	// printf("\n");
	// AnimationMan_GenericGetSetValPath(&editableValuesList, argc, argv);
}

void AnimationCanvas_ReceiveSignal(AnimationSignal_e s)
{
	switch(s)
	{
		case ANIMATION_SIGNAL_START:
		{
			state = ANIMATION_STATE_STARTING;
			break;
		}
		case ANIMATION_SIGNAL_STOP:
		{
			state = ANIMATION_STATE_STOPPING;
			break;
		}
		default:
		{

			break;
		}
	}
}

AnimationState_e AnimationCanvas_GetState(void)
{
	return state;
}
