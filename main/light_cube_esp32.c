#include <stdio.h>
#include <math.h>
// #include "driver/ledc.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
// #include "esp_system.h"
// #include "esp_log.h"
// #include "esp_console.h"
// #include "linenoise/linenoise.h"
// #include "argtable3/argtable3.h"
// #include "esp_vfs_cdcacm.h"
// #include "nvs.h"
// #include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "cmd_nvs.h"
// #include "cmd_system.h"
// #include "cmd_wifi.h"
#include "portmacro.h"
#include "shell.h"
#include "util.h"
#include "onboard_led.h"

TaskHandle_t shellTaskHandle = NULL;
TaskHandle_t onBoardLedTaskHandle = NULL;

void app_main(void)
{
	Util_InitializeNvs();
	Shell_Initialize();

	/* Run tasks */
	xTaskCreate(OnBoardLED_Task, "Onboard LED", 4096, NULL, 10, &onBoardLedTaskHandle);
	xTaskCreate(Shell_Task, "Shell Task", 4096, NULL, 10, &shellTaskHandle);
}
