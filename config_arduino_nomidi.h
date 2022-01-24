////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a plain vanilla ArduinoUno
// A "speed potentiometer" expected at analog input pin A2
//
////////////////////////////////////////////////////////////////////////////

#define HWSERIAL                    // use standard serial connection (1200 baud) for Winkey protocol

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              0   // Digital input for right paddle
#define PaddleLeft               1   // Digital input for left paddle
#define StraightKey              2   // Digital input for straight key
#define CW1                      5   // Digital output (active high) for CW key-down
//#define CW2                   10   // Digital output (active high) for CW key-down
#define PTT1                     4   // Digital output (active high) for PTT on/off
//#define PTT2                   6   // Digital output (active low)  for PTT on/off
#define TONEPIN                7   // Digital output for square wave side tone

////////////////////////////////////////////////////////////////////////////
//
// Analog input pin for speed pot
// This setting does not become effective if the CWKeyerShield module is used.
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                A2    // Analog input for speed pot handled in the keyer


