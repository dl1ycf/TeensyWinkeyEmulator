////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a plain vanilla Arduino-Uno, but this also works
// for many other boards.
//
// - it uses the built-in serial port (this uses digital lines D0, D1)
// - inputs for Paddle *and* a straight key can be connected (D2, D3, D4)
// - "speed potentiometer"  (analog input line A0)
// - push-button array (analog input line A1)
// - active-high and active-low KEY and PTT output
// - digital (square wave) output of a side tone
//
////////////////////////////////////////////////////////////////////////////

//#define POWERSAVE                // enable "sleep" mode if not used for some time
#define MYSERIAL Serial

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      6   // Digital output (active high) for CW key-down
#define PTT1                     7   // Digital output (active high) for PTT on/off
#define CW2                      8   // Digital output (active low) for CW key-down
#define PTT2                     9   // Digital output (active low)  for PTT on/off
#define TONEPIN                 10   // Digital output for square wave side tone


////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton
// array.
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                  A6   // Analog input for speed pot handled in the keyer
#define BUTTONPIN               A8   // Analog input for push-button array
