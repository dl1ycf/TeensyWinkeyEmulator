# TeensyWinkeyEmulator

Winkey (K1EL) emulator, can be used with microcrontrollers such as
Ardunio, Teensy, etc.

Via the config file config.h, it can make use of the CWKeyerShield library
initiated by Steve Haynal
(github.com://softerhardware/CWKeyer/libraries/teensy/CWKeyerShield)
that works on a Teensy 4.0 and implements a low-latency side tone that
is mixed into an USB audio output stream.

However, the keyer also works on plain vanilla Arduinos. See below for a list of hardware
options.

Standalone keyer mode
=====================

The keyer can be  operated "stand-alone", that is, without being connected to a computer.
However, this is not the  mode of operation I have in mind, since this keyer offers
NO POSSIBILITY to "program" the keyer (or  change its settings) via the paddle.
This means that in standalone mode, you have to be happy with the settings in the EEPROM,
the only exception is that you can change the CW speed with a potentiometer.

Implementation of the Winkey Protocol
=====================================

This is not meant as a full-fledged Winkey emulator. It is a stripped-down one that
I wrote to make *me* happy. If you want a complete emulation, look for the K3NG keyer.

Using standard "WinKey" compatible programs, e.g. "flwkey" from the "fldigi universe",
you can change the settings but also read/modify/write the EEPROM contents that specify
the settings for standalone operation.

The most prominent use of the WinKey protocol is to allow contest logging programs to
"talk" with the keyer.

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


Hardware options
================

The keyer can be used with quite a variety of micro controllers.
To activate this, the file config.xxxxx.h has to be referenced in config.h
(or just copied to config.h ). Here is a description of the config files 
provided. If an I/O line (e.g. for a speed potentiometer) is defined but you
do not wire one, you have to un-comment tha #define.

config.arduino.h
----------------

This works with an Arduino Uno or similar, and is the most stripped-down variant.
It is a keyer implementing the K3NG WinKey protocol over a USB-to-serial connection.


config.leonardo.h
-----------------

This works with an Arduino Leonardo. The WinKey protocol is implemented on the
HARDWARE serial line, while the USB connection is used for MIDI.
MIDI messages for Key-down/up, PTT on/off, and CW speed change are sent to the host PC via USB.

config.teensy.h
---------------

This works with a Teensy2 (or higher). The WinKey protocol is implemented on the
HARDWARE serial line, while the USB connection is used for MIDI.
MIDI messages for Key-down/up, PTT on/off, and CW speed change are sent to the host PC via USB.
The only difference between teensy and leonardo is that teensy uses USBMIDI while
leonardo uses MIDIUSB.

config.teens4_bare.h
--------------------

This works with a Teensy4 (but Teensy3 should also work). It is like teensy but
the WinKey protocol is implemented over the USB-to-serial connection. This is
possible because a Teensy4 can do both MIDI and Serial simultaneously over USB.

config.teensy4_sgtl5000.h
-------------------------

This works with a Teensy4 and the AudioShield attached. In addition to the teensy4_bare
model, a side tone is produced with the AudioShield. Audio fed to the LineIn inputs
(e.g. RX audio) is copied to the Headphone output except when PTT is active, then
the audio is replaced by "silence + side tone". Compile with the "Serial+MIDI"
USB option (USB audio is NOT needed and NOT used, which means that, in case
of RFI problems,  one can use  USB opto-isolators which normally restrict the speed
such that USB audio stops working).

config.keyershield_sgtl5000.h
-----------------------------

This works with a Teensy4 and the AudioShield attached. In addition to the teensy4_bare
the device also appears as a sound card at the host computer, such that RX audio can
be played over the AudioShield. In addition, a low-latency side tone is generated and
mixed with the RX audio from USB. This is a model where the "CW Keyer" functionality
of the full-fledged "CW Keyer Shield" can be obtained from standard hardware, namely
a Teensy4 and a Teensy AudioShield.

config.keyershield.h
--------------------

This uses a Teensy4 with the "CW Keyer Shield" and fully supports that hardware, including
the knobs for Master Volume, Sidetone Volume, Sidetone Frequency and CW Speed, as well
as microphone and PTT input etc. etc.

