# TeensyWinkeyEmulator

Winkey (K1EL) emulator, can be used with microcrontrollers such as
Ardunio, Teensy, etc.

Via the config file config.h, it can make use of the CWKeyerShield library
initiated by Steve Haynal
(github.com://softerhardware/CWKeyer/libraries/teensy/CWKeyerShield)
that works on a Teensy 4.0 and implements a low-latency side tone that
is mixed into an USB audio output stream.

However, the keyer also works on plain vanilla Arduinoi Nanos. Here only hardware keying
(e.g. through digital output lines and opto-couplers) is available. For microcontrollers
that support the USBmidi class (e.g. Teensy 2.0, Arduinos with 32U4 microcontroller), the
keyer can (additionally) send out MIDI messages (CW key-down/up and PTT on/off events)
to an SDR software via the USB cable.

Implementation of the Winkey Protocol
=====================================

This is not meant as a full-fledged Winkey emulator. It is a stripped-down one that
I wrote to make *me* happy. If you want a complete emulation, look for the K3NG keyer.

EXTENSION to Winkey Protocol
============================

To facilitate contesting with logging programs that are not aware of the "buffered speed"
capability, three legal but rarely used characters have been given a special meaning:

- "["  (opening bracket):  set buffered speed to 40 wpm
- "$"  (Dollar sign):      set buffered speed to 20 wpm
- "]"  (closing bracket):  cancel buffered speed

This way, you speficy e.g. your CQ call as

  CQ DL1YCF [TEST]

and will get the "test" at 40 wpm no matter which speed you operate at. Likewise,
you can define the sent-exchange such that it reads (for number 1)

  TU UR [5nn] $001]

and then you get the "5nn" at 40 wpm, followed by the serial number (001) at 20 wpm,
and in-between a word space of nominal length (after the closing bracket, keying continues
with the operating speed).
Note the "buffered speed" *only* applies to characters from the serial line,
the Paddle is always working at nominal speed.
