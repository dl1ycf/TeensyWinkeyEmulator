/////////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for an Arduino Leonardo, an Arduino micro,
// or other boards using the 32U4 micro-controller (including Teensy2)
//
// The only difference to
// the "Arduino" version is that the "Serial1" line and not the USB
// connection is used for the WinKey protocol.
//
// The USB connection is used for MIDI
//
// - it uses the built-in serial port (this uses digital lines D0, D1)
// - inputs for Paddle *and* a straight key can be connected (D2, D3, D4)
// - "speed potentiometer"
// - push-button array
// - active-high KEY and PTT output
// - digital (square wave) output of a side tone
//
/////////////////////////////////////////////////////////////////////////////////

//#define POWERSAVE              // MIDI USB connection won't survive sleep
#define MYSERIAL Serial1         // Use hardware UART rather than USB connection
#define MIDIUSB                  // Use USB connection to send MIDI to SDR program

/////////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse key inputs, CW/PTT output
// and an output for a square wave side tone
//
/////////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      6   // Digital output (active high) for CW key-down
#define PTT1                     7   // Digital output (active high) for PTT on/off
#define CW2                      8   // Digital output (active low) for CW key-down
#define PTT2                     5   // Digital output (active low)  for PTT on/off
#define TONEPIN                  9   // Digital output for square wave side tone

/////////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton array.
//
/////////////////////////////////////////////////////////////////////////////////

#define POTPIN                  A6   // Analog input for speed pot handled in the keyer
#define BUTTONPIN               A8   // Analog input for push-button array

#define MY_MIDI_CHANNEL          5   // This is the default value
#define MY_KEYDOWN_NOTE          1   // This is the default value
#define MY_PTT_NOTE              2   // This is the default value
#define MY_SPEED_CTL             3   // This is the default value
