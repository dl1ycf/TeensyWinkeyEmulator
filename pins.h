////////////////////////////////////////////////////////////////////////////////////////////////
//
// IMPORTANT:
// Input pins that you do not connect should not be defined.
// This applies to the optional inputs StraightKey, POTPIN, VOLPIN, PTTIN
//
// EVEN MORE IMPORTANT:
// Teensy input pins are NOT 5V-tolerant, max. input voltage is 3.3 Volt.
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
//
// There are some "additional pins" at the Teensy back side, but it should be clear that
// the use of additional FEATURES is somewhat restricted by I/O pin shortage.
//
// Below we define Digital Input  Pins 0,1,2,3  (paddle, straight key, PTT foot-switch)
//                 Digital Output Pins 4,5,     (CW, PTT digital out to opto-coupler)
//                 Analog  Input  Pins A2, A3   (speed  and sidetone-volume potentiometer)
//
//
// If the LCD option is used, one needs six additional digital output
// pins. Here we use 24-28 which are on the Teensy3/4 back side.
// However I have not tried this now for some time.
////////////////////////////////////////////////////////////////////////////////////////////


#define PaddleRight         0      // input for right paddle
#define PaddleLeft          1      // input for left paddle
#define StraightKey         2      // input for straight key

#define CWOUT               4      // CW keying output (active high)
#define PTTOUT              5      // PTT output (active high)

//#define POTPIN             A2      // analog input pin for the Speed pot (pin 16)
//#define VOLPIN             A3      // analog input pin for the volume pot (pin 17)
