////////////////////////////////////////////////////////////////////////////
//
// Example config.h file that uses the CWKeyerShield library using a Teensy4.
// It uses no audio hardware, but instead produces the audio and pins 10+12
// of the Teensy (as a PWM-modulated output with reasonable quality).
//
// There is only one pot (for speed control) connected to Analog input A2.
//
////////////////////////////////////////////////////////////////////////////

#define HWSERIAL                  // use standard serial connection (1200 baud) for Winkey protocol

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
#define SHIELD_AUDIO_OUTPUT             0   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SHIELD_ANALOG_SPEED            A2   // Analog input for speed pot
#define SHIELD_DIGITAL_PTTOUT           4   // Digital output for PTT
#define SHIELD_DIGITAL_CWOUT            5   // Digital output for CW Keydown

#define MY_MUTE_OPTION                0   // set to 1 then RX audio is muted during CW PTT
#define MY_DEFAULT_FREQ             800   // initial setting of side tone frequency
#define MY_DEFAULT_VOLUME            80   // initial setting of side tone volume (0-127)

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins for Morse keys and CW/PTT output
// In this section a digital output pin for a square-wave side tone can be
// defined.
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight            1   // Digital input for right paddle
#define PaddleLeft             0   // Digital input for left paddle
#define StraightKey            2   // Digital input for straight key

////////////////////////////////////////////////////////////////////////////
//
// MIDI channels and note values. When using USBMIDI or MOCOLUFA, they must
// be given. If not given, default values will be used.
//
////////////////////////////////////////////////////////////////////////////
#define MY_MIDI_CHANNEL               5   // default MIDI channel to use
#define MY_KEYDOWN_NOTE               1   // default MIDI key-down note
#define MY_PTT_NOTE                   2   // default MIDI ptt note
