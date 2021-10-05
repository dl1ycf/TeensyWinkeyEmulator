//
// User-specific configuration goes here
//
//

#define MIDI_CW_CHANNEL      5   // channel for sending key-down and ptt messages
#define MIDI_CW_NOTE         1   // Note for key-up/down
#define MIDI_PTT_NOTE        2   // Note for PTT on/off
#define MIDI_SPEED_CTRL      3   // Controller for reporting speed
#define MIDI_CONTROL_CHANNEL 2   // channel for receiving parameters
#define AUDIO_USE_I2S        0   // if false, MQS audio output is used
#define SIDETONE_VOLUME    0.2   // initial side tone volume
#define SIDETONE_FREQ      800   // initial side tone frequency
#define MUTE_ON_PTT          1   // if true, mute RX audio while PTT is active
