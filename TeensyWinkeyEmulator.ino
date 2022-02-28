////////////////////////////////////////////////////////////////////////////////////////
//
// Teensy keyer with audio (WinKey emulator)
//
// (C) Christoph van Wüllen, DL1YCF, 2020/2021
//
// Designed for Arduino or Teensy
//
// It can make use of the "MIDI" features of the Teensy 2.0 and 4.0
// It can make use of the "AUDIO" features of Teensy 4.0
// There are some basic #define's in the file config.h
// which control which "backends" are used for signalling key-up/down and
// PTT on/off events, and the way to produce a side tone. This choice
// also affects which Serial module can be used.
//
//
// Incompatible options from config.h (to be cleared up below):
//
// -  HWSERIAL overrides SWSERIAL
// -  CWKEYERSHIELD overrides USBMIDI and MOCOLUFA.
// -  USBMIDI overrides MOCOLUFA
// -  MOCOLUFA disables HWSERIAL
// -  SWSERIAL is disables unless RXD and TXD is defined
//
//
// File config.h also defines the hardware pins (digital input, digital output,
// analog input) to be used. Note that when using CWKEYERSHIELD, a speed pot
// should be handled there (below we #undef POTPIN if using CWKEYERSHIELD).
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "config.sgtl5000.h"


////////////////////////////////////////////////////////////////////////////////////////
//
//                                MAIN FEATURES
//
////////////////////////////////////////////////////////////////////////////////////////
//
//   MIDI (only with CWKEYERSHIELD, USBMIDI, or MOCOLUFA)
//   =========================================================
//
// - key-up/down and PTT-on/off events are sent via MIDI to the computer.
//   For USBMIDI and MOCOLUFA, this is coded here in the sketch, while for
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
//   High-quality side tone (only with CWKEYERSHIELD)
//   =====================================================
//
//   A high-quality side tone is generated, using either I2S or MQS output
//   (an i2s audio shield is required in the latter case). The Teensy
//   shows up at the computer as a sound card which you can choose from
//   your SDR console application. In this case, the headphone connected
//   to the Teensy (both for MQS and I2S) will have RX audio as well as
//   a low-latency side tone.
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
// Eliminate incompatible options
//
////////////////////////////////////////////////////////////////////////////////////////

#ifdef CWKEYERSHIELD
#undef USBMIDI
#undef MOCOLUFA
#undef POTPIN
#endif

#ifdef USBMIDI
#undef MOCOLUFA
#endif

#ifdef MOCOLUFA
#undef HWSERIAL
#endif

#ifdef HWSERIAL
#undef SWSERIAL
#endif

#if !defined(RXD_PIN) || !defined(TXD_PIN)
#undef SWSERIAL
#endif

#if defined(USBMIDI) || defined(MOCOLUFA)
//
// The CWKeyerShield (Teensy4) library has built-in defaults,
// for USBMIDI (e.g. Teensy2) or MOCOLUFA (Arduino) we define
// defaults here that can be overriden by defining these values
// in the config.h file.
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
#endif // USBMIDI or MOCOLUFA

#include <EEPROM.h>

#ifdef CWKEYERSHIELD
#include "CWKeyerShield.h"
#undef POTPIN
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
  ADMIN_CMD8       =  8,
  ADMIN_GETCAL     =  9,
  ADMIN_SETWK1     = 10,
  ADMIN_SETWK2     = 11,
  ADMIN_DUMPEEPROM = 12,
  ADMIN_LOADEEPROM = 13,
  ADMIN_SENDMSG    = 14,
  ADMIN_LOADXMODE  = 15,
  ADMIN_CMD16      = 16,
  ADMIN_HIGHBAUD   = 17,
  ADMIN_LOWBAUD    = 18,
  ADMIN_CMD19      = 19,
  ADMIN_CMD20      = 20
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
// NOTE:
// When reporting EEPROM contents, report un-used parameters as well, to be compatible with
// WinKey programs
//

