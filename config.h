//
// User-specific configuration goes here
//
//
#define OPTION_MIDI_CW_CHANNEL      5   // channel for sending key-down and ptt messages
#define OPTION_MIDI_CW_NOTE         1   // Note for key-up/down
#define OPTION_MIDI_PTT_NOTE        2   // Note for PTT on/off
#define OPTION_MIDI_CONTROL_CHANNEL 2   // channel for receiving parameters
//#define OPTION_AUDIO_MQS                // define if using MQS audio out instead of I2S
#define OPTION_SIDETONE__VOLUME    0.2  // initial side tone volume
#define OPTION_SIDETONE_FREQ       800  // initial side tone frequency
//#define OPTION_ROUTE_AUDIO_BACK      1  // if defined route back "headphone audio" to USB
#define OPTION_TRAILING_MUTE       100  // no RX audio is routed within that number of milli-seconds
                                        // after a dit or dah. Used to suppress a delayed side tone
                                        // in the RX audio which cannot be switched off.

//
// Default values:
// ===============
//
// If OPTION_MIDI_CW_CHANNEL      is not defined, channel 1 will be used
// If OPTION_MIDI_CW_NOTE         is not defined, Note #1 is used for MIDI CW-key messages
// If OPTION_MIDI_PTT_NOTE        is not defined, PTT MIDI messages are not sent
// If OPTION_MIDI_CONTROL_CHANNEL is not defined, channel 2 will be used
// If OPTION_AUDIO_MQS            is not defined the I2S audio device will be used
// If OPTION_SIDETONE_VOLUME      is not defined 0.2 will be used
// If OPTION_SIDETONE_FREQ        is not defined 600 Hz will be used
// If OPTION_TRAILING_MUTE        is not defined 0 msec will be used
//
