////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a plain vanilla Arduino-Uno, but this also works
// for many other boards.
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

//#define POWERSAVE                // enable "sleep" mode if not used for some time
#define MYSERIAL Serial

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
// Note it is also possible to use an external free-running sine oscillator
// and modulate it using the "pulse shaper" output.
//
// For THIS keyer the "pulse shaper" and "key down" lines are functionally equivalent.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               3   // Digital input for left paddle
#define StraightKey              4   // Digital input for straight key
#define CW1                      13   // Digital output (active high) for CW key-down
#define PTT1                     7   // Digital output (active high) for PTT on/off
#define PTT2                     8   // Digital output (active low) for PTT on/off
#define TONEPIN                 10   // Digital output for square wave side tone


////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton
// array.
//
////////////////////////////////////////////////////////////////////////////

//#define POTPIN                   A6   // Analog input for speed pot handled in the keyer
//#define BUTTONPIN                A8   // Analog input for push-buttons
