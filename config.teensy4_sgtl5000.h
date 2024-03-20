////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a combination of the Teensy4 with its AudioShield
// that has an NXP SGTL5000 audio codec. This version does *not* use USB audio,
// the audio codec is solely used to produce a nice side tone.
// So compile with the "Serial+MIDI" option. If you do not need MIDI commmands
// to the PC host, un-comment "USBMIDI" below, and compile with the "Serial"
// option.
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial             // use Serial-over-USB for Winkey protocol
#define TEENSY4AUDIO                // use I2S audio (SGTL5000) for side tone
#define USBMIDI                     // use MIDI to control SDR program

////////////////////////////////////////////////////////////////////////////
//
// Digital I/O pins
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              1   // Digital input for right paddle
#define PaddleLeft               2   // Digital input for left paddle
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
