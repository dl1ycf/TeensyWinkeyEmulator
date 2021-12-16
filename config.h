////////////////////////////////////////////////////////////////////////////
//
// This file is for a Teensy4 using the TeensyUSBAudioMidi low-latency lib
// but may be adapted to produce, for example, a simple Arduino-based
// WinKey clone with one serial connection only and a square wave side tone
// on one of the output pins.
//
////////////////////////////////////////////////////////////////////////////

#define TEENSYUSBAUDIOMIDI          // use TeensyUSBAudioMidi library (audio and MIDI)
//#define USBMIDI                   // use usbMIDI for MIDI messages (Teensy or 32U4 based Arduino)
//#define MOCOLUFA                  // use standard serial port for MIDI messages
#define HWSERIAL                    // use "standard" serial connection for Winkey protocol
//#define SWSERIAL                  // use "software serial" for Winkey protocol (requires TXD RXD)
//#define TXD 8                     // TX pin if using SWSERIAL
//#define RXD 9                     // RX pin if using SWSERIAL
//#define SERIAL1                   // use UART not USB for Winkey protocol (Teensy only)

////////////////////////////////////////////////////////////////////////////
//
// Define channels and Note/controllers for MIDI messages.
// USBMIDI and MOCOLUFA do not use MIDI_PITCH_CTRL and MIDI_CONTROL_CHANNEL
//
////////////////////////////////////////////////////////////////////////////

#define MIDI_CW_CHANNEL          5   // channel for sending key-down and ptt messages
#define MIDI_CW_NOTE             1   // Note (On/Off) for key-up/down
#define MIDI_PTT_NOTE            2    // Note (On/Off) for PTT on/off
#define MIDI_SPEED_CTRL          3    // Controller (0-127) for reporting speed
#define MIDI_PITCH_CTRL          4   // Controller (0-127) for reporting side tone frequency
#define MIDI_CONTROL_CHANNEL     2   // channel for receiving MIDI commands

////////////////////////////////////////////////////////////////////////////
//
// Defaults for audio system (only TEENSYUSBAUDIOMIDI)
//
////////////////////////////////////////////////////////////////////////////

#define AUDIO_OUTPUT             0   // 0: MQS, 1: I2S(wm8960), 2: I2S(sgtl5000)
#define SIDETONE_VOLUME        0.2   // initial side tone volume
#define SIDETONE_FREQ          800   // initial side tone frequency
#define MUTE_ON_PTT              1   // if true, mute RX audio while PTT is active

////////////////////////////////////////////////////////////////////////////
//
// Digital input and output pins
//
////////////////////////////////////////////////////////////////////////////

#define PaddleRight              1   // input for right paddle
#define PaddleLeft               0   // input for left paddle
#define StraightKey              2   // input for straight key
#define CW1                      4   // output (active high) for CW key-down
#define PTT1                     5   // output (active high) for PTT on/off
//#define PTT2                   6   // output (active low)  for PTT on/off
//#define TONEPIN                7   // output for square wave side tone

////////////////////////////////////////////////////////////////////////////
//
// Analog input pins to monitor by TeensyUSBAudioMidi class
//
////////////////////////////////////////////////////////////////////////////

#define TEENSY_ANALOG_SIDEVOL   -1   // Analog input for side tone volume, -1 if not used
#define TEENSY_ANALOG_SIDEFREQ  -1   // Analog input for CW pitch, -1 if not used
#define TEENSY_ANALOG_MASTERVOL -1   // Analog input for master volume, -1 if not used
#define TEENSY_ANALOG_SPEED     A2   // Analog input for speed pot in TeensyUSBAudioMidi, -1 if not used

////////////////////////////////////////////////////////////////////////////
//
// Analog input pins when *not* using the TeensyUSBAudioMidi class
//
////////////////////////////////////////////////////////////////////////////

//#define POTPIN                A2    // Analog input for speed pot handled in the keyer
