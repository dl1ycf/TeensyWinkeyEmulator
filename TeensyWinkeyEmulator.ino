////////////////////////////////////////////////////////////////////////////////////////
//
// CW keyer with audio (WinKey emulator)
//
// (C) Christoph van Wüllen, DL1YCF, 2020-2023
//
// Tested with
// - Arduino with ATmega 328p (Uno) or 32U4 (Leonardo) processors
// - Teensy 2.0  (32U4 processor)
// - Teensy 4.0  (Cortex processor) without audio
// - Teensy 4.0  with AudioShield (SGTL5000), with USB audio
// - Teensy 4.0  with CWKeyerShield, with USB audio
// - Teensy 4.0 with AudioShield (SGTL5000), *without* USB audio
//
// It can make use of the "MIDI" features of the Leonardo (and other 32U4 Arduinos), Teensy 2.0 and 4.0
// It can make use of the "AUDIO" features of Teensy 4.0
// There are some basic #define's in the file config.h
// which control which "backends" are used for signalling key-up/down and
// PTT on/off events, and the way to produce a side tone.
//
// Some recent additions:
// - supports push-button array to "play" CW messages from EEPROM
// - support low-power mode on AVR CPUs (Uno, Leonardo, Micro, Teensy2.0)
//
// File config.h mostly defines the hardware pins (digital input, digital output,
// analog input) to be used. Note that when using CWKEYERSHIELD, a speed pot
// is handled there.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include <EEPROM.h>


////////////////////////////////////////////////////////////////////////////////////////
//
//                                MAIN FEATURES
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   MIDI (only when using USBMIDI, MIDIUSB,  or the CWKeyerShield library)
//   ======================================================================
//
// - key-up/down and PTT-on/off events are sent via MIDI to the computer.
//   For USBMIDI and MIDIUSB, this is coded here in the sketch, while for
//   the CWKeyerShield library this is done inside the library.
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   Straight key connection
//   =======================
//
// - A straight key can be connected to digital input pin StraightKey.
//   (the key should connect this input with ground). Straight key takes
//   precedence over the paddle. If you do not define the StraightKey pin
//   number in pins.h, this feature is essentially deactivated.
//
//   In host mode, characters entered via the straight key are decoded and sent
//   to the host (just as for characters entered via the paddle).
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   High-quality side tone (only with TEENSY4AUDIO and CWKEYERSHIELD)
//   =================================================================
//
//   A high-quality side tone is generated, using an audio codec connected
//   to the Teensy via i2s. When using the KeyerShield library, the device
//   also operates as an USB sound card so the side tone can be mixed with
//   the RX audio.
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   K1EL Winkey (version 2.3) protocol
//   ==================================
//
// - Note it is not a full WinKey emulation, just what you need everyday.
//   Search the internet for a full-featured WinKey emulation. This is OK
//   for me to do normal QSOs and CW contests, so it is fine for me.
//
//   It does have
//   ------------
//     - PTT output with configurable lead-in and (tail or hang) times
//     - Weight, Compensation, Ratio etc. as in WinKeyer
//     - side tone on/off is implemented
//     - a speed pot
//     - a volume pot so you can always have the side tone "on"
//
//   So in standalone mode, this keyer operates on its standard settings,
//   only the speed can then  be changed with the speed pot.
//   In "host mode", the settings can be tailored through the WinKey protocol.
//
//   Note that if a valid "magic" byte is found in the EEPROM, then the settings
//   are loaded on start-up. The settings can be changed through the WinKey
//   protocol and are written to EEPROM upon each "host close" command.
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   Iambic-A/B, Ultimatic, and all that stuff
//   =========================================
//
//   These variants differ in the way the keyer works when *both* contacts of the
//   paddle are closed. Therefore, these differences all vanish if you use a
//   single-lever paddle where only one contact can be closed at a time.
//
//   Iambic-A and Iambic-B modes
//   ---------------------------
//
//   In the iambic modes, closing both contacts for some time produces a series
//   of alternating dits and dahs. If only one of the two contacts opens at
//   a later time, the series is continued with dits or dahs (only).
//
//   In Iambic-A, if both paddles are released then the elements being sent
//   is completed and then nothing more. This seems to be the most logical
//   thing to do.
//   There exists, however the mode Iambic-B. If both paddles are released
//   while sending a dah, then the following dit will also be sent. This is
//   not the place discussing this mode, simply because there are people
//   used to it.
//
//   Ultimatic mode
//   --------------
//
//   There exists a non-iambic mode for paddles. Briefly, if both contacts
//   are closed, the last one to be closed takes control, and a series
//   of dots or dashes is sent. If the last-closed contact is released
//   while the first one is still closed, the first one gains control
//   again. So to send a letter "X", close the dash paddle first, the
//   dot paddle shortly thereafter, and release the dot paddle as
//   soon you hear the second dot.
//
//   Note the K1EL chip implements dit/dah "priorities", that is, not the last-closed
//   contac "wins" but it can be chosen whether dit or dah "wins" when both contacts
//   are closed. This is *not* implemented here.
//
//   Bug mode
//   --------
//
//   In this mode, the dot paddle is treated normally but the dash paddle like a straight
//   key. And this is exactly how bug mode is implemented here (treating dash paddle
//   contacts close/open like straight key contacts close/open)
//
//   The modes are determined by the bits 4 and 5 of the ModeRegister,
//   00 = Iambic-B, 01 = Iambic-A, 10 = Ultimatic, 11 = Bug
//
//
////////////////////////////////////////////////////////////////////////////////////////
//
//
// The whole thing is programmed as (independent) state machines,
//
// keyer state machine:
//  - handles the paddle/straight key and produces the dits and dahs,
//  - if the paddle is free it transmits characters from the character ring buffer
//
// WinKey state machine:
// - handles the serial line and emulates a Winkey (K1EL chip)
//   device. It "talks" with the keyer by putting characters in the character
//   riung buffer
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// Eliminate incompatible options and set defaults
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AVR__
// deep sleep mode currently implemented only for AVR architecture
#undef  POWERSAVE
#endif

#ifdef CWKEYERSHIELD

#include "CWKeyerShield.h"
#undef USBMIDI              // handled in the KeyerShield library
#undef MIDIUSB
#undef POTPIN               // handled in the KeyerShield library
#undef TEENSY4AUDIO         // audio handled in the KeyerShield library
////////////////////////////////////////////////////////////////////////////////////////
//
// set KeyerShield variables that are *not* defined in config.h to -1
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef SHIELD_ANALOG_SIDETONEVOLUME
#define SHIELD_ANALOG_SIDETONEVOLUME   -1
#endif
#ifndef SHIELD_ANALOG_SIDETONEFREQ
#define SHIELD_ANALOG_SIDETONEFREQ     -1
#endif
#ifndef SHIELD_ANALOG_MASTERVOLUME
#define SHIELD_ANALOG_MASTERVOLUME     -1
#endif
#ifndef SHIELD_ANALOG_SPEED
#define SHIELD_ANALOG_SPEED            -1
#endif
#ifndef SHIELD_DIGITAL_MICPTT
#define SHIELD_DIGITAL_MICPTT          -1
#endif
#ifndef SHIELD_DIGITAL_PTTOUT
#define SHIELD_DIGITAL_PTTOUT          -1
#endif
#ifndef SHIELD_DIGITAL_CWOUT
#define SHIELD_DIGITAL_CWOUT           -1
#endif

#endif // CWKEYERSHIELD

//
// USBMIDI (Teensy) and MIDIUSB (Leonardo) are mutually exclusive
//
#ifdef USBMIDI
#undef MIDIUSB
#endif

//
// Some defaults for MIDI channel and notes. These are taken only
// if *not* defined in the config.h file.
//
#ifndef MY_MIDI_CHANNEL
#define MY_MIDI_CHANNEL 5
#endif
#ifndef MY_KEYDOWN_NOTE
#define MY_KEYDOWN_NOTE 1
#endif
#ifndef MY_PTT_NOTE
#define MY_PTT_NOTE 2
#endif
#ifndef MY_SPEED_CTL
#define MY_SPEED_CTL 3
#endif


//
// keyer state machine: the states
//
enum KSTAT {
      CHECK=0,        // idle
      SENDDOT,        // sending a dot
      SENDDASH,       // sending a dash
      DOTDELAY,       // pause following a dot
      DASHDELAY,      // pause following a dash
      STARTDOT,       // aquire PTT and start dot
      STARTDASH,      // aquire PTT and start dash
      STARTSTRAIGHT,  // aquire PTT and send "key-down" message
      SENDSTRAIGHT,   // wait for releasing the straight key and send "key-up" message
      SNDCHAR_PTT,    // aquire PTT, key-down
      SNDCHAR_ELE,    // wait until end of element (dot or dash), key-up
      SNDCHAR_DELAY   // wait until end of delay (inter-element or inter-word)
      } keyer_state=CHECK;

//
// WinKey state machine: the states
// The numerical (enum) values ADMIN ... BUFNOP must agree with the
// winkey command bytes.
//
#define WKVERSION 23  // This version is returned to the host


enum WKSTAT {
  ADMIN       =0x00,
  SIDETONE    =0x01,
  WKSPEED     =0x02,
  WEIGHT      =0x03,
  PTT         =0x04,
  POTSET      =0x05,
  PAUSE       =0x06,
  GETPOT      =0x07,
  BACKSPACE   =0x08,
  PINCONFIG   =0x09,
  CLEAR       =0x0a,
  TUNE        =0x0b,
  HSCW        =0x0c,
  FARNS       =0x0d,
  WK2MODE     =0x0e,
  LOADDEF     =0x0f,
  EXTENSION   =0x10,
  KEYCOMP     =0x11,
  PADSW       =0x12,
  NULLCMD     =0x13,
  SOFTPAD     =0x14,
  WKSTAT      =0x15,
  POINTER     =0x16,
  RATIO       =0x17,
  SETPTT      =0x18,
  KEYBUF      =0x19,
  WAIT        =0x1a,
  PROSIGN     =0x1b,
  BUFSPD      =0x1c,
  HSCWSPD     =0x1d,
  CANCELSPD   =0x1e,
  BUFNOP      =0x1f,
  FREE,
  SWALLOW,
  XECHO,
  WRPROM,
  RDPROM,
  MESSAGE,
  POINTER_1,
  POINTER_2,
  POINTER_3
} winkey_state=FREE;

enum ADMIN_COMMAND {
  ADMIN_CALIBRATE  =  0,
  ADMIN_RESET      =  1,
  ADMIN_OPEN       =  2,
  ADMIN_CLOSE      =  3,
  ADMIN_ECHO       =  4,
  ADMIN_PAD_A2D    =  5,
  ADMIN_SPD_A2D    =  6,
  ADMIN_GETVAL     =  7,
  ADMIN_DEBUG      =  8,  // K1EL Debug use only
  ADMIN_GETMAJOR   =  9,  // Firmware major version
  ADMIN_SETWK1     = 10,  // Disables pushbutton reporting
  ADMIN_SETWK2     = 11,  // Pushbutton reporting enabled, alternate WK status mode
  ADMIN_DUMPEEPROM = 12,
  ADMIN_LOADEEPROM = 13,
  ADMIN_SENDMSG    = 14,
  ADMIN_LOADX1     = 15,
  ADMIN_FWUPDT     = 16,
  ADMIN_LOWBAUD    = 17,
  ADMIN_HIGHBAUD   = 18,
  ADMIN_RTTY       = 19, // Set RTTY registers, WK3.1 only
  ADMIN_SETWK3     = 20, // Set WK3 mode
  ADMIN_VCC        = 21, // Read Back Vcc
  ADMIN_LOADX2     = 22, // WK3 only
  ADMIN_GETMINOR   = 23, // Firmware minor version, WK3 only
  ADMIN_GETTYPE    = 24, // Get IC Type, WK3 only
  ADMIN_VOLUME     = 25  // Set side tone volume low/high, WK3 only
};


