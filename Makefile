# name of your application
APPLICATION = light_cube 

# If no BOARD is found in the environment, use this default:
BOARD ?= seeedstudio-esp32s3

# This has to be the absolute path to the RIOT base directory:
# RIOTBASE ?= $(CURDIR)/../..
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
USEMODULE += ws281x_esp32
USEMODULE += xtimer
USEMODULE += shell
USEMODULE += esp_wifi
USEMODULE += shell_cmds_default
USEMODULE += ps

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

include $(RIOTBASE)/Makefile.include
