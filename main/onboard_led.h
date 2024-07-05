#include "driver/ledc.h"

#define LED_TIMER              LEDC_TIMER_0
#define LED_MODE               LEDC_LOW_SPEED_MODE
#define LED_OUTPUT_IO          (21) // Define the output GPIO
#define LED_CHANNEL            LEDC_CHANNEL_0
#define LED_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LED_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LED_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz
#define LED_GPIO_POLARIZED     (true)

void OnBoardLED_Task(void *arg);
