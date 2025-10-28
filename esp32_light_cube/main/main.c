/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#include "addr_led_driver.h"
#include "animation_manager.h"

void app_main(void)
{
  bool ret = true;
  ret = AddrLedDriver_Init();
  ret |= AnimationMan_Init();

  while(1){
    /*vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));*/
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
