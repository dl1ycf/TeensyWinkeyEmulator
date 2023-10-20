////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library with a Teensy4
// and the TeensyAudioShield that has an NXP SGTL5000 audio codec. This way
// you can make a "home-brew" build of the Keyer.
// This version only features a single potentiometer (for speed control).
//
// We also have no "microphone" and "Mic PTT" support since our hardware
// prototype just does not feature the jacks associated with these.
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL    Serial                // use standard serial connection Winkey protocol

////////////////////////////////////////////////////////////////////////////
//
// Digital input  pins for Paddle and StraightKey.
//
// Note the keyer does not need digital output lines defined if working
// with the CWKeyerShield library
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              0   // Digital input for right paddle
#define PaddleLeft               1   // Digital input for left paddle
#define StraightKey              2   // Digital input for straight key

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

#define CWKEYERSHIELD                       // use CWKeyerShield library (audio and MIDI)

#define SHIELD_AUDIO_OUTPUT             2   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SHIELD_ANALOG_SPEED            A2   // Analog input for speed pot
#define SHIELD_DIGITAL_PTTOUT           4   // Digital output for PTT
#define SHIELD_DIGITAL_CWOUT            5   // Digital output for CW Keydown

////////////////////////////////////////////////////////////////////////////
//
// run-time configurable settings for the CWKeyerShield library
//
// Note that the (compile-time) default settings for side tone frequency,
// side tone volume, and for the master volume are necessary here because
// there are no potentiometers to adjust them.
//
////////////////////////////////////////////////////////////////////////////

#define MY_MUTE_OPTION                1   // set to 1 then RX audio is muted during CW PTT
#define MY_DEFAULT_FREQ             800   // initial setting of side tone frequency
#define MY_DEFAULT_SIDETONE_VOLUME   80   // initial setting of side tone volume (0-127)
#define MY_DEFAULT_MASTER_VOLUME    100   // initial setting of master volume (0-127)
