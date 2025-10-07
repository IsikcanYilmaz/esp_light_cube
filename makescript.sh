#!/usr/bin/env bash

FLASH=""
TERM=""
PORT="/dev/tty.usbmodem111301"
RIOTBASE="$HOME/KODMOD/RIOT/"
# BOARD="esp32s3-pros3" #"esp32s3-seeedstudio"
BOARD="esp32s3-seeedstudio"
if [ $(which compiledb) ]; then
  COMPILEDB="compiledb"
else
  COMPILEDB=""
fi

while [ $# -gt 0 ]; do
  case "$1" in
    "flash") # Select board. available options: "seeedstudio-xiao-nrf52840" "iotlab-m3" "nrf52850dk"
      shift
      FLASH="flash"
      ;;
    "term") # We're on fit iot boards. flash them and such
      shift
      TERM="term" 
      ;;
    "port")
      shift
      PORT="$1"
      shift
      ;;
    *)
      shift
      ;;
  esac
done

$COMPILEDB make $FLASH $TERM PORT=$PORT WERROR=0 BOARD="$BOARD" RIOTBASE="$RIOTBASE"

