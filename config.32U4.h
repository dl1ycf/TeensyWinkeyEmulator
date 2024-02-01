////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for an Arduino Leonardo, an Arduino micro,
// or other boards using the 32U4 micro-controller.
//
// Note that Teensy2 is different since it uses USBMIDI rather than MIDIUSB.
//
// The only difference to
// the "ArduinoUno" version is that the "Serial1" line and not the USB
// connection is used for the WinKey protocol.
//
// The USB connection is used for MIDI
//
// - it uses the built-in serial port (this uses digital lines D0, D1)
// - inputs for Paddle *and* a straight key can be connected (D2, D3, D4)
// - "speed potentiometer"  (analog input line A0)
// - push-button array (analog input line A1)
// - active-high KEY and PTT output (D5, D6)
// - digital (square wave) output of a side tone (D7)
// - additional "pulse shaper" output for modulating a possibly free-running
//   side tone
//
////////////////////////////////////////////////////////////////////////////

#define POWERSAVE                // enable "sleep" mode if not used for some time
#define MYSERIAL Serial1         // Use hardware UART rather than USB connection
#define MIDIUSB                  // Use USB connection to send MIDI to SDR program

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
//
// For THIS keyer the "pulse shaper" and "key down" lines are functionally equivalent.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      6   // Digital output (active high) for CW key-down
#define CW2                      7   // Digital output (active low) for CW key-down
#define PTT1                     8   // Digital output (active high) for PTT on/off
#define PTT2                     9   // Digital output (active low)  for PTT on/off
#define TONEPIN                 10   // Digital output for square wave side tone

////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton
// array.
//
////////////////////////////////////////////////////////////////////////////

//#define POTPIN                   A0   // Analog input for speed pot handled in the keyer
//#define BUTTONPIN                A1   // Analog input for push-button array