const static uint8_t MAGIC=0xA5;        // EEPROM magic byte
static uint8_t ModeRegister=0x10;       // Iambic-A by default
static uint8_t Speed=21;                // overridden by the speed pot in standalong mode
static uint8_t Sidetone=5;              // 800 Hz
static uint8_t Weight=50;               // used to modify dit/dah length
static uint8_t LeadIn=0;                // PTT Lead-in time (in units of 10 ms)
static uint8_t Tail=0;                  // PTT tail (in 10 ms), zero means "use hang bits"
static uint8_t MinWPM=8;                // CW speed when speed pot maximally CCW
static uint8_t WPMrange=20;             // CW speed range for SpeedPot
static uint8_t Extension=0;             // ignored in this code (1st extension)
static uint8_t Compensation=0;          // Used to modify dit/dah lengths
static uint8_t Farnsworth=10;           // Farnsworth speed (10 means: no Farnsworth)
static uint8_t PaddlePoint=50;          // ignored in this code (Paddle Switchpoint setting)
static uint8_t Ratio=50;                // dah/dit ratio = (3*ratio)/50
static uint8_t PinConfig=0x0E;          // PTT disabled
const static uint8_t k12ext=0;          // Letter space, zero cuts, etc. (unused)
const static uint8_t CmdWpm=15;         // ignored in this code (Command WPM setting)
const static uint8_t FreePtr=0x18;      // free space in message memory. Initially 24.
const static uint8_t MsgPtr=0x18;       // Pointer to empty message


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

static uint8_t ps1;                     // first byte of a pro-sign
static uint8_t pausing=0;               // "pause" state
static uint8_t breakin=1;               // breakin state
static uint8_t straight=0;              // state of the straight key (1 = pressed, 0 = released)
static uint8_t tuning=0;                // "Tune" mode active, deactivate paddle
static uint8_t hostmode  = 0;           // host mode
static uint8_t SpeedPot =  0;           // Speed value from the Potentiometer
static uint16_t myfreq=800;              // current side tone frequency
static uint8_t prosign=0;               // set if we are in the middle of a prosign
static uint8_t num_elements=0;          // number of elements sent in a sequence

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ring buffer for characters queued for sending
//
#define BUFLEN 128     // number of bytes in buffer (much larger than in K1EL chip)
#define BUFMARGIN 85   // water mark for reporting "buffer almost full"

static unsigned char character_buffer[BUFLEN];  // circular buffer
static uint8_t bufrx=0;                             // output (read) pointer
static uint8_t buftx=0;                             // input (write) pointer
static uint8_t bufcnt=0;                            // number of characters in buffer
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
static unsigned long wait;      // when "actual" reaches this value terminate current keyer state
static unsigned long last;      // time of last enddot/enddash
static unsigned long actual;    // time-stamp for this execution of loop()

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

////////////////////////////////////////////////////////////////////////////////////////
//
// Some variables are needed, set them to -1 if not defined
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
  Tail=0;                                    //  This is necessary to enable the "hang time"
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


//////////////////////////////////////////////////////////////////////////////
//
// setup:
// Initialize serial port and hardware lines
// init eeprom or load settings from eeprom
// init Audio+MIDI
//
//////////////////////////////////////////////////////////////////////////////

#ifdef SWSERIAL
#include <SoftwareSerial.h>
SoftwareSerial swserial = SoftwareSerial(RXD_PIN, TXD_PIN);
#endif

