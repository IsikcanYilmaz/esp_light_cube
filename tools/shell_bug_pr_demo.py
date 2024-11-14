#!/usr/bin/env python3

import serial, time

ser = serial.Serial("/dev/tty.usbmodem111301")
count = 10
while(True):
    ser.write("help\n".encode())
    time.sleep(0.001)
    count -= 1
    if (count == 0):
        break
print("Writer done")
