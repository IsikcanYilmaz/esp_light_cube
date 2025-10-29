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
#include "usr_commands.h"
#include "addr_led_driver.h"
#include "animation_manager.h"

static const char* TAG = "MAIN";

void app_main(void)
{
  bool ret = true;
  ret = AddrLedDriver_Init();
  ret |= AnimationMan_Init();
  ret |= UserCommand_Init();

  if (!ret)
  {
    ESP_LOGE(TAG, "Init sequence failed!");
  }

  for(;;){}
}
