#include <stdlib.h>
#include <stdio.h>
#include "button.h"
#include "logger.h"
// #include "hardware/gpio.h"

ButtonContext_s buttonContext;

char *buttonEnumStrings[] = {
	[BUTTON_0] = "BUTTON_0", 
	[BUTTON_1] = "BUTTON_1", 
	[BUTTON_BOTH] = "BUTTON_BOTH", 
	[BUTTON_NONE] = "BUTTON_NONE"
};

char *buttonGestureStrings[] = {
	[GESTURE_SINGLE_TAP] = "SINGLE_TAP",
	[GESTURE_DOUBLE_TAP] = "DOUBLE_TAP",
	[GESTURE_TRIPLE_TAP] = "TRIPLE_TAP",
	[GESTURE_LONG_PRESS] = "LONG_PRESS",
	[GESTURE_VLONG_PRESS] = "VLONG_PRESS",
	[GESTURE_VVLONG_PRESS] = "VVLONG_PRESS",
	[GESTURE_VVVLONG_PRESS] = "VVVLONG_PRESS",
	[GESTURE_NONE] = "NONE"
};

// static Button_e Button_GpioToButtonEnum(uint8_t gpio)
// {
// 	// if (gpio == BUTTON_KEY_0_GPIO)
// 	// {
// 	// 	return BUTTON_0;
// 	// }
// 	// else if (gpio == BUTTON_KEY_1_GPIO)
// 	// {
// 	// 	return BUTTON_1;
// 	// }
// 	// else
// 	// {
// 		return BUTTON_NONE;
// 	// }
// }
//
// static void Button_PrintContext(void)
// {
// 	logprint("b0:%d b1:%d | taps:%d gesture:%s ts:%llu\n", buttonContext.buttonState[BUTTON_0], buttonContext.buttonState[BUTTON_1], buttonContext.currentNumTaps, buttonGestureStrings[buttonContext.currentGesture], buttonContext.currentTapTimestamp);
// }
//
// static Button_e Button_GetLockedPressedButtons(void)
// {
// 	// bool b0 = (buttonContext.lockedButtonState[BUTTON_0] == BUTTON_STATE_PRESSED);
// 	// bool b1 = (buttonContext.lockedButtonState[BUTTON_1] == BUTTON_STATE_PRESSED);
// 	// if (b0 && !b1) return BUTTON_0;
// 	// if (b1 && !b0) return BUTTON_1;
// 	// if (b0 && b1) return BUTTON_BOTH;
// 	return BUTTON_NONE;
// }

// Oddly I dont see a need to handle debounce on this hw? maybe
// its the oled/button board thats doing something? maybe its the pico?
// So I wont handle it??
// static bool Button_TimerCallback(struct repeating_timer_t *t)
// {
// 	Button_e currPressed = Button_GetPressedButtons();
// 	ButtonGesture_e g = GESTURE_NONE;
// 	Button_e b = Button_GetLockedPressedButtons();
// 	// logprint("BUTTON CALLBACK %s LOCKED %s\n", buttonEnumStrings[currPressed], buttonEnumStrings[b]);
// 	
// 	// Button presses are done. time to process
// 	if (currPressed == BUTTON_NONE)
// 	{
// 		uint64_t elapsedMs = (get_absolute_time() - buttonContext.currentTapTimestamp)/1000;
// 		uint16_t numElapsedLongPress = elapsedMs / BUTTON_LONG_PRESS_TIME_MS;
// 		if (buttonContext.currentNumTaps > 1 && b != BUTTON_BOTH)
// 		{
// 			if (buttonContext.currentNumTaps == 2) g = GESTURE_DOUBLE_TAP;
// 			if (buttonContext.currentNumTaps == 3) g = GESTURE_TRIPLE_TAP;
// 		}
// 		else
// 		{
// 			if (numElapsedLongPress == GESTURE_SINGLE_TAP_SEC) g = GESTURE_SINGLE_TAP;
// 			else if (numElapsedLongPress == GESTURE_LONG_PRESS_SEC) g = GESTURE_LONG_PRESS;
// 			else if (numElapsedLongPress == GESTURE_VLONG_PRESS_SEC) g = GESTURE_VLONG_PRESS;
// 			else if (numElapsedLongPress < GESTURE_VVVLONG_PRESS_SEC) g = GESTURE_VVLONG_PRESS;
// 			else g = GESTURE_VVVLONG_PRESS;
// 		}
// 		Button_GestureHappened(b, g);
// 		return false;
// 	}
// 	return true;
// }

