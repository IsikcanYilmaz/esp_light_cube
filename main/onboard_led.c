#include "onboard_led.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "portmacro.h"
#include "sdkconfig.h"


/*
*	This module will take care of analog LEDs, specifically the onboard one
*	LEDC related code mostly taken from ESP's examples
*/

static uint32_t convert_percent_to_duty(uint8_t percent)
{
#if LED_GPIO_POLARIZED
	return pow(2, 13) * (100 - percent) / 100;
#else
	return pow(2, 13) * percent / 100;
#endif
}

static void OnBoardLED_InitLedc(void)
{
	// Prepare and then apply the LEDC PWM timer configuration
	ledc_timer_config_t ledc_timer = {
		.speed_mode       = LED_MODE,
		.timer_num        = LED_TIMER,
		.duty_resolution  = LED_DUTY_RES,
		.freq_hz          = LED_FREQUENCY,  // Set output frequency at 4 kHz
		.clk_cfg          = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	// Prepare and then apply the LEDC PWM channel configuration
	ledc_channel_config_t ledc_channel = {
		.speed_mode     = LED_MODE,
		.channel        = LED_CHANNEL,
		.timer_sel      = LED_TIMER,
		.intr_type      = LEDC_INTR_DISABLE,
		.gpio_num       = LED_OUTPUT_IO,
		.duty           = convert_percent_to_duty(0), // Set duty to 0%
		.hpoint         = 0
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void OnBoardLED_Task(void *arg)
{
	OnBoardLED_InitLedc();
	for (;;)
	{
		for (int i = 0; i < 100; i++)
		{
			uint32_t duty = convert_percent_to_duty(i);
			ledc_set_duty(LED_MODE, LED_CHANNEL, duty);
			ledc_update_duty(LED_MODE, LED_CHANNEL);
			vTaskDelay( 10 / portTICK_PERIOD_MS	);
		}
		for (int i = 100; i >= 0; i--)
		{
			uint32_t duty = convert_percent_to_duty(i);
			ledc_set_duty(LED_MODE, LED_CHANNEL, duty);
			ledc_update_duty(LED_MODE, LED_CHANNEL);
			vTaskDelay( 10 / portTICK_PERIOD_MS	);
		}
	}
}
