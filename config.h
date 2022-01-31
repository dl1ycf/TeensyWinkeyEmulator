////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library.
// This library implements a latency-free side tone mixed into an USB audio output
// stream, as well as MIDI output to the SDR program.
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
#define SHIELD_AUDIO_OUTPUT             1   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SHIELD_ANALOG_MASTERVOLUME     A1   // Analog input for master volume pot
#define SHIELD_ANALOG_SIDETONEVOLUME   A2   // Analog input for side tone volume pot
#define SHIELD_ANALOG_SIDETONEFREQ     A3   // Analog input for side tone frequency pot
#define SHIELD_ANALOG_SPEED            A8   // Analog input for speed pot
#define SHIELD_DIGITAL_MICPTT           3   // Digital input for PTT
#define SHIELD_DIGITAL_PTTOUT           4   // Digital output for PTT
#define SHIELD_DIGITAL_CWOUT            5   // Digital output for CW Keydown        

////////////////////////////////////////////////////////////////////////////
//
// Options for the CWKeyerShield library. Unlike the "hardware settings"
// in the preceeding section, these values can be changed any time, and none
// of these #defines is necessary.
//
/////////////////////////////////////////////////////////////////////////////

#define MY_RADIO_CHANNEL        1           // MIDI channel for communication with radio
#define MY_KEYDOWN_NOTE         1           // MIDI note value for CW_KEYER_KEYDOWN to radio
#define MY_CWPTT_NOTE           2           // MIDI note value for PTT to radio
#define MY_SPEED_CTRL           3           // MIDI controller value for reporting speed to radio
#define MY_FREQ_CTRL            4           // MIDI controller valule for reporting sidetone frequency to radio
