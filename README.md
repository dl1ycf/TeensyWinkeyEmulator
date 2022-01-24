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
