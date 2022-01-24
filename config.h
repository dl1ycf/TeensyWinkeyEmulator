////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library.
// This library implements a latency-free side tone mixed into an USB audio output
// stream, as well as MIDI output to the SDR program.
//
////////////////////////////////////////////////////////////////////////////

//#define USBMIDI                   // use usbMIDI for MIDI messages (Teensy or 32U4 based Arduino)
//#define MOCOLUFA                  // use standard serial port for MIDI messages at 31250 baud
#define HWSERIAL                    // use standard serial connection (1200 baud) for Winkey protocol
//#define SWSERIAL                  // use "software serial" (1200 baud) for Winkey protocol (requires TXD+RXD)
//#define TXD 8                     // TXD digital output pin if using SWSERIAL
//#define RXD 9                     // RXD digital input  pin if using SWSERIAL
//
// Note that
// -USBMIDI overrides MOCOLUFA
// -MOCOLUFA overrides HWSERIAL
// -HWSERIAL overrides SWSERIAL
// -SWSERIAL is disables unless both TXD and RXD are defined


////////////////////////////////////////////////////////////////////////////
//
// Settings for the CWKeyerShield library.
// (if CWKEYERSHIELD is not defined, the other settings in this section
//  have no meaning)
//
// Note that in the Keyer, defining CWKEYERSHIELD automatically disables
// the USBMIDI, MOCOLUFA, and POTPIN settings.
//
////////////////////////////////////////////////////////////////////////////

#define CWKEYERSHIELD                       // use CWKeyerShield library (audio and MIDI)
#define SHIELD_AUDIO_OUTPUT             1   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SHIELD_SIDETONE_VOLUME        0.2   // initial side tone volume
#define SHIELD_SIDETONE_FREQ          800   // initial side tone frequency
#define SHIELD_ANALOG_SIDEVOL   A2          // Analog input for side tone volume
#define SHIELD_ANALOG_SIDEFREQ  A3          // Analog input for CW pitch
#define SHIELD_ANALOG_MASTERVOL A1          // Analog input for master volume
#define SHIELD_ANALOG_SPEED     A8          // Analog input for speed pot in TeensyUSBAudioMidi

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
//#define TONEPIN                7   // Digital output for square wave side tone

////////////////////////////////////////////////////////////////////////////
//
// Analog input pin for speed pot
// This setting does not become effective if the CWKeyerShield module is used.
//
////////////////////////////////////////////////////////////////////////////

//#define POTPIN                A2    // Analog input for speed pot handled in the keyer