void setup() {

#ifdef HWSERIAL
  Serial.begin(1200);  // baud rate has no meaning on 32U4 and Teensy systems
#endif

#ifdef SWSERIAL
  pinMode(RXD_PIN, INPUT);
  pinMode(TXD_PIN, OUTPUT);
  swserial.begin(1200);
#endif
#ifdef MOCOLUFA
  Serial.begin(31250);
#endif

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

  init_eeprom();
  
#ifdef CWKEYERSHIELD

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
#ifdef MY_DEFAULT_VOLUME
  cwshield.sidetonevolume(MY_DEFAULT_VOLUME);
#endif
#endif
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
// the EEPROM, if upon startup the EEPROM is found "empty".
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
// MIDI might require to drain or process incoming MIDI messages, so this
// one is periodically called.
//
//////////////////////////////////////////////////////////////////////////////

void DrainMIDI() {
#ifdef CWKEYERSHIELD
    cwshield.loop();
#endif
#ifdef USBMIDI
    if (usbMIDI.read()) {
      // nothing to be done
    }
#endif
#ifdef MOCOLUFA
    if (Serial.available()) {
      Serial.read();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Read one byte from host
//
//////////////////////////////////////////////////////////////////////////////

int FromHost() {
  int c=0;
#ifdef HWSERIAL
  c=Serial.read();
#endif

#ifdef SWSERIAL
  c=swserial.read();
#endif
  return c;
}

//////////////////////////////////////////////////////////////////////////////
//
// Write one byte to the host
//
//////////////////////////////////////////////////////////////////////////////

void ToHost(int c) {
#ifdef HWSERIAL
  Serial.write(c);
#endif

#ifdef SWSERIAL
  swserial.write(c);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Check if there is a byte from host
//
//////////////////////////////////////////////////////////////////////////////

int ByteAvailable() {
#ifdef HWSERIAL
  return Serial.available();
#endif

#ifdef swserial
  return swserial.available();
#endif
  return 0;
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
#ifdef TONEPIN                                          // square wave (if enabled)
  if (SIDETONE_ENABLED) {
    tone(TONEPIN, myfreq);
  }
#endif

#ifdef CW1                                              // active-high CW output
  digitalWrite(CW1,HIGH);
#endif

#ifdef CW2                                              //active-low CW line
  digitalWrite(CW2,LOW);
#endif

#if defined(USBMIDI)  && defined(MY_KEYDOWN_NOTE) && defined(MY_MIDI_CHANNEL)
  usbMIDI.sendNoteOn(MY_KEYDOWN_NOTE, 127, MY_MIDI_CHANNEL);
  usbMIDI.send_now();
#endif

#if defined(MOCOLUFA) && defined(MY_KEYDOWN_NOTE) && defined(MY_MIDI_CHANNEL)
  Serial.write(0x90 | (MY_MIDI_CHANNEL-1) & 0x0F));
  Serial.write(MY_KEYDOWN_NOTE);
  Serial.write(127);
#endif

#ifdef CWKEYERSHIELD                               // MIDI and sidetone
 cwshield.key(1);
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
#ifdef TONEPIN                                          // square wave line
  noTone(TONEPIN);
#endif

#ifdef CW1                                              // active-high CW output
  digitalWrite(CW1,LOW);
#endif

#ifdef CW2
  digitalWrite(CW2,HIGH);                               // active-low CW output
#endif

#if defined(USBMIDI) && defined(MY_KEYDOWN_NOTE) && defined(MY_MIDI_CHANNEL)
  usbMIDI.sendNoteOn(MY_KEYDOWN_NOTE, 0, MY_MIDI_CHANNEL);
  usbMIDI.send_now();
#endif

#if defined(MOCOLUFA) && defined(MY_KEYDOWN_NOTE) && defined(MY_MIDI_CHANNEL)
  Serial.write(0x90 | (MY_MIDI_CHANNEL-1) & 0x0F));
  Serial.write(MY_KEYDOWN_NOTE);
  Serial.write(0);
#endif

#ifdef CWKEYERSHIELD                               // MIDI and side tone
 cwshield.key(0);
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

#if defined(USBMIDI) && defined(MY_PTT_NOTE) && defined(MY_MIDI_CHANNEL)
  usbMIDI.sendNoteOn(MY_PTT_NOTE, 127, MY_MIDI_CHANNEL);
#endif

#if defined(MOCOLUFA) && defined(MY_PTT_NOTE) && defined(MY_MIDI_CHANNEL)
  Serial.write(0x90 | (MY_MIDI_CHANNEL-1) & 0x0F));
  Serial.write(MY_PTT_NOTE);
  Serial.write(127);
#endif

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

#if defined(USBMIDI) && defined(MY_PTT_NOTE) && defined(MY_MIDI_CHANNEL)
  usbMIDI.sendNoteOn(MY_PTT_NOTE, 0, MY_MIDI_CHANNEL);
#endif

#if defined(MOCOLUFA) && defined(MY_PTT_NOTE) && defined(MY_MIDI_CHANNEL)
  Serial.write(0x90 | (MY_MIDI_CHANNEL-1) & 0x0F));
  Serial.write(MY_CWPTT_NOTE);
  Serial.write(0);
#endif

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
//////////////////////////////////////////////////////////////////////////////

unsigned char morse[58] = {
   0x01,     //33 !    not in ITU-R M.1677-1
   0x52,     //34 "    .-..-.
   0x01,     //35 #    not in ITU-R M.1677-1
   0x01,     //36 $    not in ITU-R M.1677-1
   0x01,     //37 %    not in ITU-R M.1677-1
   0x01,     //38 &    not in ITU-R M.1677-1
   0x5e,     //39 '    .----.
   0x36,     //40 (    -.--.
   0x6d,     //41 )    -.--.-
   0x01,     //42 *    not in ITU-R M.1677-1
   0x2a,     //43 +    .-.-.
   0x73,     //44 ,    --..--
   0x61,     //45 -    -....-
   0x6A,     //46 .    .-.-.-
   0x29,     //47 /    -..-.
   0x3f,     //48 0    -----
   0x3e,     //49 1    .----
   0x3c,     //50 2    ..---
   0x38,     //51 3    ...--
   0x30,     //52 4    ....-
   0x20,     //53 5    .....
   0x21,     //54 6    -....
   0x23,     //55 7    --...
   0x27,     //56 8    ---..
   0x2f,     //57 9    ----.
   0x78,     //58 :    ---...
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
   0x0b,     //71 G    --.
   0x10,     //72 H    ....
   0x04,     //73 I    ..
   0x1e,     //74 J    .---
   0x0d,     //75 K    -.-
   0x12,     //76 L    .-..
   0x07,     //77 M    --
   0x05,     //78 N    -.
   0x0f,     //79 O    ---
   0x16,     //80 P    .--.
   0x1b,     //81 Q    --.-
   0x0a,     //82 R    .-.
   0x08,     //83 S    ...
   0x03,     //84 T    -
   0x0c,     //85 U    ..-
   0x18,     //86 V    ...-
   0x0e,     //87 W    .--
   0x19,     //88 X    -..-
   0x1d,     //89 Y    -.--
   0x13      //90 Z    --..
};

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
  // Abort sending buffered characters (and clear the buffer)
  // if a paddle is hit. Set "breakin" flag
  //
  if ((eff_kdash || eff_kdot || straight) && (keyer_state >= SNDCHAR_PTT)) {
    breakin=1;
    clearbuf();
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
        for (i=33; i<= 90; i++) {
          if (collecting == morse[i-33]) {
             if (PADDLE_ECHO && hostmode) ToHost(i);
             break;
          }
        }
        collecting=0;
        collpos=0;
      }
      //
      // if there is a long pause, send one space (no matter how long the pause is)
      //
      if (collpos == 0 && sentspace == 0 && actual > last + 6*dotlen) {
         if (PADDLE_ECHO && hostmode) ToHost(32);
         sentspace=1;
      }
      //
      // The dash, dot and straight key contact may be closed at
      // the same time. Priority here is straight > dot > dash
      //
      if (eff_kdash) {
        wait=actual;
        sentspace=0;
        keyer_state=STARTDASH;
        collecting |= (1 << collpos++);
        if (!ptt_stat && PTT_ENABLED) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
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
      }

      if (straight) {
        sentspace=0;
        keyer_state=STARTSTRAIGHT;
        wait=actual;
        if (!ptt_stat && PTT_ENABLED) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
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
      if (actual > wait) {
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
          // Note that the tail-time *ADDS* to the three-dotlen inter-word pause
          // already accounted for, so a non-zero PTT tail should not be necessary
          // in most cases.
          // "wait" will be over-written in due course if there are still characters in the
          // buffer, of if characters arrive over the serial line in due course.
          wait += 10*Tail;
        } else {
          keydown();
          keyer_state=SNDCHAR_ELE;
          wait=actual + ((sending & 0x01) ? dashlen : dotlen);
          sending = (sending >> 1) & 0x7F;
        }
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
  if (bufcnt > 0 && keyer_state==CHECK && !pausing) {
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
        // ignore "non-key'able characters"
        if (byte >='a' && byte <= 'z') byte -= 32;  // convert to lower case
        if (byte > 32 && byte <= 'Z') {
          sending=morse[byte-33];
          if (sending != 1) {
            wait=actual;  // no lead-in wait by default
            if (!ptt_stat && PTT_ENABLED) {
              ptt_on();
              wait=actual+LeadIn*10;
            }
            keyer_state=SNDCHAR_PTT;
          }
        }
        break;
    }
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
      // dump EEPROM command
      // report constants for addr 0 and 15-23
      // report zeroes for addr 24-255
      //
      // Nothing must interrupt us, therefore no state machine
      // at 1200 baud, each character has a mere transmission time
      // of more than 8 msec, so do a pause of 12 msec after each
      // character. Since a complete dump takes much time, check
      // for incoming MIDI messages frequently (every 6 msec or so).
      //

      for (inum=0; inum<256; inum++) {
        if (inum ==  0              ) ToHost(0xA5);
        if (inum >=  1 && inum <= 14) ToHost(EEPROM.read(inum));
        if (inum == 15              ) ToHost(k12ext);
        if (inum == 16              ) ToHost(CmdWpm);
        if (inum == 17              ) ToHost(FreePtr);
        if (inum >= 18 && inum <= 23) ToHost(MsgPtr);
        if (inum >= 24              ) ToHost(0);
        delay(6);
        DrainMIDI();
        delay(6);
        DrainMIDI();
      }
      winkey_state=FREE;
      break;
    case WRPROM:
      //
      // Load EEPROM command. Only load bytes at addr
      // 1 to 14 since we make no use of the others. Write our
      // "own" magic byte to addr 0.
      // Nothing must interrupt us, hence no state machine
      // no delay is required upon reading, but check for
      // incoming MIDI messages after each byte received.
      //
      for (inum=0; inum<256; inum++) {
        while (!ByteAvailable()) ;
        byte=FromHost();
        if (inum == 0              ) EEPROM.update(0, MAGIC);
        if (inum >= 1 && inum <= 14) EEPROM.update(inum, byte);
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
  // (Note ADMIN commands should only be sent if hostmode is closed, with the exception
  // of the CLOSE command).
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
        winkey_state=FREE;
        break;
      case XECHO:
        ToHost(byte);
        winkey_state=FREE;
        break;
      case MESSAGE:
        // output stored message as CW
        winkey_state=FREE;
        break;
      case ADMIN:
        switch (byte) {
          case ADMIN_CALIBRATE:
            winkey_state=SWALLOW;  // expect a byte 0xFF
            break;
          case ADMIN_RESET:
#ifdef HWSERIAL
            Serial.end();
            Serial.begin(1200);
#endif
#ifdef SWSERIAL
            swserial.end();
            swserial.begin(1200);
#endif
            read_from_eeprom();
            hostmode=0;
            HostSpeed=0;
            clearbuf();
            winkey_state=FREE;
            break;
          case ADMIN_OPEN:
            hostmode = 1;
            clearbuf();
            ToHost(WKVERSION);
            winkey_state=FREE;
            break;
          case ADMIN_CLOSE:
            hostmode = 0;
            HostSpeed = 0;
            clearbuf();
            // restore "standalone" settings from EEPROM
            read_from_eeprom();
#ifdef HWSERIAL
            Serial.end();
            Serial.begin(1200);
#endif
#ifdef SWSERIAL
            swserial.end();
            swserial.begin(1200);
#endif
            winkey_state=FREE;
            break;
          case ADMIN_ECHO:
            winkey_state=XECHO;
            break;
          case ADMIN_PAD_A2D:     // Admin Paddle A2D, historical command
          case ADMIN_SPD_A2D:     // Admin Speed  A2D, historical command
          case ADMIN_CMD8:        // K1EL debug only
          case ADMIN_GETCAL:      // Get Cal, historical command
            // For backwards compatibility return a zero
            ToHost(0);
            winkey_state=FREE;
            break;
          case ADMIN_GETVAL:
            // never used so I refrain from making an own "state" for this.
            // we need no delays since the buffer should be able to hold 15
            // bytes.
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
          case ADMIN_LOADXMODE:
            winkey_state=SWALLOW;  // not (yet) implemented, letter spacing etc.
            break;
          case ADMIN_CMD16: // Admin Reserved (return a zero!)
            ToHost(0);
            winkey_state=FREE;
            break;
          case ADMIN_HIGHBAUD: // Admin Set High Baud
#ifdef HWSERIAL
            Serial.end();
            Serial.begin(9600);
#endif
#ifdef SWSERIAL
            swserial.end();
            swserial.begin(9600);
#endif
            winkey_state=FREE;
            break;
          case ADMIN_LOWBAUD: // Admin Set Low Baud
#ifdef HWSERIAL
            Serial.end();
            Serial.begin(1200);
#endif
#ifdef SWSERIAL
            swserial.end();
            swserial.begin(1200);
#endif
            break;
          case ADMIN_CMD19: // Admin reserved
          case ADMIN_CMD20: // Admin reserved
             winkey_state=FREE;
             break;
          default: // Should not occur.
             winkey_state=FREE;
             break;
        }
        break;
      case SIDETONE:
        Sidetone=byte;
        myfreq=4000/(Sidetone & 0x0F);
#ifdef CWKEYERSHIELD
        if (myfreq <= 1270) {
          cwshield.sidetonefrequency((myfreq+5)/10);
        } else {
          cwshield.sidetonefrequency(127);
        }
#endif
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
        if (inum == 0) {
          LeadIn=byte;
          inum=1;
        } else {
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
            delay(50);
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
            myfreq=4000/(Sidetone & 0x0F);
#ifdef CWKEYERSHIELD
            if (myfreq <= 1270) {
              cwshield.sidetonefrequency((myfreq+5)/10);
            } else {
              cwshield.sidetonefrequency(127);
            }
#endif
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
        if (inum == 0) {
          ps1=byte;
          inum=1;
        } else {
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

//////////////////////////////////////////////////////////////////////////////
//
// function to debounce an analog input
//
//////////////////////////////////////////////////////////////////////////////
void analogDebounce(unsigned long actual, int pin, unsigned long *debounce, int *value) {
  int i;

  if (actual < *debounce) return;
  *debounce=actual+20;

  i=analogRead(pin); // 0 - 1023

  *value = (15 * *value / 16 + i) ;  // output value in the range 0 - 16368
}



//////////////////////////////////////////////////////////////////////////////
//
// This is executed again and again at a high rate.
// Most of the time, there will be nothing to do.
//
//////////////////////////////////////////////////////////////////////////////

void loop() {
  int i;
  static uint8_t LoopCounter=0;
#ifdef POTPIN
  static int SpeedPinValue=8000;           // default value: mid position
  static unsigned long SpeedDebounce=0;    // used for "debouncing" speed pot
#endif
  static unsigned long DotDebounce=0;      // used for "debouncing" dot paddle contact
  static unsigned long DashDebounce=0;     // used for "debouncing" dash paddle contact
  static unsigned long StraightDebounce=0; // used for "debouncing" straight key contact
#ifdef CWKEYERSHIELD
  static uint8_t old_sidetone_enabled = 1;
#endif


  //////////////////////////////////////////////////////////////////////////////
  //
  //
  // This sets the "now" time
  //
  //////////////////////////////////////////////////////////////////////////////

  actual=millis();

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
      StraightDebounce=actual+10;
      straight=i;
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
  if (keyer_state == CHECK || num_elements > 5) {
    analogDebounce(actual, POTPIN, &SpeedDebounce, &SpeedPinValue);
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
    case 0:
       //
       // Adjust CW speed
       //
#ifdef POTPIN
      SpeedPot=(SpeedPinValue*WPMrange+8180)/16368; // between 0 and WPMrange
      Speed=MinWPM+SpeedPot;
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
       // Report side tone enable etc. to upstream software
       //
#ifdef CWKEYERSHIELD
       if (old_sidetone_enabled != SIDETONE_ENABLED) {
          old_sidetone_enabled = SIDETONE_ENABLED;
          cwshield.sidetoneenable(old_sidetone_enabled);
       }
#endif
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
