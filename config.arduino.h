////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a plain vanilla ArduinoUno
//
// - it uses the built-in serial port (D0, D1)
// - inputs for Paddle *and* a straight key can be connected (D2, D3, D4)
// - four push-buttons for EEPROM messages (D8, D9, D10, D11)
// - "speed potentiometer"  (A0)
// - active-high KEY and PTT output (D5, D6)
// - digital (square wave) output of a side tone (D7)
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial             // use sofware serial connection (1200 baud) for Winkey protocol

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
#define CW1                      5   // Digital output (active high) for CW key-down
#define PTT1                     6   // Digital output (active high) for PTT on/off
#define TONEPIN                  7   // Digital output for square wave side tone

#define MSG1PIN                  8   // Push-button for message #1
#define MSG2PIN                  9   // Push-button for message #2  
#define MSG3PIN                 10   // Push-button for message #3
#define MSG4PIN                 11   // Push-button for message #4


////////////////////////////////////////////////////////////////////////////
//
// Analog input pin for speed pot
// This setting does not become effective if the CWKeyerShield module is used.
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                A0    // Analog input for speed pot handled in the keyer
