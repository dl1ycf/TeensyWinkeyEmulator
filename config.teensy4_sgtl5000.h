////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library with a Teensy4
// and the TeensyAudioShield that has an NXP SGTL5000 audio codec.
// There is only one potentiometer (for speed control) 
// at analog input line A2, the CW and PTT outputs are at digital output
// lines 5 and 4.
// Digital input lines are 0 (Straight Key), 1 (PaddleRight) and 2 (PaddleLeft).
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial             // use Serial-over-USB for Winkey protocol

////////////////////////////////////////////////////////////////////////////
//
// Digital input  pins for Paddle and StraightKey.
//
// Note the keyer does not need digital output lines defined if working
// with the CWKeyerShield library
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              1   // Digital input for right paddle
#define PaddleLeft               2   // Digital input for left paddle
#define StraightKey              0   // Digital input for straight key
#define PTT1                     4
#define CW1                      5
#define POTPIN                  A2
#define USBMIDI

#define TEENSY4AUDIO
////////////////////////////////////////////////////////////////////////////
//
// Hardware settings for the CWKeyerShield library.
// (if CWKEYERSHIELD is not defined, the other settings in this section
//  have no meaning)
//
// These settings are passed to the CWKeyerShield upon construction. Note that
// hardware lines not defined here will be set to -1 so they are not used.
//
////////////////////////////////////////////////////////////////////////////

//#define CWKEYERSHIELD                       // use CWKeyerShield library (audio and MIDI)
#define SHIELD_AUDIO_OUTPUT             2   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SHIELD_ANALOG_SPEED            A2   // Analog input for speed pot
#define SHIELD_DIGITAL_PTTOUT           4   // Digital output for PTT
#define SHIELD_DIGITAL_CWOUT            5   // Digital output for CW Keydown        

////////////////////////////////////////////////////////////////////////////
//
// run-time configurable settings for the CWKeyerShield library
// (not necessary to define them if we are using the default values)
// Note that setting the side-tone frequency/volume is only meaningful
// if there are *no* pots to adjust them, otherwise the initial values
// defined here well be overwritten upon the first reading of the pots.
//
////////////////////////////////////////////////////////////////////////////
#define MY_MUTE_OPTION                1   // set to 1 then RX audio is muted during CW PTT
#define MY_DEFAULT_FREQ             800   // initial setting of side tone frequency
#define MY_DEFAULT_SIDETONE_VOLUME   80   // initial setting of side tone volume (0-127)
#define MY_DEFAULT_MASTER_VOLUME    100   // initial setting of master volume (0-127)