//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EEPROM variables
//
// The EEPROM layout of the K1EL chip. The first 24 bytes encode
// settings etc., from addr=24 to addr=255 is the storage for the
// messages (unused in this program). We only use the EEPROM at
// addresses 1-14. Byte 0 is fixed (magic byte),
// bytes 15-23 are initialized with a reasonable
// constant and bytes 24-255 are left empty.
//
// ADDR  Name          Explanation
// =================================================================
//   0   Magic         Magic Byte (0xA5)
//   1   ModeRegister  see below
//   2   Speed         in wpm, if pot not used
//   3   Sidetone      0x05 encodes 800 Hz, see WK manual
//   4   Weight        used to modify dit/dah length
//   5   PTT LeadIn    PTT lead-in time in units of 10 msec
//   6   PTT Tail      PTT tail time in units of 10 msec
//   7   MinWPM        WPM when speed pot is at minimum
//   8   WpmRange      WPM range of speed pot
//   9   Extension     used to elongate first dit/dah after PTT
//  10   Compensation  Key compensation
//  11   Farnsworth    Farnsworth speed (10 = no Farnsworth)
//  12   PaddlePoint   Paddle Switch-point (default: 50)
//  13   Ratio         Dah/Dit ratio (50 = 3:1)
//  14   PinConfig     see below
//  15   k12ext        special settings (additional letter space, zero cuts, etc.)
//  16   CmdWpm        Speed (WPM) for commands (unused, default: 15 wpm)
//  17   FreePtr       first free position in character buffer (0x18 if no messages are stored)
//  18   Msg1          Start addr of message #1  (0 if empty)
//  19   Msg2          Start addr of message #2  (0 if empty)
//  20   Msg3          Start addr of message #3  (0 if empty)
//  21   Msg4          Start addr of message #4  (0 if empty)
//  22   Msg5          Start addr of message #5  (0 if empty)
//  23   Msg6          Start addr of message #6  (0 if empty)
//
// bits used in PinConfig:
//   b0:    PTT enable/disable bit, if PTT disabled, LeadIn times will have no effect
//   b1:    Sidetone enable/disable bit
//   b4/b5: PTT hang time = word space + 8/4/2/1 dits (11,10,01,00)
//
// bits used in ModeRegister:
//   b6:    echo characters entered by paddle or straight key (to serial line)
//   b4/5:  Paddle mode (00 = Iambic-B, 01 = Iambic-A, 10 = Ultimatic, 11 = Bugmode)
//   b3:    swap paddles
//   b2:    echo characters received from the serial line when they are keyed
//   b0:    use contest (CT) spacing
//

static uint8_t MAGIC=0xA5;              // addr=0x00 // EEPROM magic byte
static uint8_t ModeRegister=0x10;       // addr=0x01 // Iambic-A by default
static uint8_t Speed=21;                // addr=0x02 // overridden by the speed pot in standalong mode
static uint8_t Sidetone=5;              // addr=0x03 // 800 Hz
static uint8_t Weight=50;               // addr=0x04 // used to modify dit/dah length
static uint8_t LeadIn=0;                // addr=0x05 // PTT Lead-in time (in units of 10 ms)
static uint8_t Tail=0;                  // addr=0x06 // PTT tail (in 10 ms), zero means "use hang bits"
static uint8_t MinWPM=8;                // addr=0x07 // CW speed when speed pot maximally CCW
static uint8_t WPMrange=20;             // addr=0x08 // CW speed range for SpeedPot
static uint8_t Extension=0;             // addr=0x09 // ignored in this code (1st extension)
static uint8_t Compensation=0;          // addr=0x0a // Used to modify dit/dah lengths
static uint8_t Farnsworth=10;           // addr=0x0b // Farnsworth speed (10 means: no Farnsworth)
static uint8_t PaddlePoint=50;          // addr=0x0c // ignored in this code (Paddle Switchpoint setting)
static uint8_t Ratio=50;                // addr=0x0d // dah/dit ratio = (3*ratio)/50
static uint8_t PinConfig=0x0E;          // addr=0x0e // PTT disabled
                                        // addr=0x0f // k12ext (Letter space, zero cuts, etc.)
                                        // addr=0x10 // CmdWpm
                                        // addr=0x11 // FreePtr
                                        // addr=0x12 // MsgPtr1
                                        // addr=0x13 // MsgPtr2
                                        // addr=0x14 // MsgPtr3
                                        // addr=0x15 // MsgPtr4
                                        // addr=0x16 // MsgPtr5
                                        // addr=0x17 // MsgPtr6

//
// Macros to read the ModeRegister
//
#define PADDLE_SWAP   (ModeRegister & 0x08)
#define PADDLE_ECHO   (ModeRegister & 0x40)
#define SERIAL_ECHO   (ModeRegister & 0x04)

#define IAMBIC_A      ((ModeRegister & 0x30) == 0x10)
#define IAMBIC_B      ((ModeRegister & 0x30) == 0x00)
#define BUGMODE       ((ModeRegister & 0x30) == 0x30)
#define ULTIMATIC     ((ModeRegister & 0x30) == 0x20)

#define USE_CT        (ModeRegister & 0x01)

//
// Macros to read PinConfig
//
#define SIDETONE_ENABLED (PinConfig & 0x02)
#define PTT_ENABLED      (PinConfig & 0x01)
#define HANGBITS         ((PinConfig & 0x30) >> 4)


static uint8_t BufSpeed=0;              // "Buffered" speed, if nonzero
static uint8_t HostSpeed=0;             // Speed from host in host-mode.
static uint8_t WKstatus=0xC0;           // reported to host when it changes

static uint16_t inum;                   // counter for number of bytes received in a given state
static uint8_t  highbaud=0;             // flag, set if we use high baud rate

static uint8_t ps1;                     // first byte of a pro-sign
static uint8_t pausing=0;               // "pause" state
static uint8_t breakin=1;               // breakin state
static uint8_t straight=0;              // state of the straight key (1 = pressed, 0 = released)
static uint8_t tuning=0;                // "Tune" mode active, deactivate paddle
static uint8_t hostmode  = 0;           // host mode
static uint8_t SpeedPot =  0;           // Speed value from the Potentiometer
static uint16_t myfreq=800;             // current side tone frequency


static uint8_t prosign=0;               // set if we are in the middle of a prosign
static uint8_t num_elements=0;          // number of elements sent in a sequence

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ring buffer for characters queued for sending
//
#define BUFLEN 128     // number of bytes in buffer (much larger than in K1EL chip)
#define BUFMARGIN 85   // water mark for reporting "buffer almost full"

static unsigned char character_buffer[BUFLEN];  // circular buffer
static uint8_t bufrx=0;                         // output (read) pointer
static uint8_t buftx=0;                         // input (write) pointer
static uint8_t bufcnt=0;                        // number of characters in buffer
//
//////////////////////////////////////////////////////////////////////////////////////////////////////


//
// "sending" encodes the actual character being sent from the character_buffer
// a value of 1 means "nothing to do"
//
static uint8_t sending=0x01;    // bitmap for current morse character to be sent
static uint8_t collecting=0;    // bitmap for character entered with the paddle
static uint8_t collpos=0;       // position for collecting
static uint8_t sentspace=1;     // space already sent for inter-word distance
static uint8_t ReplayPointer=0; // This indicates a message is being sent
static unsigned long wait=0;    // when "actual" reaches this value terminate current keyer state
static unsigned long last=0;    // time of last enddot/enddash
static unsigned long actual;    // time-stamp for this execution of loop()
#ifdef POWERSAVE
static unsigned long watchdog;  // used for going to sleep
#endif

//
// variables updated in interrupt service routine
//
static uint8_t kdot = 0;      // This variable reflects the value of the dot paddle
static uint8_t kdash = 0;     // This value reflects the value of the dash paddle
static uint8_t memdot=0;      // set, if dot paddle hit since the beginning of the last dash
static uint8_t memdash=0;     // set,  if dash paddle hit since the beginning of the last dot
static uint8_t lastpressed=0; // Indicates which paddle was pressed last (for ULTIMATIC)
static uint8_t eff_kdash;     // effective kdash (may be different from kdash in BUG and ULTIMATIC mode)
static uint8_t eff_kdot;      // effective kdot  (may be different from kdot in ULTIMATIC mode)

static uint8_t dash_held=0;  // dot paddle state at the beginning of the last dash
static uint8_t dot_held=0;   // dash paddle state at the beginning of the last dot

static uint8_t ptt_stat=0;   // current PTT status
static uint8_t cw_stat=0;    // current CW output line status

#ifdef CWKEYERSHIELD

//
// Create an instance of the KeyerShield class.
// Implement some "callback" functions by which the KeyerShield classs
// can change keyer settings.
//
void speed_set(int s) {
  //
  // Interface to let the TeensyUSBAudioMidi class
  // set the CW speed. Such a change may arise from
  // a speed pot monitored by TeensyUSBAudioMidi or
  // from a MIDI message arriving there.
  //
  if (s <  1) s= 1;
  if (s > 99) s=99;
  Speed=s;
}

void keyer_autoptt_set(int e) {
  //
  // Interface to let the TeensyUSBAudioMidi class
  // set the "use PTT" flag.
  //
  if (e) {
    PinConfig |= 0x01;
  } else {
    PinConfig &= 0xFE;
  }
}

void keyer_leadin_set(int t) {
  //
  // Interface to let the TeensyUSBAudioMidi class
  // set the lead-in time, (t*10) milli-seconds.
  //
  if (t > 50) t=50;
  if (t <  0) t= 0;
  LeadIn = t;
}

void keyer_hang_set(int h) {
  //
  // Interface to let the TeensyUSBAudioMidi class
  // set the "PTT hang" time. The time is in
  // dot-lengths so it varies with the CW speed.
  //
  // Note: WinKeyer encodes 8/9/11/15 dot lengths in HANGBITS
  //       so we map all possible values of h onto these four
  //       possibilities
  //
  PinConfig &= 0x30;                         //  8 dot lengths hang time
  if (h >=  9  && h <10) PinConfig |= 0x10;  //  9 dot lengths hang time
  if (h >= 11  && h <13) PinConfig |= 0x20;  // 11 dot lengths hang time
  if (h >= 14)           PinConfig |= 0x30;  // 15 dot lengths hang time
}

CWKeyerShield cwshield(SHIELD_AUDIO_OUTPUT,
                       SHIELD_ANALOG_SIDETONEVOLUME,
                       SHIELD_ANALOG_SIDETONEFREQ,
                       SHIELD_ANALOG_MASTERVOLUME,
                       SHIELD_ANALOG_SPEED,
                       SHIELD_DIGITAL_MICPTT,
                       SHIELD_DIGITAL_PTTOUT,
                       SHIELD_DIGITAL_CWOUT);

#endif

void init_eeprom();

#ifdef TEENSY4AUDIO

#include "Audio.h"
#include "utility/dspinst.h"

//
// Sine oscillator with off/on and pulse shaper
// All is integrated into a monolithic block
// to minimize computational overhead.
// The only methods are on/off and setting the frequency.
// A stereo output with both channels being equal is produced.
//
class SideToneSource : public AudioStream
{
public:
    SideToneSource() : AudioStream(0, NULL),
                       tone(0),  phase(0), incr(0),
                       rampindex(0) {}

    virtual void update(void);

