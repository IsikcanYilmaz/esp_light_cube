
#include <stdio.h>
#include <string.h>
#include "usr_commands.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"

#include "animation_manager.h"
#include "addr_led_driver.h"

static const char* TAG = "UsrCommands";

static esp_console_repl_t *repl = NULL;
static esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

#define MAX_CMD_LINE_LENGTH 1024

static int testcmd(int argc, char **argv)
{
  ESP_LOGI(TAG, "test argc %d", argc);
  for (int i = 0; i < argc; i++)
  {
    ESP_LOGI(TAG, "%d:%s", i, argv[i]);
  }
  return 0;
}

static void register_test(void)
{
  const esp_console_cmd_t cmd = {
    .command = "testcmd", 
    .help = "Test command. Will print all arguments",
    .hint = NULL,
    .func = &testcmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static const esp_console_cmd_t commands[] = {
  {.command = "testcmd", .help = "Test command. Will print all arguments", .hint = NULL, .func = &testcmd }, 
  {.command = "anim", .help = "Animation man commands", .hint = NULL, .func = &AnimationMan_TakeUsrCommand },
  {.command = "aled", .help = "aled driver commands", .hint = NULL, .func = &AddrLedDriver_TakeUsrCommand },
};

static void UserCommand_RegisterCommands(void)
{
  for (int i = 0; i < sizeof(commands)/sizeof(esp_console_cmd_t); i++)
  {
    ESP_ERROR_CHECK(esp_console_cmd_register(&commands[i]));
  }
}

bool UserCommand_Init(void)
{
  repl_config.prompt = ">";
  repl_config.max_cmdline_length = MAX_CMD_LINE_LENGTH;

  esp_err_t err;
  #if defined(CONFIG_ESP_CONSOLE_USB_CDC)
  err = esp_console_register_help_command();
  ESP_ERROR_CHECK(err);
  esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
  err |= esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl);
  ESP_ERROR_CHECK(err);
  #else
  #error "CDC NEEDS TO BE TURNED ON"
  #endif

  // register_test();
  UserCommand_RegisterCommands();

  esp_console_start_repl(repl);
  return (err == ESP_OK);
}
