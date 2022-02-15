////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a Teensy, using the "USB MIDI" option.
// Contol via the WinKey protocol goes over a "software serial" line, while
// the USB connection is used for sending MIDI key-down and ptt events
// to the SDR program. So for WinKey communication, you need additional
// hardware (USB-to-serial interface).
//
// There is only one pot (for speed control) connected to Analog input A2.
//
////////////////////////////////////////////////////////////////////////////

#define RXD 7
#define TXD 8
#define SWSERIAL                  // use standard serial connection (1200 baud) for Winkey protocol
#define USBMIDI

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight            0   // Digital input for right paddle
#define PaddleLeft             1   // Digital input for left paddle
#define StraightKey            2   // Digital input for straight key

////////////////////////////////////////////////////////////////////////////
//
// MIDI messages
//
////////////////////////////////////////////////////////////////////////////
#define MY_MIDI_CHANNEL               5   // default MIDI channel to use
#define MY_KEYDOWN_NOTE               1   // default MIDI key-down note
#define MY_PTT_NOTE                   2   // default MIDI ptt note
