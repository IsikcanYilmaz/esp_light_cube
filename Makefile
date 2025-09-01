# name of your application
APPLICATION = light_cube 

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32s3-seeedstudio

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../CCNL_RIOT/

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
# USEMODULE += esp_wifi
USEMODULE += shell_cmds_default
USEMODULE += ps

# Bluetooth
USEPKG += nimble
USEMODULE += nimble_scanner
USEMODULE += nimble_scanlist

# Mic
USEMODULE += periph_adc

# gnrc_networking
# USEMODULE += netdev_default
# USEMODULE += auto_init_gnrc_netif
# # Activate ICMPv6 error messages
# USEMODULE += gnrc_icmpv6_error
# # Specify the mandatory networking module for a IPv6 routing node
# USEMODULE += gnrc_ipv6_router_default
# # Add a routing protocol
# USEMODULE += gnrc_rpl
# USEMODULE += auto_init_gnrc_rpl
# # Additional networking modules that can be dropped if not needed
# USEMODULE += gnrc_icmpv6_echo
# USEMODULE += shell_cmd_gnrc_udp
# # Add also the shell, some shell commands
# USEMODULE += ps
# USEMODULE += netstats_l2
# USEMODULE += netstats_ipv6
# USEMODULE += netstats_rpl

# gnrc_networking end

# CCN LITE

# CFLAGS += -DUSE_LINKLAYER
# CFLAGS += -DUSE_RONR
# CFLAGS += -DCCNL_UAPI_H_
# CFLAGS += -DUSE_SUITE_NDNTLV
# CFLAGS += -DNEEDS_PREFIX_MATCHING
# CFLAGS += -DNEEDS_PACKET_CRAFTING
#
# USEMODULE += ps
# USEMODULE += shell_cmds_default
# # Include packages that pull up and auto-init the link layer.
# # NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
# USEMODULE += netdev_default
# USEMODULE += auto_init_gnrc_netif
# # This application dumps received packets to STDIO using the pktdump module
# USEMODULE += gnrc_pktdump
# USEMODULE += prng_xorshift
#
# USEPKG += ccn-lite

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
