////////////////////////////////////////////////////////////////////////////
//
// Example config.h file for a "naked" Teensy4 with no external hardware
// or keys connected.
// This is meant to produce a device which forms a "bridge" between a
// contest logger (using the WinKey protocol) and an SDR program such as piHPSDR.
//
// The Teensy runs as a keyer in host mode, and sends CW (key-down/up and
// PTT  on/off events) via MIDI to the SDR program.
// Any CW keys must be attached to the radio and handled there.
//
////////////////////////////////////////////////////////////////////////////

#define MYSERIAL Serial             // use Serial-over-USB for Winkey protocol
#define USBMIDI

////////////////////////////////////////////////////////////////////////////
//
// MIDI channels and note values. When using USBMIDI or MOCOLUFA, they must
// be given. If not given, default values will be used.
//
////////////////////////////////////////////////////////////////////////////

#define MY_MIDI_CHANNEL               5   // default MIDI channel to use
#define MY_KEYDOWN_NOTE               1   // default MIDI key-down note
#define MY_PTT_NOTE                   2   // default MIDI ptt note
#define MY_SPEED_CTL                  3   // default MIDI controller for reporting speed
