#!/usr/bin/env python3

import threading
import serial
import time
import argparse

def writerThread(ser, payloadStr, count, sleeptime):
    print("Writer Thread")
    print(payloadStr, count, sleeptime)
    while(True):
        ser.write("anim next\n".encode())
        time.sleep(0.001)
        count -= 1
        if count == 0:
            break
    print("Writer Thread Done")

def readerThread(ser):
    print("Reader Thread")
    count = 0
    while(True):
        print(ser.readline(), count)
        count += 1

# import serial
# ser = serial.Serial("/dev/tty.usbmodem111301")
# while(True):
#     ser.write("help\n".encode())
#     time.sleep(0.001)
#     count -= 1
#     if (count == 0):
#         break
# print("Writer done")

def main():
    ser = serial.Serial("/dev/tty.usbmodem111301")
    parser = argparse.ArgumentParser()
    parser.add_argument("--payload", type=str, default="help")
    parser.add_argument("--count", type=int, default=3)
    parser.add_argument("--sleep", type=float, default=0.001)
    parser.add_argument("--no_reader", action="store_true")
    args = parser.parse_args()
    
    if not args.no_reader:
        reader = threading.Thread(target=readerThread, args=(ser,))
        reader.start()

    writer = threading.Thread(target=writerThread, args=(ser, args.payload, args.count, args.sleep))
    
    writer.start()
    writer.join()
    
    if not args.no_reader:
        reader.join()

    ser.close()


if __name__ == "__main__":
    main()
