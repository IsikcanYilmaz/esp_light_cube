#!/usr/bin/env python3

import argparse, sys, os, pdb, time
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.animation as animation
import pyaudio
import wave
from scipy.fft import fft, fftfreq
from scipy.signal import ShortTimeFFT, spectrogram
from scipy.io import wavfile

def attempt1(args):    
    samplerate, data = wavfile.read(args.filename)

    print(f"Name: {args.wavfilename}")
    print(f"Samplerate: {samplerate}")
    print(f"Seconds: {len(data)/samplerate}")

    mono = (data.T[0] + data.T[1]) / 2
    num_samples = int(samplerate/16)
    head = 0
    fig, ax = plt.subplots()
    plt.ion()
    plt.show()
    while ((head + num_samples) < len(data)):
        ax.clear()
        ax.set_ylim(-10e6, 10e6)
        ax.set_title(head/samplerate)
        y = fft(mono[head:head+num_samples])
        ax.plot(y[0:int(len(y)/2)])
        # time.sleep(0.1)
        input()
        head += num_samples

    fig,ax = plt.subplots()
    ax.plot(data)
    plt.show()

def rec():
    pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--wavfile", type=str)
    # parser.add_argument("--seconds", type=int)
    parser.add_argument("--rec", action='store_true')
    args = parser.parse_args()
    print(args)


if __name__ == "__main__":
    main()