// static void Button_StartTimer(void)
// {
// 	// if (buttonContext.timerRunning) return;
// 	// buttonContext.timerRunning = true;
// 	// add_repeating_timer_ms(BUTTON_POLL_PERIOD_MS, Button_TimerCallback, NULL, &(buttonContext.buttonTimer));
// }
//
// static void Button_StopTimer(void)
// {
// 	// if (!buttonContext.timerRunning) return;
// 	// cancel_repeating_timer(&(buttonContext.buttonTimer));
// 	// buttonContext.timerRunning = false;
// }
//
// static void Button_ResetContext(void)
// {
// 	buttonContext.buttonState[BUTTON_0] = BUTTON_STATE_RELEASED;
// 	buttonContext.buttonState[BUTTON_1] = BUTTON_STATE_RELEASED;
// 	buttonContext.lockedButtonState[BUTTON_0] = BUTTON_STATE_RELEASED;
// 	buttonContext.lockedButtonState[BUTTON_1] = BUTTON_STATE_RELEASED;
// 	buttonContext.timerRunning = false;
// 	buttonContext.currentGesture = GESTURE_NONE;
// 	buttonContext.currentNumTaps = 0;
// 	buttonContext.currentTapTimestamp = 0;
// }
//
// // Events v
// // "LEVEL_LOW",  // 0x1
// // "LEVEL_HIGH", // 0x2
// // "EDGE_FALL",  // 0x4
// // "EDGE_RISE"   // 0x8
// static void Button_GpioIrqCallback(uint gpio, uint32_t events) 
// {
// 	// Button_e eventButton = Button_GpioToButtonEnum(gpio);
// 	// // logprint("GPIO %d EVENTS 0x%x B%d S%d\n", gpio, events, eventButton, buttonContext.buttonState[eventButton]);
// 	// switch(events)
// 	// {
// 	// 	case BUTTON_EVENT_LEVEL_LOW:
// 	// 	{
// 	// 		return; // unused
// 	// 		break;
// 	// 	}
// 	// 	case BUTTON_EVENT_LEVEL_HIGH:
// 	// 	{
// 	// 		return; // unused
// 	// 		break;
// 	// 	}
// 	// 	case BUTTON_EVENT_EDGE_RISE: // BUTTON RELEASE
// 	// 	{
// 	// 		buttonContext.buttonState[eventButton] = BUTTON_STATE_RELEASED;
// 	// 		Button_StopTimer();
// 	// 		Button_StartTimer();
// 	// 		break;
// 	// 	}
// 	// 	case BUTTON_EVENT_EDGE_FALL: // BUTTON PRESS
// 	// 	{
// 	// 		Button_StopTimer(); // TODO?
// 	// 		Button_StartTimer();
// 	// 		Button_e otherButton = ((eventButton == BUTTON_0) ? BUTTON_1 : BUTTON_0);
// 	// 		// Increment num taps if this specific button wasnt pressed before
// 	// 		// Also, hack, dont increment if this is a double press.
// 	// 		if (buttonContext.buttonState[eventButton] != BUTTON_STATE_PRESSED &&
// 	// 			  buttonContext.buttonState[otherButton] != BUTTON_STATE_PRESSED)
// 	// 		{
// 	// 			buttonContext.currentNumTaps++;
// 	// 		}
// 	// 		buttonContext.buttonState[eventButton] = BUTTON_STATE_PRESSED;
// 	// 		buttonContext.lockedButtonState[eventButton] = BUTTON_STATE_PRESSED;
// 	// 		buttonContext.currentTapTimestamp = get_absolute_time();
// 	// 		break;
// 	// 	}
// 	// 	default:
// 	// 	{
// 	// 		logprint("Button Gpio bad event %x\n", events);
// 	// 		return;
// 	// 	}
// 	// }
// 	// // Button_PrintContext();
// }
//
void Button_Init(void)
{ 
// 	gpio_init(BUTTON_KEY_0_GPIO);
// 	gpio_init(BUTTON_KEY_1_GPIO);
//
// 	gpio_set_dir(BUTTON_KEY_0_GPIO, GPIO_IN);
// 	gpio_set_dir(BUTTON_KEY_1_GPIO, GPIO_IN);
//
// 	gpio_pull_down(BUTTON_KEY_0_GPIO);
// 	gpio_pull_down(BUTTON_KEY_1_GPIO);
//
// 	gpio_set_irq_enabled_with_callback(BUTTON_KEY_0_GPIO, 
// 																		GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
// 																		true, 
// 																		Button_GpioIrqCallback);
// 	gpio_set_irq_enabled_with_callback(BUTTON_KEY_1_GPIO, 
// 																		GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
// 																		true, 
// 																		Button_GpioIrqCallback);
//
// 	Button_ResetContext();
// }
//
// bool Button_IsPressed(Button_e b)
// {
// 	switch(b)
// 	{
// 		case BUTTON_0:
// 		{
// 			return (gpio_get(BUTTON_KEY_0_GPIO) == BUTTON_STATE_PRESSED);
// 		}
// 		case BUTTON_1:
// 		{
// 			return (gpio_get(BUTTON_KEY_1_GPIO) == BUTTON_STATE_PRESSED);
// 		}
// 		case BUTTON_BOTH:
// 		{
// 			return ((gpio_get(BUTTON_KEY_0_GPIO) == BUTTON_STATE_PRESSED) && (gpio_get(BUTTON_KEY_1_GPIO) == BUTTON_STATE_PRESSED));
// 		}
// 		default:
// 		{
// 			logprint("Wrong argument to Button_IsPressed %d\n", b);
// 			return false;
// 		}
// 	}
}

Button_e Button_GetPressedButtons(void)
{
	// bool b0 = (gpio_get(BUTTON_KEY_0_GPIO) == BUTTON_STATE_PRESSED);
	// bool b1 = (gpio_get(BUTTON_KEY_1_GPIO) == BUTTON_STATE_PRESSED);
	// if (b0 && !b1) return BUTTON_0;
	// if (b1 && !b0) return BUTTON_1;
	// if (b0 && b1) return BUTTON_BOTH;
	return BUTTON_NONE;
}

void Button_GestureHappened(Button_e b, ButtonGesture_e g)
{
	// // TODO make this section critical! disable irq
	// logprint("Button press %s %d:%s\n", buttonEnumStrings[b], g, buttonGestureStrings[g]);
	// if (g < GESTURE_NONE && b < BUTTON_NONE)
	// {
	// 	// Misc_TakeButtonInput(b, g);
	// 	// GuiMan_TakeButtonInput(b, g);
	// }
	// Button_ResetContext();
}