    void set_frequency(int frequency) {
      if (frequency < 0) frequency = 0;
      if (frequency > 8000) frequency = 8000;
      incr = (frequency * 4294967296.0) / AUDIO_SAMPLE_RATE_EXACT;
    }

    void onoff(uint8_t state) {
        tone = state;
    }

private:
    uint8_t  tone;         // tone on/off flag
    uint32_t phase;
    uint32_t incr;
    uint8_t  rampindex;  // pointer into the "ramp"

    //
    // Both the Ramp and the Sine table have been produces with MATHEMATICA
    // The final value for the audio system must be int32_t and can be
    // calculated as ramp (uint32_t) * sintab (int32_t)
    //
    // The Blackman-Harris-Ramp has a width of 5 msec
    //
    static constexpr int RAMP_LENGTH=240;
    static constexpr uint16_t BlackmanHarrisRamp[RAMP_LENGTH] = {
      0,      0,      0,      0,      1,      1,      1,      2,
      2,      3,      4,      5,      7,      9,     11,     13,
     16,     20,     23,     28,     33,     39,     46,     54,
     63,     72,     83,     96,    110,    125,    142,    161,
    182,    205,    231,    259,    290,    323,    360,    400,
    443,    490,    541,    596,    656,    720,    789,    864,
    943,   1029,   1120,   1218,   1322,   1433,   1551,   1677,
   1810,   1952,   2101,   2260,   2427,   2603,   2789,   2984,
   3190,   3405,   3632,   3869,   4117,   4377,   4648,   4931,
   5226,   5533,   5853,   6185,   6529,   6887,   7257,   7641,
   8038,   8448,   8871,   9308,   9758,  10221,  10698,  11188,
  11691,  12207,  12736,  13277,  13832,  14399,  14978,  15569,
  16172,  16786,  17412,  18048,  18695,  19352,  20019,  20695,
  21380,  22073,  22775,  23484,  24200,  24922,  25651,  26385,
  27124,  27868,  28615,  29366,  30119,  30874,  31631,  32389,
  33146,  33904,  34661,  35416,  36169,  36920,  37667,  38411,
  39150,  39884,  40613,  41335,  42051,  42760,  43462,  44155,
  44840,  45516,  46183,  46840,  47487,  48123,  48749,  49363,
  49966,  50557,  51136,  51703,  52258,  52799,  53328,  53844,
  54347,  54837,  55314,  55777,  56227,  56664,  57087,  57497,
  57894,  58278,  58648,  59006,  59350,  59682,  60002,  60309,
  60604,  60887,  61158,  61418,  61666,  61903,  62130,  62345,
  62551,  62746,  62932,  63108,  63275,  63434,  63583,  63725,
  63858,  63984,  64102,  64213,  64317,  64415,  64506,  64592,
  64671,  64746,  64815,  64879,  64939,  64994,  65045,  65092,
  65135,  65175,  65212,  65245,  65276,  65304,  65330,  65353,
  65374,  65393,  65410,  65425,  65439,  65452,  65463,  65472,
  65481,  65489,  65496,  65502,  65507,  65512,  65515,  65519,
  65522,  65524,  65526,  65528,  65530,  65531,  65532,  65533,
  65533,  65534,  65534,  65534,  65535,  65535,  65535,  65535
};

    static constexpr int SinTabLen = 257;
    static constexpr int16_t SineTab[SinTabLen] = {
        0,      804,     1608,     2410,     3212,     4011,     4808,     5602,
     6393,     7179,     7962,     8739,     9512,    10278,    11039,    11793,
    12539,    13279,    14010,    14732,    15446,    16151,    16846,    17530,
    18204,    18868,    19519,    20159,    20787,    21403,    22005,    22594,
    23170,    23731,    24279,    24811,    25329,    25832,    26319,    26790,
    27245,    27683,    28105,    28510,    28898,    29268,    29621,    29956,
    30273,    30571,    30852,    31113,    31356,    31580,    31785,    31971,
    32137,    32285,    32412,    32521,    32609,    32678,    32728,    32757,
    32767,    32757,    32728,    32678,    32609,    32521,    32412,    32285,
    32137,    31971,    31785,    31580,    31356,    31113,    30852,    30571,
    30273,    29956,    29621,    29268,    28898,    28510,    28105,    27683,
    27245,    26790,    26319,    25832,    25329,    24811,    24279,    23731,
    23170,    22594,    22005,    21403,    20787,    20159,    19519,    18868,
    18204,    17530,    16846,    16151,    15446,    14732,    14010,    13279,
    12539,    11793,    11039,    10278,     9512,     8739,     7962,     7179,
     6393,     5602,     4808,     4011,     3212,     2410,     1608,      804,
        0,     -804,    -1608,    -2410,    -3212,    -4011,    -4808,    -5602,
    -6393,    -7179,    -7962,    -8739,    -9512,   -10278,   -11039,   -11793,
   -12539,   -13279,   -14010,   -14732,   -15446,   -16151,   -16846,   -17530,
   -18204,   -18868,   -19519,   -20159,   -20787,   -21403,   -22005,   -22594,
   -23170,   -23731,   -24279,   -24811,   -25329,   -25832,   -26319,   -26790,
   -27245,   -27683,   -28105,   -28510,   -28898,   -29268,   -29621,   -29956,
   -30273,   -30571,   -30852,   -31113,   -31356,   -31580,   -31785,   -31971,
   -32137,   -32285,   -32412,   -32521,   -32609,   -32678,   -32728,   -32757,
   -32767,   -32757,   -32728,   -32678,   -32609,   -32521,   -32412,   -32285,
   -32137,   -31971,   -31785,   -31580,   -31356,   -31113,   -30852,   -30571,
   -30273,   -29956,   -29621,   -29268,   -28898,   -28510,   -28105,   -27683,
   -27245,   -26790,   -26319,   -25832,   -25329,   -24811,   -24279,   -23731,
   -23170,   -22594,   -22005,   -21403,   -20787,   -20159,   -19519,   -18868,
   -18204,   -17530,   -16846,   -16151,   -15446,   -14732,   -14010,   -13279,
   -12539,   -11793,   -11039,   -10278,    -9512,    -8739,    -7962,    -7179,
    -6393,    -5602,    -4808,    -4011,    -3212,    -2410,    -1608,     -804,
    0
};

};

void SideToneSource::update() {
  audio_block_t *block;
  //if (tone || rampindex) {
    block = allocate();
    if (block) {
      uint32_t ph = phase;  // use local variable to allow for compiler optimization
      uint32_t in = incr;   // use local variable to allow for compiler optimization
      for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
        int ind = ph >> 24;                  // bits 24-31 of phase: index to SineTab
        uint32_t scal = (ph >> 8) & 0xFFFF;  // bits 8-16  of phase: used for interpolation
        ph += in;                            // increment phase
        int32_t val1 = SineTab[ind];         // left  edge value, Range: -32767 ... 32767
        int32_t val2 = SineTab[ind+1];       // right edge value, Range: -32767 ... 32767
        val2 *= scal;
        val1 *= (65536 - scal);              // val1 + val2 is the interpolated value -2^31 ... 2^31
        //
        // We must use upper 16 bits of val1+val2 if we have climbed the ramp.
        // Within the ramp, take ((val1+val2)*ramp) >> 32
        //
        if (tone) {
          if (rampindex < RAMP_LENGTH) {
            // key-down, still climbing the ramp
            uint16_t ramp = BlackmanHarrisRamp[rampindex++];
            block->data[i] = multiply_32x32_rshift32(val1 + val2, ramp);
          } else {
            // key-down, max. amplitude reached
            block->data[i] = (val1 + val2) >> 16;
          }
        } else if (rampindex) {
          // key-up but still descending the ramp
          uint16_t ramp = BlackmanHarrisRamp[--rampindex];
          block->data[i] = multiply_32x32_rshift32(val1 + val2, ramp);
        } else {
          // key-down and pulse completed
          block->data[i]=0;
        }
      }
      phase = ph;
      //
      // transmit the side tone to left and right channel, then release
      //
      transmit(block, 0);
      transmit(block, 1);
      release(block);
    }
  //}
}

AudioOutputI2S           i2s;                           // audio output
AudioControlSGTL5000     sgtl5000;                      // controller for SGTL volume etc.
SideToneSource           sidetone;                      // our side tone generator


#endif



//////////////////////////////////////////////////////////////////////////////
//
// setup:
// Initialize serial port and hardware lines
// init eeprom or load settings from eeprom
// init Audio+MIDI
//
//////////////////////////////////////////////////////////////////////////////

void setup() {

#ifdef POWERSAVE
//
// Set watchdog to current time
//
   watchdog=millis();
#endif
//
// Configure serial line if WinKey protocol is used
//
#ifdef MYSERIAL
  MYSERIAL.begin(1200);  // baud rate has no meaning  for "true" USB-Serial connections
  highbaud=0;
#endif

//
// Configure digital input (StraightKey, PaddleLeft, PaddleRight) and
// digital output (CW1, CW2, PTT1, PTT2, TONEPIN) lines
//
#ifdef StraightKey
  pinMode(StraightKey, INPUT_PULLUP);
#endif

#if defined(PaddleLeft) && defined(PaddleRight)
  pinMode(PaddleLeft,  INPUT_PULLUP);
  pinMode(PaddleRight, INPUT_PULLUP);
#endif

#ifdef CW1
  // active-high CW output
  pinMode(CW1, OUTPUT);
  digitalWrite(CW1, LOW);
#endif
#ifdef CW2
  // active-low CW output
  pinMode(CW2, OUTPUT);
  digitalWrite(CW2, HIGH);
#endif
#ifdef PTT1         // active-high PTT output
  pinMode(PTT1, OUTPUT);
  digitalWrite(PTT1, LOW);
#endif
#ifdef PTT2         //active-low PTT output
  pinMode(PTT2, OUTPUT);
  digitalWrite(PTT2, HIGH);
#endif
#ifdef TONEPIN
  pinMode(TONEPIN, OUTPUT);
  digitalWrite(TONEPIN, LOW);
#endif

  init_eeprom();

#ifdef CWKEYERSHIELD

 //
 // Initialize the KeyerShield  library, and set some defaults.
 //
 cwshield.setup();

#ifdef MY_MUTE_OPTION
  cwshield.set_cwptt_mute_option(MY_MUTE_OPTION);
#endif
#ifdef MY_MIDI_CHANNEL
  cwshield.set_midi_channel(MY_MIDI_CHANNEL);
#endif
#ifdef  MY_KEYDOWN_NOTE
  cwshield.set_midi_keydown_note(MY_KEYDOWN_NOTE);
#endif
#ifdef MY_PTT_NOTE
  cwshield.set_midi_ptt_note(MY_PTT_NOTE);
#endif
#ifdef MY_DEFAULT_FREQ
  cwshield.sidetonefrequency(MY_DEFAULT_FREQ/10);
#endif
#ifdef MY_DEFAULT_SIDETONE_VOLUME
  cwshield.sidetonevolume(MY_DEFAULT_SIDETONE_VOLUME);
#endif
#ifdef MY_DEFAULT_MASTER_VOLUME
  cwshield.mastervolume(MY_DEFAULT_MASTER_VOLUME);  // 0..127
#endif

#endif  // CWKEYERSHIELD


#ifdef TEENSY4AUDIO
//
// Init audio subsystem and set default parameters
//
AudioMemory(32);
AudioNoInterrupts();

sidetone.set_frequency(400);  // Initial setting (has no effect)
sgtl5000.enable();       // Enable I2S output
sgtl5000.volume(0.40);   // SGTL master volume. Adjust to your hardware

(void) new AudioConnection(sidetone, 0, i2s, 0);
(void) new AudioConnection(sidetone, 1, i2s, 1);

AudioInterrupts();


#endif  // TEENSY4AUDIO

}

