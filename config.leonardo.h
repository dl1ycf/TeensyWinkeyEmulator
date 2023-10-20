////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a Leonardo. It does NOT use the USB serial
// connection but rather the hardware UART (on D0 and A1). So the only
// difference to the Arduino setup is to use SERIAL1 rather than SERIAL.
//
// - inputs for Paddle *and* a straight key can be connected (D2, D3, D4)
// - four push-buttons for EEPROM messages (D8, D9, D10, D11)
// - "speed potentiometer"  (analog input line A0)
// - active-high KEY and PTT output (D5, D6)
// - digital (square wave) output of a side tone (D7)
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial1

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined. Note if the PULSESHAPER option is used, the side tone may be
// free-running but can also be switched on and off. However, when switching
// off the side tone using the pulse shaper, the side tone must continue
// for about 10 milli-seconds after PULSESHAPER went down. This can most
// elegantly be accomplished by substituting
//
// noTone(TONEPIN)  ==> tone(TONEPIN, current_frequency, 10)
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      5   // Digital output (active high) for CW key-down
#define PTT1                     6   // Digital output (active high) for PTT on/off
#define TONEPIN                  7   // Digital output for square wave side tone
#define PULSESHAPER             13   // Digital output for "switching on" side  tone

////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton
// array.
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                   A0   // Analog input for speed pot handled in the keyer
#define BUTTONPIN                A1   // Analog input for push-buttons
