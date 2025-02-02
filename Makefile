# name of your application
APPLICATION = light_cube 

# If no BOARD is found in the environment, use this default:
BOARD ?= seeedstudio-esp32s3

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT/

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Use a peripheral timer for the delay, if available
FEATURES_OPTIONAL += periph_timer

# Modules
USEMODULE += stdio_usb_serial_jtag
USEMODULE += ws281x_esp32
# USEMODULE += ws281x_esp32_non_blocking_rmt
USEMODULE += xtimer
USEMODULE += shell
USEMODULE += esp_wifi
USEMODULE += shell_cmds_default
USEMODULE += ps

# Bluetooth
USEPKG += nimble
USEMODULE += nimble_scanner
USEMODULE += nimble_scanlist

# Mic
USEMODULE += periph_adc

# External modules
EXTERNAL_MODULE_DIRS += $(CURDIR)/submodules
USEMODULE += colorspace-conversions

# Use a network interface, if available. The handling is done in
# Makefile.board.dep, which is processed recursively as part of the dependency
# resolution.
FEATURES_OPTIONAL += netif

# Configs
TIMER = 2
FREQ = 16000000

WIFI_SSID = "TP-Link_B616"
WIFI_PASS = "21854002"

INC_DIR += $(CURDIR)/submodules/colorspace-conversions/
CFLAGS += -I$(INC_DIR)

CFLAGS += -g -O0
CFLAGS_OPT = -Os # -O0

# ESP32_SDK_LOCATION = $(RIOTBASE)/build/pkg/esp32_sdk/
# SRC += $(ESP32_SDK_LOCATION)/components/driver/gdma.c \
#     $(ESP32_SDK_LOCATION)/components/hal/gdma_hal.c \
#     $(ESP32_SDK_LOCATION)/components/soc/esp32s3/gdma_periph.c 

include $(RIOTBASE)/Makefile.include