//////////////////////////////////////////////////////////////////////////////
//
// read_from_eeprom:
// if magic bytes in the eeprom are set: read settings from eeprom
//
//////////////////////////////////////////////////////////////////////////////

void read_from_eeprom() {
//
// Called upons startup and from an Admin:Reset command.
//
// If EEPROM contains valid data (magic byte at 0x00),
// init variables from EEPROM.
//
// If magic byte is not found, nothing is done.
//
  uint8_t i;


  if (EEPROM.read(0) == MAGIC) {
     ModeRegister = EEPROM.read( 1);
     i            = EEPROM.read( 2);
     if (i >= 5 && i <= 65) Speed=i;
     Sidetone     = EEPROM.read( 3);
     Weight       = EEPROM.read( 4);
     LeadIn       = EEPROM.read( 5);
     Tail         = EEPROM.read( 6);
     MinWPM       = EEPROM.read( 7);
     WPMrange     = EEPROM.read( 8);
     Extension    = EEPROM.read( 9);
     Compensation = EEPROM.read(10);
     Farnsworth   = EEPROM.read(11);
     PaddlePoint  = EEPROM.read(12);
     Ratio        = EEPROM.read(13);
     PinConfig    = EEPROM.read(14);
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// write_to_eeprom:
// write magic bytes and current settings to eeprom
//
// This is currently ONLY used to write compile-time default settings into
// a "virgin" EEPROM. Therefore, we also give some default values to the
// unused parameters.
//
// The EEPROM is further updated by the "LOAD EEPROM" admin command.
//
//////////////////////////////////////////////////////////////////////////////

void write_to_eeprom() {
    EEPROM.update( 0, MAGIC);           // This position will not be modified
    EEPROM.update( 1, ModeRegister);
    EEPROM.update( 2, Speed);
    EEPROM.update( 3, Sidetone);
    EEPROM.update( 4, Weight);
    EEPROM.update( 5, LeadIn);
    EEPROM.update( 6, Tail);
    EEPROM.update( 7, MinWPM);
    EEPROM.update( 8, WPMrange);
    EEPROM.update( 9, Extension);
    EEPROM.update(10, Compensation);
    EEPROM.update(11, Farnsworth);
    EEPROM.update(12, PaddlePoint);
    EEPROM.update(13, Ratio);
    EEPROM.update(14, PinConfig);
    EEPROM.update(15, 0);      // default k12ext
    EEPROM.update(16, 15);     // default CmdWpm
    EEPROM.update(17, 0x18);   // default FreePtr
    EEPROM.update(18, 0);      // default MsgPtr1
    EEPROM.update(19, 0);      // default MsgPtr2
    EEPROM.update(20, 0);      // default MsgPtr3
    EEPROM.update(21, 0);      // default MsgPtr4
    EEPROM.update(22, 0);      // default MsgPtr5
    EEPROM.update(23, 0);      // default MsgPtr6
}
//////////////////////////////////////////////////////////////////////////////
//
// init_eeprom:
// if eeprom magic byte is not set: write current setting to eeprom
// if eeprom magic byte is     set: read  settings from eeptorm
//
// called once upon startup
//////////////////////////////////////////////////////////////////////////////

void init_eeprom() {
  if (EEPROM.read(0) == MAGIC) {
    read_from_eeprom();
  } else {
    write_to_eeprom();
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Some utility functions when using MIDI
//
//////////////////////////////////////////////////////////////////////////////
#ifdef USBMIDI
//
// This works for USBMIDI (Teensy)
//
void SendOnOff(int chan, int note, int state) {
  if (chan < 0 || note < 0) return;
  chan = chan & 0x0F;
  if (state) {
    usbMIDI.sendNoteOn(note, 127, chan);
  } else {
    usbMIDI.sendNoteOn(note,   0, chan);
  }
  usbMIDI.send_now();
}

void SendControlChange(int chan, int control, int val) {
  if (chan < 0 || control < 0) return;
  usbMIDI.sendControlChange(control, val, chan);
  usbMIDI.send_now();
}
#else
#ifdef MIDIUSB
//
// This works for MIDIUSB (Leonardo)
//
#include "MIDIUSB.h"

void SendOnOff(int chan, int note, int state) {
  midiEventPacket_t event;
  if (chan < 0 || note < 0) return;
  chan = chan & 0x0F;
  if (state) {
    // Note On
    event.header = 0x09;
    event.byte1  = 0x90 | chan;
    event.byte2  = note;
    event.byte3  = 127;
  } else {
    // NoteOff, but we use NoteOn with velocity=0
    event.header = 0x09;
    event.byte1  = 0x90 | chan;
    event.byte2  = note;
    event.byte3  = 0;
  }
  MidiUSB.sendMIDI(event);
  // this is CW, so flush each single event
  MidiUSB.flush();
}

void SendControlChange(int chan, int control, int val) {
  midiEventPacket_t event;
  if (chan < 0 || control < 0) return;
  chan = chan & 0x0F;
  event.header = 0x09;
  event.byte1  = 0xB0 | chan;
  event.byte2  = control & 0x7F;
  event.byte3  = val & 0x7F;
  MidiUSB.sendMIDI(event);
  // this is CW, so flush each single event
  MidiUSB.flush();
}
#else
//
// Dummy functions if there is no MIDI
//
void SendOnOff(int chan, int note, int state) {
}
void SendControlChange(int chan, int control, int val) {
}
#endif
#endif
//////////////////////////////////////////////////////////////////////////////
//
// MIDI might require to drain or process incoming MIDI messages, so this
// one is periodically called. If neither using the KeyerShield library, nor
// using MIDI, then this is a no-op.
//
//////////////////////////////////////////////////////////////////////////////

void DrainMIDI() {
#ifdef CWKEYERSHIELD
    cwshield.loop();
#endif
#ifdef MIDIUSB
    midiEventPacket_t rx;
    do {
      rx = MidiUSB.read();
    } while (rx.header != 0);
#endif
#ifdef USBMIDI
    if (usbMIDI.read()) {
      // nothing to be done
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Read one byte from host.
// If not using the serial line, this always returns 0.
//
//////////////////////////////////////////////////////////////////////////////

int FromHost() {
  int c=0;
#ifdef MYSERIAL
  c=MYSERIAL.read();
#endif

  return c;
}

//////////////////////////////////////////////////////////////////////////////
//
// Write one byte to the host
// If not using the serial line, this is a no-op.
//
//////////////////////////////////////////////////////////////////////////////

void ToHost(int c) {
#ifdef MYSERIAL
  MYSERIAL.write(c);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Check if there is a byte from host
// If not using the serial line, this always returns 0.
//
//////////////////////////////////////////////////////////////////////////////

int ByteAvailable() {
  int rc=0;
#ifdef MYSERIAL
  rc=MYSERIAL.available();
#endif
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
// "key down" action
//
//////////////////////////////////////////////////////////////////////////////

void keydown() {
  if (cw_stat) return;
  cw_stat=1;
  //
  // Actions: side tone on (if enabled), set hardware line(s), send MIDI  message
  //
#ifdef TONEPIN
  tone(TONEPIN, myfreq);
#endif

#ifdef CW1                                              // active-high CW output
  digitalWrite(CW1,HIGH);
#endif

#ifdef CW2                                              //active-low CW line
  digitalWrite(CW2,LOW);
#endif

  SendOnOff(MY_MIDI_CHANNEL, MY_KEYDOWN_NOTE, 1);

#ifdef CWKEYERSHIELD                               // MIDI and sidetone
 cwshield.key(1);
#endif
#ifdef TEENSY4AUDIO
  if (SIDETONE_ENABLED) sidetone.onoff(1);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// "key up" action
//
//////////////////////////////////////////////////////////////////////////////

void keyup() {
  if (!cw_stat) return;
  cw_stat=0;
  //
  // Actions: side tone off, drop hardware line, send MIDI NoteOff message
  //

#ifdef TONEPIN
  noTone(TONEPIN);
#endif

#ifdef CW1                                              // active-high CW output
  digitalWrite(CW1,LOW);
#endif

#ifdef CW2
  digitalWrite(CW2,HIGH);                               // active-low CW output
#endif

  SendOnOff(MY_MIDI_CHANNEL, MY_KEYDOWN_NOTE, 0);

#ifdef CWKEYERSHIELD                               // MIDI and side tone
 cwshield.key(0);
#endif
#ifdef TEENSY4AUDIO
  sidetone.onoff(0);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// "PTT on" action
//
//////////////////////////////////////////////////////////////////////////////

void ptt_on() {
  if (ptt_stat) return;
  ptt_stat=1;
  //
  // Actions: raise hardware line, send MIDI NoteOn message
  //
#ifdef PTT1                                               // active high PTT line
  digitalWrite(PTT1,HIGH);
#endif

#ifdef PTT2                                               // active low PTT line
  digitalWrite(PTT2,LOW);
#endif


  SendOnOff(MY_MIDI_CHANNEL, MY_PTT_NOTE, 1);

#ifdef CWKEYERSHIELD
  cwshield.cwptt(1);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// "PTT off" action
//
//////////////////////////////////////////////////////////////////////////////

void ptt_off() {
  if (!ptt_stat) return;
  ptt_stat=0;
  //
  // Actions: drop hardware line, send MIDI NoteOff message
  //
#ifdef PTT1
  digitalWrite(PTT1,LOW);                                 // active high PTT line
#endif

#ifdef PTT2
  digitalWrite(PTT2,HIGH);                                // active low PTT line
#endif

  SendOnOff(MY_MIDI_CHANNEL, MY_PTT_NOTE, 0);

#ifdef CWKEYERSHIELD
  cwshield.cwptt(0);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// clear ring buffer, clear pausing state
//
//////////////////////////////////////////////////////////////////////////////

void clearbuf() {
  bufrx=buftx=bufcnt=0;
  pausing=0;
  BufSpeed=0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Insert zeroes, for pointer commands
//
//////////////////////////////////////////////////////////////////////////////

void bufzero(int len) {
  while (len--) {
    queue(1,0,0,0);
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Set buffer pointer to ABSOLUTE position
// for pointer commands
//
//////////////////////////////////////////////////////////////////////////////

void setbufpos(int pos) {
  buftx=pos;
}

//////////////////////////////////////////////////////////////////////////////
//
// queue up to 3 chars in character_buffer
// If there is not enough space to queue all of them, then queue none.
//
//////////////////////////////////////////////////////////////////////////////

void queue(int n, int a, int b, int c) {
  if (bufcnt + n > BUFLEN) return;
  character_buffer[buftx++]=a; if (buftx >= BUFLEN) buftx=0; bufcnt++;
  if (n > 1) {
    character_buffer[buftx++]=b; if (buftx >= BUFLEN) buftx=0; bufcnt++;
  }
  if (n > 2) {
    character_buffer[buftx++]=c; if (buftx >= BUFLEN) buftx=0; bufcnt++;
  }
  if (bufcnt > BUFMARGIN) WKstatus |= 0x01;
}

//////////////////////////////////////////////////////////////////////////////
//
// remove last queued character
//
//////////////////////////////////////////////////////////////////////////////

void backspace() {
  if (bufcnt < 1) return;
  buftx--;
  if (buftx < 0) buftx=BUFLEN-1;
  bufcnt--;
  if (bufcnt <= BUFMARGIN) WKstatus &= 0xFE;
}

//////////////////////////////////////////////////////////////////////////////
//
// get next character from character_buffer
//
//////////////////////////////////////////////////////////////////////////////

int FromBuffer() {
  int c;
  if (bufcnt < 1) return 0;
  c=character_buffer[bufrx++];
  if (bufrx >= BUFLEN) bufrx=0;
  bufcnt--;
  if (bufcnt <= BUFMARGIN) WKstatus &= 0xFE;
  return c;
}

//////////////////////////////////////////////////////////////////////////////
// Morse code for ASCII characters 33 - 90, from ITU-R M.1677-1
// read from bit0 to bit7
// 0 = dot, 1 = dash
// highest bit set indicated end-of-character (not to be sent as a dash!)
// This implies a character can have at most 7 elements.
//////////////////////////////////////////////////////////////////////////////

unsigned char morse[58] = {
   0x01,     //33 !    not in ITU-R M.1677-1
   0x52,     //34 "    .-..-.
   0x01,     //35 #    not in ITU-R M.1677-1
   0x01,     //36 $    not in ITU-R M.1677-1
   0x01,     //37 %    not in ITU-R M.1677-1
   0x01,     //38 &    not in ITU-R M.1677-1
   0x5E,     //39 '    .----.
   0x2D,     //40 (    -.--.
   0x6D,     //41 )    -.--.-
   0x01,     //42 *    not in ITU-R M.1677-1
   0x2A,     //43 +    .-.-.
   0x73,     //44 ,    --..--
   0x61,     //45 -    -....-
   0x6A,     //46 .    .-.-.-
   0x29,     //47 /    -..-.
   0x3F,     //48 0    -----
   0x3E,     //49 1    .----
   0x3C,     //50 2    ..---
   0x38,     //51 3    ...--
   0x30,     //52 4    ....-
   0x20,     //53 5    .....
   0x21,     //54 6    -....
   0x23,     //55 7    --...
   0x27,     //56 8    ---..
   0x2F,     //57 9    ----.
   0x47,     //58 :    ---...
   0x01,     //59 ;    not in ITU-R M.1677-1
   0x01,     //60 <    not in ITU-R M.1677-1
   0x31,     //61 =    -...-
   0x01,     //62 >    not in ITU-R M.1677-1
   0x4c,     //63 ?    ..--..
   0x56,     //64 @    .--.-.
   0x06,     //65 A    .-
   0x11,     //66 B    .---
   0x15,     //67 C    -.-.
   0x09,     //68 D    -..
   0x02,     //69 E    .
   0x14,     //70 F    ..-.
   0x0B,     //71 G    --.
   0x10,     //72 H    ....
   0x04,     //73 I    ..
   0x1E,     //74 J    .---
   0x0D,     //75 K    -.-
   0x12,     //76 L    .-..
   0x07,     //77 M    --
   0x05,     //78 N    -.
   0x0F,     //79 O    ---
   0x16,     //80 P    .--.
   0x1B,     //81 Q    --.-
   0x0A,     //82 R    .-.
   0x08,     //83 S    ...
   0x03,     //84 T    -
   0x0C,     //85 U    ..-
   0x18,     //86 V    ...-
   0x0E,     //87 W    .--
   0x19,     //88 X    -..-
   0x1D,     //89 Y    -.--
   0x13      //90 Z    --..
};

uint8_t ASCII_to_Morse(int c) {
  //
  // Convert Ascii character to dit/dah pattern
  //
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= 33 && c <= 90) {
    return morse[c-33];
  } else {
    return 1;
  }
}

uint8_t Morse_to_ASCII(int m) {
  // Convert dit/dah pattern to ASCII
  int i;
  for (i=33; i<=90; i++) {
    if (morse[i-33] == m) return i;
  }
  return 32; // notfound: return a space
}

///////////////////////////////////////
//
// This is the Keyer state machine
//
///////////////////////////////////////

void keyer_state_machine() {
  int i;                        // general counter variable
  uint8_t byte;                 // general one-byte variable
  uint16_t dotlen;              // length of dot (msec)
  uint16_t dashlen;             // length of dash (msec)
  uint16_t plen;                // length of delay between dits/dahs
  uint16_t clen;                // inter-character delay in addition to inter-element delay
  uint16_t wlen;                // inter-word delay in addition to inter-character delay
  uint16_t hang;                // PTT tail hang time
  uint8_t  myspeed;             // effective speed (from host, from pot, or buffered)

  static unsigned long straight_pressed;  // for timing straight key signals


  //
  // If a paddle or the straight key is hit:
  // -abort sending buffered characters (and clear the buffer)
  // -abort sending EEPROM messages
  // -set "breakin" flag (for WK2 status message)
  //
  if ((eff_kdash || eff_kdot || straight) && (keyer_state >= SNDCHAR_PTT)) {
    breakin=1;
    clearbuf();
    ReplayPointer=0;
    keyer_state=CHECK;
    wait=actual+10;      // will be re-computed soon
  }

  //
  // HostMode speed overrides "local" speed
  //
  if (HostSpeed == 0) {
    myspeed=Speed;
  } else {
    myspeed=HostSpeed;
  }

  static int old_myspeed=0;

  if (myspeed != old_myspeed) {
    old_myspeed=myspeed;
    SendControlChange(MY_MIDI_CHANNEL, MY_SPEED_CTL, myspeed);
  }


  //
  // If sending from the buffer, possibly use "buffered speed"
  //
  if (keyer_state >= SNDCHAR_PTT && BufSpeed != 0) myspeed=BufSpeed;


  //
  // Standard Morse code timing
  //
  dotlen =1200 / myspeed;          // dot length in milli-seconds
  dashlen=(3*Ratio*dotlen)/50;     // dash length
  plen   =dotlen;                  // inter-element pause
  clen   =2*dotlen;                // inter-character pause = 3 dotlen, one already done (inter-element)
  wlen   =4*dotlen;                // inter-word pause = 7 dotlen, 3 already done (inter-character)
  if (USE_CT) wlen = 3*dotlen;     // contest timing for inter-word pause

  //
  // Farnsworth timing: stretch inter-character and inter-word pauses
  //
  if (Farnsworth > 10 && Farnsworth < myspeed) {
    i=3158/Farnsworth -(31*dotlen)/19;    // based on PARIS timing, i > dotlen
    clen=3*i - dotlen;                    // stretched inter-character pause
    wlen=7*i - clen;                      // stretched inter-word pause
    if (USE_CT) wlen = 6*i - clen;        // contest timing for inter-word pause
  }

  //
  // If both Weighting and Compensation is used, the
  // dit/dah length depends on the order of the corrections
  // applied. Here first Weighting and then Compensation
  // is done, but note one should not use these options
  // at the same time anyway.
  //
  if (Weight != 50) {
    i = ((Weight-50)*dotlen)/50;
    dotlen += i;
    dashlen +=i;
    plen -= i;
  }
  if (Compensation != 0) {
    dotlen  += Compensation;
    dashlen += Compensation;
    plen    -= Compensation;
  }

  //
  // Note: new in WK2.3: tail and hang are independent
  // - hang-bits ONLY apply for CW from the paddle
  // - PTT-tail ONLY applies for auto-CW
  //

  switch (HANGBITS) {
    case 0: hang =  8*dotlen; break;     // word space + 1 dot
    case 1: hang =  9*dotlen; break;     // word space + 2 dots
    case 2: hang = 11*dotlen; break;     // word space + 4 dots
    case 3: hang = 15*dotlen; break;     // word space + 8 dots
  }

  switch (keyer_state) {
    case CHECK:
      // reset number of elements sent
      num_elements=0;
      // wait = time when PTT is switched off
      if (actual >= wait) ptt_off();
      if (collpos > 0 && actual > last + 2 * dotlen) {
        // a morse code pattern has been entered and the character is complete
        // echo it in ASCII on the display and on the serial line
        collecting |= 1 << collpos;
        if (PADDLE_ECHO && hostmode) {
          ToHost(Morse_to_ASCII(collecting));
        }
        collecting=0;
        collpos=0;
      }
      //
      // anything between one inter-word pause and "infinity pause" produces exactly one space
      // in the serial echo
      //
      if (collpos == 0 && sentspace == 0 && actual > last + 6*dotlen) {
         if (PADDLE_ECHO && hostmode) ToHost(32);
         sentspace=1;
      }
      //
      // At this point, process events in the order
      // straight > dot paddle > dash padddle > replay > send from buffer
      //
      if (straight) {
        sentspace=0;
        keyer_state=STARTSTRAIGHT;
        wait=actual;
        if (!ptt_stat && PTT_ENABLED) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
        break;
      }

      if (eff_kdot) {
        keyer_state=STARTDOT;
        collpos++;
        wait=actual;
        sentspace=0;
        if (!ptt_stat && PTT_ENABLED) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
        break;
      }

      if (eff_kdash) {
        wait=actual;
        sentspace=0;
        keyer_state=STARTDASH;
        collecting |= (1 << collpos++);
        if (!ptt_stat && PTT_ENABLED) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
        break;
      }

      if (ReplayPointer !=0) {
        pausing=0;
        clearbuf();
        sending = EEPROM.read(ReplayPointer++);
        if (sending & 0x80) {
          sending &= 0x7F;
          ReplayPointer=0;
        }
        //
        // The special "pattern" 0x1c is used to define a space.
        // When encountering a space and PTT is not set, activate PTT
        // but disregard lead-ins, since we are waiting for 4 dot-lengths
        // anyway.
        //
        if (sending == 0x1c) {
          sending=0x01;
          wait=actual+wlen;
          if (!ptt_stat && PTT_ENABLED) {
            ptt_on();
          }
          keyer_state=SNDCHAR_DELAY;
        } else {
          wait=actual;  // no lead-in wait by default
          if (!ptt_stat && PTT_ENABLED) {
            ptt_on();
            wait=actual+LeadIn*10;
          }
          keyer_state=SNDCHAR_PTT;
        }
        break;
      }

      //
      // If keyer is idle and buffered characters available, transfer next one to "sending"
      // Rely on the "completeness" of buffered commands
      // Note that when a character has been received such that we stay in the "CHECK" state,
      // wait must be set to actual+dotlen so we do not loose PTT while waiting for the next
      // character. This is important for programs that wait for the "serial echo" of any
      // character before sending the next one.
      //
      if (bufcnt > 0 && !pausing) {
        //
        // transfer next character to "sending"
        //
        byte=FromBuffer();
        if (byte >=32 && byte <=127 && SERIAL_ECHO) {
          ToHost(byte);
          wait=actual+dotlen; // host may wait for the byte before sending the next one
        }
        switch (byte) {
          case PROSIGN:
            prosign=1;
            break;
          case BUFNOP:
            break;
          case KEYBUF:        // ignored
          case WAIT:          // ignored
          case SETPTT:        // ignored
          case HSCWSPD:       // ignored
            byte=FromBuffer();
            break;
          case CANCELSPD:
            BufSpeed=0;
            break;
          case BUFSPD:
            byte=FromBuffer();
            BufSpeed=byte;
            if (BufSpeed < 5)  BufSpeed=5;
            if (BufSpeed > 99) BufSpeed=99;
            break;
          case 32:  // space
            sending=1;
            wait=actual + wlen;
            keyer_state=SNDCHAR_DELAY;
            break;
          //
          // PROTOCOL EXTENSION: Special treatment of "[", "$", and "]"
          // used for contest logging programs that are not aware of
          // the "buffered speed" facility. Typical CQ call is
          // CQ DL1YCF [TEST]
          // and a typical sent-exchange is
          // [5nn$tt1] or [5nn] $tt1]
          //
          // That is, "TEST" and "5nn" are sent at higher speed (40 wpm),
          // while the sent-exchange number is sent at lower speed (20 wpm),
          // assuming that the standard contest operating speed is between 24 and
          // 30 wpm.
          //
          case '[': // set buffered speed to "high"
            BufSpeed=40;
            break;
          case '$': // set buffered speed to "slow"
            BufSpeed=20;
            break;
          case ']': // cancel BUFSPD
            BufSpeed=0;
            break;
          case '{':
          case '}':
          case '^':
          case '_':
          case '\\':
          case '`':
          case '~':
          case 0x7f:
            break;
          case '|':  // thin space (a fulldotlen)
            // Note that the K1EL chip does half a dotlen but this is rather small
            sending=1;
            wait=actual + dotlen;
            keyer_state=SNDCHAR_DELAY;
            break;
          default:
            sending=ASCII_to_Morse(byte);
            if (sending != 1) {
              wait=actual;  // no lead-in wait by default
              if (!ptt_stat && PTT_ENABLED) {
                ptt_on();
                wait=actual+LeadIn*10;
              }
              keyer_state=SNDCHAR_PTT;
            }
            break;
        }
        break;
      }
      break;
    case STARTDOT:
      // wait = end of PTT lead-in time
      if (actual >= wait) {
        keyer_state=SENDDOT;
        memdash=0;
        dash_held=eff_kdash;
        wait=actual+dotlen;
        keydown();
        num_elements++;
      }
      break;
    case STARTDASH:
      // wait = end of PTT lead-in time
      if (actual >= wait) {
        keyer_state=SENDDASH;
        memdot=0;
        dot_held=eff_kdot;
        wait=actual+dashlen;
        keydown();
        num_elements++;
      }
      break;
    case STARTSTRAIGHT:
      // wait = end of PTT lead-in time
      memdot=memdash=dot_held=dash_held=0;
      if (actual >= wait) {
        if (straight) {
          keyer_state=SENDSTRAIGHT;
          keydown();
          straight_pressed=actual;
        } else {
          // key-up during PTT lead-in time: do not send key-down but
          // init hang time
          wait=actual+hang;
          keyer_state=CHECK;
        }
        break;
      }
    case SENDDOT:
      // wait = end of the dot
      if (actual >= wait) {
        last=actual;
        keyup();
        wait=wait+plen;
        keyer_state=DOTDELAY;
      }
      break;
    case SENDSTRAIGHT:
      //
      // do nothing until contact opens
      if (!straight) {
        last=actual;
        keyup();
        // determine length of elements and treat it as a dash if long enough
        if (actual  > straight_pressed + 2*plen) {
          collecting |= (1 << collpos++);
        } else {
          collpos++;
        }
        wait=actual+hang;
        keyer_state=CHECK;
      }
      break;
    case DOTDELAY:
      // wait = end of the pause following a dot
      if (actual >= wait) {
        if (!eff_kdot && !eff_kdash && IAMBIC_A) dash_held=0;
        if (memdash || eff_kdash || dash_held) {
          collecting |= (1 << collpos++);
          keyer_state=STARTDASH;
        } else if (eff_kdot) {
          collpos++;
          keyer_state=STARTDOT;
        } else {
          keyer_state=CHECK;
          wait=actual+hang-plen;
        }
      }
      break;
    case SENDDASH:
      // wait = end of the dash
      if (actual >= wait) {
        last=actual;
        keyup();
        wait=wait+plen;
        keyer_state=DASHDELAY;
      }
      break;
    case DASHDELAY:
      // wait = end of the pause following the dash
      if (actual > wait) {
        if (!eff_kdot && !eff_kdash && IAMBIC_A) dot_held=0;
        if (memdot || eff_kdot || dot_held) {
          collpos++;
          keyer_state=STARTDOT;
        } else if (eff_kdash) {
          collecting |= (1 << collpos++);
          keyer_state=STARTDASH;
        } else {
          keyer_state=CHECK;
          wait=actual+hang-plen;
        }
      }
      break;
    case SNDCHAR_PTT:
      // wait = end of PTT lead-in wait
      if (actual >= wait) {
        keyer_state=SNDCHAR_ELE;
        keydown();
        wait=actual + ((sending & 0x01) ? dashlen : dotlen);
        sending = (sending >> 1) & 0x7F;
      }
      break;
    case SNDCHAR_ELE:
      // wait = end of the current element (dot or dash)
      if (actual >= wait) {
        keyup();
        wait=actual+plen;
        if (sending == 1) {
          if (!prosign) wait += clen;
          prosign=0;
        }
        keyer_state=SNDCHAR_DELAY;
      }
      break;
    case SNDCHAR_DELAY:
      // wait = end  of pause (inter-element or inter-word)
      if (actual >= wait) {
        if (sending == 1) {
          keyer_state=CHECK;
          //
          // This is the ONLY PLACE where the "PTT Tail" time applies.
          // Note that the PTT Tail time *adds* to the three-dots
          // inter-word spacing, so it is not used normally.
          //
          // However we cannot use zero "tail time" here, since this
          // would lead to removing PTT *immediately* once entering
          // the CHECK state. Therefore we require a minimum
          // delay of 10 msec towards next removal of PTT, to give the
          // keyer state machine a change to send the next character from
          // the buffer (if there is one). This implies
          // that PTT is removed, at the earliest, (3*dotlen + 10 msec)
          // after the last key-up event. The "extra" 10 msec are
          // barely noticeable, given that 3*dotlen is larger than 100 msec
          // for CW speeds less than 36 wpm.
          //
          if (Tail > 0) {
            wait += 10*Tail;
          } else {
            wait += 10;
          }
        } else {
          keydown();
          keyer_state=SNDCHAR_ELE;
          wait=actual + ((sending & 0x01) ? dashlen : dotlen);
          sending = (sending >> 1) & 0x7F;
        }
      }
      break;
  }
}

///////////////////////////////////////
//
// This is the WinKey state machine
//
///////////////////////////////////////

void WinKey_state_machine() {
  uint8_t byte;
  static int OldWKstatus=-1;           // this is to detect status changes
  static int OldSpeedPot=-1;           // this is to detect Speed pot changes

  //
  // Now comes the WinKey state machine
  // First, handle commands that need no further bytes
  //
  switch (winkey_state) {
    case GETPOT:
      // send immediately
      ToHost(128+(SpeedPot & 0x1F));
      winkey_state=FREE;
      break;
    case BACKSPACE:
      backspace();
      winkey_state=FREE;
      break;
    case CLEAR:
      clearbuf();
      winkey_state=FREE;
      break;
    case WKSTAT:
      ToHost(WKstatus);
      winkey_state=FREE;
      break;
    case NULLCMD:
      winkey_state=FREE;
      break;
    case CANCELSPD: // Buffered command
      queue(1,CANCELSPD,0,0);
      winkey_state=FREE;
      break;
    case BUFNOP: // Buffered command
      queue(1,BUFNOP,0,0);
      winkey_state=FREE;
      break;
     case RDPROM:
      // dump EEPROM command. Report "standard" magic byte
      // at address 0.
      //
      // Nothing must interrupt us, therefore no state machine
      // at 1200 baud, each character has a mere transmission time
      // of more than 8 msec, so do a pause of 12 msec after each
      // character. Since a complete dump takes much time, check
      // for incoming MIDI messages frequently (every 6 msec or so).
      //

      for (inum=0; inum<256; inum++) {
        if (inum ==  0) {
          ToHost(0xA5);  // report 0xA5 no matter which magic byte we are using here
        } else {
          ToHost(EEPROM.read(inum));
        }
        if (highbaud) {
          delay(1);
          DrainMIDI();
        } else {
          delay(6);
          DrainMIDI();
          delay(6);
          DrainMIDI();
        }
      }
      winkey_state=FREE;
      break;
    case WRPROM:
      //
      // Load EEPROM command. Write our
      // "own" magic byte to addr 0.
      // Nothing must interrupt us, hence no state machine
      // no delay is required upon reading, but check for
      // incoming MIDI messages after each byte received.
      //
      for (inum=0; inum<256; inum++) {
        while (!ByteAvailable()) ;
        byte=FromHost();
        if (inum == 0) {
          EEPROM.update(0, MAGIC);
        } else {
          EEPROM.update(inum, byte);
        }
        DrainMIDI();
      }
      winkey_state=FREE;
      break;
    default:
      // This is a multi-byte command handled below
      break;
  }
  //
  // Check serial line, if in hostmode or ADMIN command enter "WinKey" state machine
  // Note: ADMIN commands should only be sent if hostmode is closed, with the exception
  //       of the CLOSE command. However we allow to use *all* admin commands at all times.
  //
  if (ByteAvailable()) {
    byte=FromHost();

    if (hostmode == 0 && winkey_state == FREE && byte != ADMIN) return;
    //
    // This switch statement builds the "WinKey" state machine
    //
    switch (winkey_state) {
      case FREE:
        //
        // In the idle state, the incoming byte is either a letter to
        // be transmitted or the start of the WinKey command.
        //
        if (byte >= 0x20) {
          queue(1,byte,0,0);
          winkey_state=FREE;
        } else {
          inum=0;
          winkey_state=(enum WKSTAT) byte;
        }
        break;
      case SWALLOW:
        // "swallow" a number of bytes given in "inum"
        if (inum-- == 0) winkey_state=FREE;
        break;
      case XECHO:
        ToHost(byte);
        winkey_state=FREE;
        break;
      case MESSAGE:
        if (byte >= 1 && byte <= 6) ReplayPointer=EEPROM.read(17+byte);
        winkey_state=FREE;
        break;
      case ADMIN:
        switch (byte) {
          case ADMIN_CALIBRATE: // swallow one byte, nothing returned
            inum=1;
            winkey_state=SWALLOW;
            break;
          case ADMIN_RESET: // nothing returned
            //
            // After doing AdminReset, the client must wait a small time
            // before sending "something", if the baud rate is changed here
            //
            if (highbaud) {
#ifdef MYSERIAL
              MYSERIAL.end();
              MYSERIAL.begin(1200);
#endif
            }
            read_from_eeprom();
            hostmode=0;
            HostSpeed=0;
            clearbuf();
            winkey_state=FREE;
            break;
          case ADMIN_OPEN: // return serial major number
            hostmode = 1;
            clearbuf();
            ToHost(WKVERSION);
            winkey_state=FREE;
            break;
          case ADMIN_CLOSE: // nothing returned
            //
            // Cannot reset baud rate, since some clients will
            // very quickly send "something" after the admin close
            //
            HostSpeed = 0;
            clearbuf();
            // restore "standalone" settings from EEPROM
            read_from_eeprom();
            winkey_state=FREE;
            break;
          case ADMIN_ECHO: // expect one byte that is echoed
            winkey_state=XECHO;
            break;
          case ADMIN_PAD_A2D:     // Admin Paddle A2D, historical command, return 0
            ToHost(0);
            winkey_state=FREE;
            break;
          case ADMIN_SPD_A2D:     // Admin Speed  A2D, historical command
            ToHost(WKVERSION);    // contrary to the manual, my WK2.1 chip returns "21"
            winkey_state=FREE;
            break;
          case ADMIN_DEBUG:        // K1EL debug only.
            //My WK 2.1 chip returns 24 bytes:
            for (int i=0; i<24; i++) {
              switch(i) {
                case  2:  ToHost(0x49); break;
                case  7:  ToHost(0x4d); break;  // sometimes 0x45 is returned here.
                case 17:  ToHost(0x4f); break;
                default:  ToHost(0x41); break;
              }
            }
            winkey_state=FREE;
            break;
          case ADMIN_GETMAJOR:      // Return K1EL version, major number
            ToHost(WKVERSION);
            winkey_state=FREE;
            break;
          case ADMIN_GETVAL:
            // never used so I refrain from making an own "state" for this.
            // we need no delays since the buffer should be able to hold 15
            // bytes.
            // NOTE: my WK2.1 chip does not return anything, while the WK3.1 manual
            //       says "return zero"
            ToHost(ModeRegister);
            ToHost(HostSpeed);
            ToHost(Sidetone);
            ToHost(Weight);
            ToHost(LeadIn);
            ToHost(Tail);
            ToHost(MinWPM);
            ToHost(WPMrange);
            ToHost(Extension);
            ToHost(Compensation);
            ToHost(Farnsworth);
            ToHost(PaddlePoint);
            ToHost(Ratio);
            ToHost(PinConfig);
            ToHost(0);
            winkey_state=FREE;
            break;
          case ADMIN_SETWK1:
            winkey_state=FREE;  // not implemented
            break;
          case ADMIN_SETWK2:
            winkey_state=FREE;  // not implemented
            break;
          case ADMIN_DUMPEEPROM:
            winkey_state=RDPROM;
            break;
          case ADMIN_LOADEEPROM:
            winkey_state=WRPROM;
            break;
          case ADMIN_SENDMSG:
            winkey_state=MESSAGE;
            break;
          case ADMIN_LOADX1:
            inum=1;
            winkey_state=SWALLOW;  // not (yet) implemented, letter spacing etc.
            break;
          case ADMIN_FWUPDT: // Admin Reserved (return a zero!)
            ToHost(0);
            winkey_state=FREE;
            break;
          case ADMIN_LOWBAUD: // Admin Set Low Baud
            if (highbaud) {
#ifdef MYSERIAL
              MYSERIAL.end();
              MYSERIAL.begin(1200);
#endif
              highbaud=0;
            }
            break;
          case ADMIN_HIGHBAUD: // Admin Set High Baud
            if (!highbaud) {
#ifdef MYSERIAL
              MYSERIAL.end();
              MYSERIAL.begin(9600);
#endif
              highbaud=1;
            }
            winkey_state=FREE;
            break;
          case ADMIN_RTTY:  // expect 2 bytes with RTTY parameters, WK3 only.
            inum=2;
            winkey_state=SWALLOW;
            break;
          case ADMIN_SETWK3: // Set WK3 mode (WK3 only)
             winkey_state=FREE;
             break;
          case ADMIN_VCC: // Read VCC (WK3 only)
             ToHost(52);  // Fake 5.0 Volts
             winkey_state=FREE;
             break;
          case ADMIN_LOADX2:  // set extension register 2, WK3 only
             inum=1;
             winkey_state=SWALLOW;
             break;
          case ADMIN_GETMINOR:  // Get minor version number, WK3 only
             ToHost(0);  // We fake a 23.00 Version
             winkey_state=FREE;
             break;
          case ADMIN_GETTYPE: // Get IC type, WK3 only
             ToHost(0);  // Fake a DIP
             winkey_state=FREE;
             break;
          case ADMIN_VOLUME: // Sidetone volume (WK3 only), ignored
            inum=1;
            winkey_state=SWALLOW;
            break;
          default: // Should not occur. Do not return anything.
             winkey_state=FREE;
             break;
        }
        break;
      case SIDETONE:
        Sidetone=byte;
        winkey_state=FREE;
        break;
      case WKSPEED:
        HostSpeed=byte;
        if (HostSpeed > 99) HostSpeed=99;
#ifdef CWKEYERSHIELD
        if (HostSpeed != 0) {
          cwshield.cwspeed(HostSpeed);
        }
#endif
        winkey_state=FREE;
        break;
      case WEIGHT:
        if (byte < 10) byte=10;
        if (byte > 90) byte=90;
        Weight=byte;
        winkey_state=FREE;
        break;
      case PTT:
        switch (inum++) {
          case 0:
            LeadIn=byte;
            break;
          case 1:
            Tail=byte;
            winkey_state=FREE;
        }
        break;
      case POTSET:
        switch (inum++) {
          case 0:
            MinWPM=byte;
            break;
          case 1:
            WPMrange=byte;
            if (WPMrange > 31) WPMrange=31;
            break;
          case 2:
            // byte part of the protocol but ignored
            winkey_state=FREE;
            break;
        }
        break;
      case PAUSE:
        pausing=byte;
        winkey_state=FREE;
        break;
      case PINCONFIG:
        PinConfig=byte;
        winkey_state=FREE;
        break;
      case TUNE:
        // use fixed lead-in/tail times with "busy waiting".
        if (byte) {
          clearbuf();
          tuning=1;
          if (PTT_ENABLED) {
            ptt_on();
            delay(100);
          }
          keydown();
        } else {
          keyup();
          if (PTT_ENABLED) {
            delay(100);
          }
          ptt_off();
          tuning=0;
        }
        winkey_state=FREE;
        break;
      case HSCW:
        // ignored
        winkey_state=FREE;
        break;
      case FARNS:
        if (byte < 10) byte=10;
        if (byte > 99) byte=99;
        Farnsworth=byte;
        winkey_state=FREE;
        break;
      case WK2MODE:
        ModeRegister=byte;
        winkey_state=FREE;
        break;
      case LOADDEF:
        //
        // get 15 bytes to load default values.
        // not much error checking done here
        //
        switch (inum++) {
          case  0:
            ModeRegister=byte;
            break;
          case  1:
            HostSpeed=byte;
#ifdef CWKEYERSHIELD
        if (HostSpeed != 0) {
          cwshield.cwspeed(HostSpeed);
        }
#endif
            break;
          case  2:
            Sidetone=byte;
            break;
          case  3:
            Weight=byte;
            break;
          case  4:
            LeadIn=byte;
            break;
          case  5:
            Tail=byte;
            break;
          case  6:
            MinWPM=byte;
            break;
          case  7:
            WPMrange=byte;
            if (WPMrange > 31) WPMrange=31;
            break;
          case  8:
            Extension=byte;
            break;
          case  9:
            Compensation=byte;
            break;
          case 10:
            Farnsworth=byte;
            break;
          case 11:
            PaddlePoint=byte;
            break;
          case 12:
            Ratio=byte;
            break;
          case 13:
            PinConfig=byte;
            break;
          case 14:
            winkey_state=FREE;
            break;
        }
        break;
      case EXTENSION:
        Extension=byte;
        winkey_state=FREE;
        break;
      case KEYCOMP:
        Compensation=byte;
        winkey_state=FREE;
        break;
      case PADSW:
        // paddle switch-point ignored
        winkey_state=FREE;
        break;
      case SOFTPAD:
        // software paddle ignored
        winkey_state=FREE;
        break;
      case POINTER_1:
      case POINTER_2:
         // set buffer position
         if (byte > 0) byte--;
         setbufpos(byte);
         winkey_state=FREE;
         break;
      case POINTER_3:
         // queue some NULLs
         bufzero(byte);
         winkey_state=FREE;
         break;
      case POINTER:
        switch (byte) {
          case 0:  clearbuf(); winkey_state=FREE; break;
          case 1:  winkey_state=POINTER_1; break;
          case 2:  winkey_state=POINTER_2; break;
          case 3:  winkey_state=POINTER_3; break;
          default: winkey_state=FREE;      break;
        }
        break;
      case RATIO:
        if (byte < 33) byte=33;
        if (byte > 66) byte=66;
        Ratio=byte;
        winkey_state=FREE;
        break;
      //
      // Here comes a bunch of "buffered" commands. We do nothing HERE,
      // since the commands are simply queued
      //
      case SETPTT:
        queue(2,SETPTT,byte,0);
        winkey_state=FREE;
        break;
      case KEYBUF:
        queue(2,KEYBUF,byte,0);
        winkey_state=FREE;
        break;
      case WAIT:
        queue(2,WAIT,byte,0);
        winkey_state=FREE;
        break;
      case PROSIGN:
        switch (inum++) {
          case 0:
            ps1=byte;
            break;
          case 1:
            // character_buffer prosign command
           queue(3,PROSIGN,ps1,byte);
           winkey_state=FREE;
        }
        break;
      case BUFSPD:
        queue(2,BUFSPD,byte,0);
        winkey_state=FREE;
        break;
      case HSCWSPD:
        queue(2,HSCWSPD,byte,0);
        break;
      default:
        winkey_state=FREE;
        break;
    }
  }

  //
  // loop exit code: update WK status and send if changed
  //
  if (breakin) {
    WKstatus |= 0x02;
    breakin=0;
  } else {
    WKstatus &= 0xFD;
    if (keyer_state == CHECK) {
      WKstatus &= 0xFB;
    } else {
      WKstatus |= 0x04;
    }
  }

  if ((WKstatus != OldWKstatus) && hostmode) {
    ToHost(WKstatus);
    OldWKstatus=WKstatus;
  }
  if ((SpeedPot != OldSpeedPot) && hostmode) {
    ToHost(128+(SpeedPot & 0x1F));
    OldSpeedPot=SpeedPot;
  }

}

#ifdef POWERSAVE
//
// currently, only for AVR architecture
//
#include <avr/sleep.h>
#include <avr/wdt.h>

ISR (WDT_vect)
{
  wdt_disable();
  sleep_disable();
  interrupts();
}

void goto_sleep() {
    //
    // Most uCs only have a limited number of I/O lines that can
    // be used for interrupts, so we do it the "pedestrian way"
    // here, and use the hardware watchdog timer to wake up every
    // second for a short while, just to check the input lines.
    // Note that some devices need to re-connect to the USB bus
    // to be able to enter host mode. This is done with USBDevice.attach()
    // on Leonardos but this does not exist for Teensy2.
    //
    set_sleep_mode(SLEEP_MODE_PWR_SAVE);
#if defined(USBCON) && !defined(TEENSYDUINO)
    USBDevice.detach();  // This works on Leonardo but not on Teensy2
#endif
#ifdef MYSERIAL
    MYSERIAL.end();  // Serial.end() includes USB shutdown on Teensy2
#endif
    for (;;) {
      //
      // There is a library function wdt_enable() but this seems
      // to do a reset rather than an interrupt after the time-out
      // so here is the "pedestrian" way to activate the hardware
      // watch-dog timer
      //
      cli();
      MCUSR = 0;
      WDTCSR |= B00011000;
      WDTCSR = B01000110;
      sei();
      sleep_enable();
      sleep_cpu();
      //
      // After wake-up, check input lines. If they are not
      // active just continue to sleep.
      //
#ifdef StraightKey
      if (digitalRead(StraightKey) == 0) break;
#endif
#ifdef PaddleLeft
      if (digitalRead(PaddleLeft) == 0) break;
      if (digitalRead(PaddleRight) == 0) break;
#endif
    }
#if defined(USBCON) && !defined(TEENSYDUINO)
    USBDevice.attach();  // This works on Leonardo but not on Teensy2
#endif
#ifdef MYSERIAL
    MYSERIAL.begin(1200); // This should re-initialize USB on Teensy2
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// This is executed again and again at a high rate.
// Most of the time, there will be nothing to do.
//
//////////////////////////////////////////////////////////////////////////////

extern int usb_wait_rx_flag, usb_wait_tx_flag;

void loop() {
  int i;
  static uint8_t LoopCounter=0;
  static unsigned long DotDebounce=0;      // used for "debouncing" dot paddle contact
  static unsigned long DashDebounce=0;     // used for "debouncing" dash paddle contact
  static unsigned long StraightDebounce=0; // used for "debouncing" straight key contact
  static uint8_t old_sidetone_enabled = 1;
  static uint8_t old_sidetone = 1;

  //////////////////////////////////////////////////////////////////////////////
  //
  //
  // This sets the "now" time
  //
  //////////////////////////////////////////////////////////////////////////////


  actual=millis();

#ifdef POWERSAVE

  //
  // If we are in host mode, we are USB powered so no need to go to sleep
  //
  if (hostmode) watchdog=actual;
  //
  // look if more than 300 seconds passed since the last update of the watchdog
  // timer and we are not in host mode. In this case, go to sleep. Take care of overflows.
  //

  unsigned long ul = watchdog + 300000UL;

  if (((ul > watchdog) && ((actual >= watchdog) && (actual <= ul))) ||
      ((ul < watchdog) && ((actual >= watchdog) || (actual <= ul)))) {
    // ul > watchdog  and actual between watchdog and ul: do not go to sleep (no-overflow case)
    // ul < watchdog and actual has not yet reached the region between ul and watchdog:
    // do not go to sleep (overflow case)
    // host connection open: do not go to sleep.
  } else {
    goto_sleep();
    actual=millis();
    watchdog=actual;
  }
#endif
  //
  //
  //////////////////////////////////////////////////////////////////////////////
  //
  // Read digital and analog input lines with debouncing.
  //
  // For the left/right contacts assign them to dit/dah or dah/dit
  // depending on the ModeRegister. If state changes, set dot/dash memory.
  //
  // Note that we are monitoring the lines at high speed thus need no
  // interrupts
  ///////////////////////////////////////////////////////////////////////////////

#if defined(PaddleRight) && defined(PaddleLeft)
  if (actual >= DotDebounce) {
    i=!digitalRead(PADDLE_SWAP ? PaddleRight : PaddleLeft);
    if (i != kdot) {
#ifdef POWERSAVE
      watchdog=actual;
#endif
      DotDebounce=actual+10;
      kdot=i;
      if (kdot) {
        memdot=1;
        lastpressed=0;
      }
    }
  }

  if (actual >= DashDebounce) {
    i=!digitalRead(PADDLE_SWAP ? PaddleLeft : PaddleRight);
    if (i != kdash) {
#ifdef POWERSAVE
      watchdog=actual;
#endif
      DashDebounce=actual+10;
      kdash=i;
      if (kdash) {
        memdash=1;
        lastpressed=1;
      }
    }
  }
#endif

#ifdef StraightKey
  if (actual >= StraightDebounce) {
    i=!digitalRead(StraightKey);
    if (i != straight) {
#ifdef POWERSAVE
      watchdog=actual;
#endif
      StraightDebounce=actual+15;
      straight=i;
    }
  }
#endif

#ifdef BUTTONPIN
//
// We have an analog input for reading out the push-buttons
// We also need a definition of intervals which provide
// a mapping from analog values to pushbutton states.
//
// Value from 0                    ...  BUTTON_BORDER_1   ==> Button1 pressed
// Value from BUTTON_BORDER_1 + 1  ...  BUTTON_BORDER_2   ==> Button2 pressed
// Value from BUTTON_BORDER_2 + 1  ...  BUTTON_BORDER_3   ==> Button3 pressed
// Value from BUTTON_BORDER_3 + 1  ...  BUTTON_BORDER_4   ==> Button4 pressed
// Value from BUTTON_BORDER_4 + 1  ...  BUTTON_BORDER_5   ==> Button5 pressed
// Value from BUTTON_BORDER_5 + 1  ...  BUTTON_BORDER_6   ==> Button6 pressed
// Value from BUTTON_BORDER_6 + 1  ...  4092              ==> no button pressed
//
// I have calculated defaults appropriate for
// a network with one 10k resistor and three 1k ones (using 10k/1k seems to be
// the quasi-standard), and an exponential averaging that produces
// values between 0 and 4092.
//
#define BUTTON_BORDER_1   186
#define BUTTON_BORDER_2   527
#define BUTTON_BORDER_3   813
#define BUTTON_BORDER_4  1057
#define BUTTON_BORDER_5  1268
#define BUTTON_BORDER_6  1450
#define BUTTON_HIGH      2000
//
// Due to the exponential averaging of the analog values, the read-out will
// slowly sweep from the max value downward upon key press.
// When the value crosses BUTTON_HIGH,
// we make 12 further averages to let it settle down.
// Upon releasing the button it will sweep up again.
//
// Thus we have several different states:
// BUTTON_PRE_STATE   is the idle state, the readout > BUTTON_HIGH
// BUTTON_WAIT_STATE  is entered from BUTTON_PRE_STATE if the readout is below BUTTON_HIGH
//                    for the first time. Then the averaging continues for 12 cycles, the
//                    value is read and an action is taken
// BUTTON_POST_STATE  is entered after a valid read-out has been made. From this,
//                    BUTTON_PRE_STATE is reached if the read-out is above BUTTON_HIGH.
//
//
// Since analog reads are expensive, we read every 10 msec if we are in pre  or post state, every 5 msec
// in the wait state

#define BUTTON_PRE_STATE 255
#define BUTTON_WAIT_STATE 12
#define BUTTON_POST_STATE 0
static uint8_t button_state=BUTTON_PRE_STATE;
static uint32_t    button_debounce=0;
static uint16_t    button_val=4092;

if (actual >= button_debounce) {
  i=analogRead(BUTTONPIN);
  // exponential averaging.
  // button_val is between zero and 4*1023
  // (1023 is max value returned by analogRead)
  button_val += (i - button_val/4);
  switch (button_state) {
    case BUTTON_PRE_STATE:
      if (button_val > BUTTON_HIGH) {
        button_debounce=actual+10;
      } else {
        button_debounce=actual+5;
        button_state=BUTTON_WAIT_STATE;
#ifdef POWERSAVE
      // pressing a  button resets the "deep sleep" timer
      watchdog=actual;
#endif
      }
      break;
    case BUTTON_POST_STATE:
      // remain in "post" state until the readout is above BUTTON_HIGH
      button_debounce=actual+10;
      if (button_val > BUTTON_HIGH) {
        button_state=BUTTON_PRE_STATE;
      }
      break;
    default:
      // our state cycles from WAIT downto POST
      if (--button_state == BUTTON_POST_STATE) {
        button_debounce=actual+10;
        // we can make a read-out and trigger an  action. Note that pushing
        // a button while sending a message aborts that message and starts
        // the new one.
        if (button_val < BUTTON_BORDER_1) {
            ReplayPointer = EEPROM.read(18);
        } else if (button_val < BUTTON_BORDER_2) {
            ReplayPointer = EEPROM.read(19);
        } else if (button_val < BUTTON_BORDER_3) {
            ReplayPointer = EEPROM.read(20);
        } else if (button_val < BUTTON_BORDER_4) {
            ReplayPointer= EEPROM.read(21);
        } else if (button_val < BUTTON_BORDER_5) {
            ReplayPointer= EEPROM.read(22);
        } else if (button_val < BUTTON_BORDER_6) {
            ReplayPointer= EEPROM.read(23);
        }  else {
            // Hardware spike, do nothing
        }
      }
      break;
  }
}
#endif

#ifdef POTPIN
  //
  // only query the potentiometer while the keyer is idle.
  // This prevents speed changes within a letter, and also
  // makes the speed pot immune against RFI.
  // However, sometimes one wants to adjust the speed pot while
  // producing a long sequence of dots (or dahs) and then
  // we have to adjust the speed. This is detected by num_elements.
  //
  // The speed pot is debounced with a relatively long time constant
  //
  static int SpeedPinValue=2000;           // default value: mid position
  static unsigned long SpeedDebounce=0;    // used for "debouncing" speed pot

  if ((keyer_state == CHECK || num_elements > 5) && (actual >= SpeedDebounce)) {
    SpeedDebounce=actual + 20;
    i = analogRead(POTPIN);
    SpeedPinValue += (i - SpeedPinValue/4);  // Range 0 ... 4092
  }
#endif

  /////////////////////////////////////////////////////////////////////////////////
  //
  // The bug and ultimatic modes are not implemented in the keyer.
  // instead, we apply some logic to the "contact closures"
  //
  // So kdash and kdot reflect the "physical" state of the paddle
  // contacts while eff_kdash and eff_kdot are the states as seen by
  // the keyer.
  //
  // BUG MODE:
  //     logical-OR the dash to the straight keyer contact,
  //     and let the effective dash contact always "open"
  //
  // ULTIMATIC MODE:
  //     never report "both contacts closed" to the keyer.
  //     in this case, only the last-pressed contact wins.
  //
  /////////////////////////////////////////////////////////////////////////////////

  eff_kdash=kdash;
  eff_kdot=kdot;

  if (BUGMODE) {
    straight |= kdash;
    memdash=0;
    eff_kdash=0;
  }

  if (ULTIMATIC && kdash && kdot) {
    if (lastpressed) {
      // last contact closed was dash, so do not report a closed dot contact upstream
      eff_kdot=0;
    } else {
      // last contact closed was dot, so do not report a closed dash contact upstream
      eff_kdash=0;
    }
  }

  // WK2.3 change: end "TUNE" mode if a paddle is pressed
  if (tuning && (kdot || kdash)) {
      keyup();
      if (PTT_ENABLED) {
        delay(50);
      }
      ptt_off();
      tuning=0;
  }
  /////////////////////////////////////////////////////////////////////////////////
  //
  // Distribute the remaining work across different executions of loop()
  //
  /////////////////////////////////////////////////////////////////////////////////
  switch (LoopCounter++) {
#ifdef POTPIN
    static int16_t OldSpeedPinValue = 2000;
#endif
    case 0:
       //
       // Adjust CW speed
       //
#ifdef POTPIN
      // ATTN: no 16-bit overflow, result between 0 and 31 (if WPMrange is at most 31)
      // SpeedPinValue           is in the range 0 - 4092
      // x = SpeedPinValue >> 6  is in the range 0 - 63
      // x*WPMrange + 32         is in the range 0 - 1985
      // final result            is in the range 0 - 31
      //
      // Do not change Speed if the SpeedPinValue changes slightly
      //
      if (SpeedPinValue > OldSpeedPinValue + 100 || SpeedPinValue < OldSpeedPinValue - 100) {
        OldSpeedPinValue = SpeedPinValue;
        SpeedPot=((SpeedPinValue >> 6)*WPMrange+32) >> 6;
        Speed=MinWPM+SpeedPot;
      }
#else
       //
       // If there is no speed pot, or speed pot is handled externally,
       // compute a "virtual" SpeedPot value since reporting this back
       // is part of the WK protocol
       //
       SpeedPot = Speed - MinWPM; // maintain "virtual" value of speed pot
       if (Speed < MinWPM)  SpeedPot=0;
       if (SpeedPot > WPMrange) SpeedPot=WPMrange;
#endif
       break;
    case 2:
       //
       // Check if Side tone settings changed
       //
       if (Sidetone != old_sidetone) {
         myfreq=4000/(Sidetone & 0x0F);
         old_sidetone = Sidetone;
#ifdef TEENSY4AUDIO
         sidetone.set_frequency(myfreq);
#endif
#ifdef CWKEYERSHIELD
         if (myfreq <= 1270) {
           cwshield.sidetonefrequency((myfreq+5)/10);
         } else {
           cwshield.sidetonefrequency(127);
         }
#endif
       }
       if (old_sidetone_enabled != SIDETONE_ENABLED) {
          old_sidetone_enabled = SIDETONE_ENABLED;
#ifdef CWKEYERSHIELD
          cwshield.sidetoneenable(old_sidetone_enabled);
#endif
       }
      break;
    case 4:
      //
      // One heart-beat of WinKey state machine
      //
      WinKey_state_machine();
      break;
    case 6:
      //
      // This is for checking incoming MIDI messages
      // (TeensyUSBAudioMidi also monitors potentiometers)
      //
      DrainMIDI();
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      //
      // execute the keyer state machine every second loop
      //
      if (!tuning) keyer_state_machine();
      break;
    default:
      //
      // here we arrive when one cycle is complete
      //
      LoopCounter=0;
  }
}
