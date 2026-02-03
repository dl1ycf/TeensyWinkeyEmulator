////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a combination of the Teensy4 with its AudioShield
// that has an NXP SGTL5000 audio codec. This version does *not* use USB audio,
// it should be compiled with the "Serial+MIDI" USB-Option and runs with
// clock frequencies as low as 24 MHz.
//
// Audio from the Radio is meant to be fed to the LineIn inputs of the
// audio shield, the headphone is connected to its Headphone output.
//
// The audio from the radio, sampled at LineIn, is then just copied to the
// Headphone output, but if PTT is active, the audio is replaced by
// "silence + side tone"
//
// This runs fine with an audio library that uses 44.1 kHz sample rate.
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial             // use Serial-over-USB for Winkey protocol
#define TEENSY4AUDIO                // use I2S audio (SGTL5000) for side tone
#define USBMIDI                     // use MIDI to control SDR program

////////////////////////////////////////////////////////////////////////////
//
// Digital I/O pins
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              2   // Digital input for right paddle
#define PaddleLeft               1   // Digital input for left paddle
#define StraightKey              0   // Digital input for straight key
#define PTT1                     4   // Digital output for PTT
#define CW1                      5   // Digital output for Key-Down


////////////////////////////////////////////////////////////////////////////
//
// Analog input pins. These may include a the Speed Pot and the PushButton
// array.
//
////////////////////////////////////////////////////////////////////////////

#define POTPIN                A2    // Analog input for speed pot handled in the keyer

#define MY_MIDI_CHANNEL        5
#define MY_KEYDOWN_NOTE        1
#define MY_PTT_NOTE            2
#define MY_SPEED_CTL           3
