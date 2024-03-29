////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library and is valid
// for the Hardware developed by Steve
// (see github.com://softerhardware/CWKeyer)
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL  Serial             // use standard serial connection for Winkey protocol

////////////////////////////////////////////////////////////////////////////
//
// Digital input  pins for Paddle and StraightKey.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              0   // Digital input for right paddle
#define PaddleLeft               1   // Digital input for left paddle
#define StraightKey              2   // Digital input for straight key

////////////////////////////////////////////////////////////////////////////
//
// Hardware settings for the CWKeyerShield library.
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
// run-time configurable settings for the CWKeyerShield library
// (not necessary to define them if we are using the default values)
//
////////////////////////////////////////////////////////////////////////////

#define MY_MUTE_OPTION                 1   // if set to 1 then RX audio is muted while CW PTT is active
