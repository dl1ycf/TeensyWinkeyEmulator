////////////////////////////////////////////////////////////////////////////////////////////////
// Hardware lines.
// For simple Arduinos, you cannot use 0 and 1 (hardware serial line)
//
// Input pins that you do not connect should not be defined. This applies to
// StraightKey, POTPIN, VOLPIN
//
////////////////////////////////////////////////////////////////////////////////////////////
//                                  Teensy-3,4 Audio Shield
//                                  =======================
//
// When using the AudioShield audio output, a lot of Teensy pins
// are "consumed" for the connection of the Shield. Those still "free" are
//
// Teensy 3: Digital Pins 0,1,2,3,4,5,8,20,21       Analog pins A2, A3
// Teensy 4: Digital Pins 0,1,2,3,4,5,9,14,22       Analog pins A2, A3
//
// If analog pins are not needed, digital pin 16 (17) can be used instead of A2 (A3)
//
// There are some "additional pins" at the Teensy back side, but it should be clear that
// the use of additional FEATURES is somewhat restricted by I/O pin shortage.
//
// Below we define Digital Input  Pins 0,1,2    (paddle, straight key)
//                 Digital Output Pins 4,5,     (CW, PTT)
//                 Analog  Input  Pins A2, A3   (speed  and sidetone-volume potentiometer)
//
//
// IMPORTANT: Teensy4 analog input pins are NOT 5-Volt-tolerant, limit input to 3.3 Volt!
//
// Such that it should not need modification for Teensy 3 and 4.
//
//
// If the LCD option is used, one needs six additional digital output
// pins. Here we use 24-28 which are on the Teensy3/4 back side.
////////////////////////////////////////////////////////////////////////////////////////////


#define PaddleRight         0      // input for right paddle
#define PaddleLeft          1      // input for left paddle
#define StraightKey         2      // input for straight key

#define CWOUT               4      // CW keying output (active high)
#define PTTOUT              5      // PTT output (active high)

#define POTPIN             A2      // analog input pin for the Speed pot
#define VOLPIN             A3      // analog input pin for the volume pot

#define RSPIN              24      // LCD display RS (only used with the LCDDISPLAY feature)
#define ENPIN              25      // LCD display EN (only used with the LCDDISPLAY feature)
#define D4PIN              26      // LCD display D4 (only used with the LCDDISPLAY feature)
#define D5PIN              27      // LCD display D5 (only used with the LCDDISPLAY feature)
#define D6PIN              28      // LCD display D6 (only used with the LCDDISPLAY feature)
#define D7PIN              29      // LCD display D7 (only used with the LCDDISPLAY feature)
