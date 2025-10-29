#!/usr/bin/env bash

idf.py set-target esp32s3
idf.py -p /dev/tty.usbmodem11401 flash term
