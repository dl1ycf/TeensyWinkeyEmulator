////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a Teensy (Teensy2 or higher).
// Contol via the WinKey protocol goes over the built-in UART ("Serial1"),
// while the USB connection is used for sending MIDI key-down and ptt events
// to a SDR program.
//
// There is only one pot (for speed control) connected to Analog input A2.
//
// Note later Teensy models (TeensyLC, Teensy3.x, Teensy4.x) can do
// Serial and MIDI *simultaneously* over the  USB port.
// In this case, simply replace "Serial1" by "Serial" few lines
// below, and you get both MIDI and WinKey over the USB cable.
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial1	        // use built-in UART ...
#define USBMIDI                   // ... since USB is used for MIDI

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
// When using "Serial1", take care not to use its default RXD/TXD pins,
// which seem to be 0+1 for a Teensy4.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      6   // Digital output (active high) for CW key-down
#define PTT1                     7   // Digital output (active high) for PTT on/off
#define TONEPIN                 10   // Digital output for square wave side tone
#define PULSESHAPER              9   // Digital output for "switching on" side  tone


////////////////////////////////////////////////////////////////////////////
//
// Analog input pins
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                     A6  // Analog input for speed pot handled in the keyer
#define BUTTONPIN                  A8 // Analog input for push-buttons

////////////////////////////////////////////////////////////////////////////
//
// MIDI channels and note values. If these are not given, the sketch defines
// default values.
//
////////////////////////////////////////////////////////////////////////////

#define MY_MIDI_CHANNEL               5   // default MIDI channel to use
#define MY_KEYDOWN_NOTE               1   // default MIDI key-down note
#define MY_PTT_NOTE                   2   // default MIDI ptt note
