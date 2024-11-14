#ifndef __BUTTON_H_
#define __BUTTON_H_
#include <stdbool.h>
#include <stdint.h>
// #include "pico/stdlib.h"
// #include "hardware/timer.h"

#define BUTTON_KEY_0_GPIO 15
#define BUTTON_KEY_1_GPIO 17

#define BUTTON_POLL_PERIOD_MS 150

#define NUM_BUTTONS 2

#define BUTTON_LONG_PRESS_TIME_MS 1000

typedef enum ButtonEventBitMask_e_
{
	BUTTON_EVENT_LEVEL_LOW = 0x1,
	BUTTON_EVENT_LEVEL_HIGH = 0x2,
	BUTTON_EVENT_EDGE_FALL = 0x4,
	BUTTON_EVENT_EDGE_RISE = 0x8
} ButtonEventBitMask_e;

typedef enum ButtonState_e_ // Polarity
{
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASED,
	BUTTON_STATE_MAX
} ButtonState_e;

typedef enum Button_e_
{
	BUTTON_0,
	BUTTON_1,
	BUTTON_BOTH,
	BUTTON_NONE
} Button_e;

typedef enum PressLength_e_
{
	PRESS_TAP,
	PRESS_LONG,
	PRESS_VLONG,
	PRESS_NONE,
} PressLength_e;

typedef enum ButtonGesture_e_
{
	GESTURE_SINGLE_TAP,
	GESTURE_DOUBLE_TAP,
	GESTURE_TRIPLE_TAP,
	GESTURE_LONG_PRESS,
	GESTURE_VLONG_PRESS,
	GESTURE_VVLONG_PRESS,
	GESTURE_VVVLONG_PRESS,
	GESTURE_NONE,
} ButtonGesture_e;

typedef enum LongPressLengthSec_e_
{
	GESTURE_SINGLE_TAP_SEC = 0,
	GESTURE_LONG_PRESS_SEC = 1,
	GESTURE_VLONG_PRESS_SEC = 2,
	GESTURE_VVLONG_PRESS_SEC = 3,
	GESTURE_VVVLONG_PRESS_SEC = 7
} LongPressLengthSec_e;

typedef struct ButtonContext_s_
{
	// volatile struct repeating_timer buttonTimer;
	volatile ButtonState_e buttonState[NUM_BUTTONS];
	volatile ButtonState_e lockedButtonState[NUM_BUTTONS];
	volatile bool timerRunning;
	volatile uint8_t currentNumTaps;
	volatile ButtonGesture_e currentGesture;
	volatile uint64_t currentTapTimestamp;
} ButtonContext_s;

void Button_Init(void);
bool Button_IsPressed(Button_e b);
Button_e Button_GetPressedButtons(void);
void Button_GestureHappened(Button_e b, ButtonGesture_e g);
#endif
