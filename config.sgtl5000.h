////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library with a Teensy4
// and the TeensyAudioShield. It only has one pot (for speed control) 
// at analog input line A2, the CW and PTT outputs are at digital output
// lines 5 and 4.
//
////////////////////////////////////////////////////////////////////////////

#define HWSERIAL                    // use standard serial connection (1200 baud) for Winkey protocol

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
// These settings are passed to the CWKeyerShield upon construction
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
// (not necessary to define them if we are using the default values)
// Note that settings the side-tone frequency/volume is only meaningful
// if there are *no* pots to adjust them.
//
////////////////////////////////////////////////////////////////////////////
#define MY_MIDI_CHANNEL              10   // MIDI channel to be used by the keyer
#define MY_MUTE_OPTION                0   // set to 1 then RX audio is muted during CW PTT
#define MY_DEFAULT_FREQ             800   // initial setting of side tone frequency
#define MY_DEFAULT_VOLUME          0.20   // initial setting of side tone volume
