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
#include "ble_manager.h"

static const char* TAG = "MAIN";

static bool nvsInit(void)
{
  esp_err_t ret = ESP_OK;

  /* NVS flash initialization */
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
    return false;
  }
  return true;
}

// static bool usbCdcInit(void)
// {
//   // for some reason ESP-IDF 
// }

void app_main(void)
{
  bool ret = true;

  ret = nvsInit();
  ret &= AddrLedDriver_Init();
  ret &= AnimationMan_Init();
  ret &= UserCommand_Init();
  ret &= BleMan_Init();

  setvbuf(stdout, NULL, _IONBF, 0);

  if (!ret)
  {
    ESP_LOGE(TAG, "Init sequence failed!");
  }

  for(;;){}
}
