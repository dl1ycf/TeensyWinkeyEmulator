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
// Incompatible options (to be removed below):
//
// -  HWSERIAL overrides SWSERIAL
// -  TEENSYUSBAUDIOMIDI overrides USBMIDI and MOCOLUFA.
// -  USBMIDI overrides MOCOLUFA
// -  MOCOLUFA disables HWSERIAL
//
//
// File config.h also defines the hardware pins (digital input, digital output,
// analog input) to be used. Note that when using TEENSYUSBAUDIOMIDI, a speed pot
// should be handled there (below we #undef POTPIN if using TEENSYUSBAUDIOMIDI).				
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "config.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// Eliminate incompatible options
//
////////////////////////////////////////////////////////////////////////////////////////

#ifdef TEENSYUSBAUDIOMIDI
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

////////////////////////////////////////////////////////////////////////////////////////
//
// Some variables are needed, set them to -1 if not defined
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef TEENSY_ANALOG_SIDEVOL
#define TEENSY_ANALOG_SIDEVOL	-1
#endif
#ifndef TEENSY_ANALOG_SIDEFREQ
#define TEENSY_ANALOG_SIDEFREQ	-1
#endif
#ifndef TEENSY_ANALOG_MASTERVOL
#define TEENSY_ANALOG_MASTERVOL -1
#endif
#ifndef TEENSY_ANALOG_SPEED
#define TEENSY_ANALOG_SPEED		-1
#endif

#ifndef MIDI_CW_CHANNEL
#define MIDI_CW_CHANNEL			-1
#endif
#ifndef MIDI_CW_NOTE
#define MIDI_CW_NOTE			-1
#endif
#ifndef MIDI_PTT_NOTE
#define MIDI_PTT_NOTE			-1
#endif
#ifndef MIDI_SPEED_CTRL
#define MIDI_SPEED_CTRL			-1
#endif
#ifndef MIDI_PITCH_CTRL
#define MIDI_PITCH_CTRL			-1
#endif
#ifndef MIDI_CONTROL_CHANNEL
#define MIDI_CONTROL_CHANNEL	-1
#endif 
////////////////////////////////////////////////////////////////////////////////////////
//
//								  MAIN FEATURES
//
////////////////////////////////////////////////////////////////////////////////////////
//
//	 MIDI (only with TEENSYUSBAUDIOMIDI, USBMIDI, or MOCOLUFA)
//	 =========================================================
//
// - key-up/down and PTT-on/off events are sent via MIDI to the computer.
// - TEENSYUSBAUDIOMIDI: incoming MIDI messages my control the keyer
//
//
// - the MIDI details (which notes and channels) are defined in config.h
//
////////////////////////////////////////////////////////////////////////////////////////
//
//	 Straight key connection
//	 =======================
//
// - A straight key can be connected to digital input pin StraightKey.
//	 (the key should connect this input with ground). Straight key takes
//	 precedence over the paddle. If you do not define the StraightKey pin
//	 number in pins.h, this feature is essentially deactivated.
//
//	 In host mode, characters entered via the straight key are decoded and sent
//	 to the host (just as for characters entered via the paddle).
//
////////////////////////////////////////////////////////////////////////////////////////
//
//	 High-quality side tone (only with TEENSYUSBAUDIOMIDI)
//	 =====================================================
//
//	 A high-quality side tone is generated, using either I2S or MQS output
//	 (an i2s audio shield is required in the latter case). The Teensy
//	 shows up at the computer as a sound card which you can choose from
//	 your SDR console application. In this case, the headphone connected
//	 to the Teensy (both for MQS and I2S) will have RX audio as well as
//	 a low-latency side tone.
//
////////////////////////////////////////////////////////////////////////////////////////
//
//	 K1EL Winkey (version 2.3) protocol
//	 ==================================
//
// - Note it is not a full WinKey emulation, just what you need everyday.
//	 Search the internet for a full-featured WinKey emulation. This is OK
//	 for me to do normal QSOs and CW contests, so it is fine for me.
//
//	 It does have
//	 ------------
//	   - PTT output with configurable lead-in and (tail or hang) times
//	   - Weight, Compensation, Ratio etc. as in WinKeyer
//	   - side tone on/off is implemented
//	   - a speed pot
//	   - a volume pot so you can always have the side tone "on"
//
//	 So in standalone mode, this keyer operates on its standard settings,
//	 only the speed can then  be changed with the speed pot.
//	 In "host mode", the settings can be tailored through the WinKey protocol.
//
//	 Note that if a valid "magic" byte is found in the EEPROM, then the settings
//	 are loaded on start-up. The settings can be changed through the WinKey
//	 protocol and are written to EEPROM upon each "host close" command.
//
////////////////////////////////////////////////////////////////////////////////////////
//
//	 Iambic-A/B, Ultimatic, and all that stuff
//	 =========================================
//
//	 These variants differ in the way the keyer works when *both* contacts of the
//	 paddle are closed. Therefore, these differences all vanish if you use a
//	 single-lever paddle where only one contact can be closed at a time.
//
//	 Iambic-A and Iambic-B modes
//	 ---------------------------
//
//	 In the iambic modes, closing both contacts for some time produces a series
//	 of alternating dits and dahs. If only one of the two contacts opens at
//	 a later time, the series is continued with dits or dahs (only).
//
//	 In Iambic-A, if both paddles are released then the elements being sent
//	 is completed and then nothing more. This seems to be the most logical
//	 thing to do.
//	 There exists, however the mode Iambic-B. If both paddles are released
//	 while sending a dah, then the following dit will also be sent. This is
//	 not the place discussing this mode, simply because there are people
//	 used to it.
//
//	 Ultimatic mode
//	 --------------
//
//	 There exists a non-iambic mode for paddles. Briefly, if both contacts
//	 are closed, the last one to be closed takes control, and a series
//	 of dots or dashes is sent. If the last-closed contact is released
//	 while the first one is still closed, the first one gains control
//	 again. So to send a letter "X", close the dash paddle first, the
//	 dot paddle shortly thereafter, and release the dot paddle as
//	 soon you hear the second dot.
//
//	 Note the K1EL chip implements dit/dah "priorities", that is, not the last-closed
//	 contac "wins" but it can be chosen whether dit or dah "wins" when both contacts
//	 are closed. This is *not* implemented here.
//
//	 Bug mode
//	 --------
//
//	 In this mode, the dot paddle is treated normally but the dash paddle like a straight
//	 key. And this is exactly how bug mode is implemented here (treating dash paddle
//	 contacts close/open like straight key contacts close/open)
//
//	 The modes are determined by the bits 4 and 5 of the ModeRegister,
//	 00 = Iambic-B, 01 = Iambic-A, 10 = Ultimatic, 11 = Bug
//
//
////////////////////////////////////////////////////////////////////////////////////////
//
//
// The whole thing is programmed as three (independent) state machines,
//
// keyer state machine:
//	- handles the paddle/straight key and produces the dits and dahs,
//	- if the paddle is free it transmits characters from the character ring buffer
//
// WinKey state machine:
// - handles the serial line and emulates a Winkey (K1EL chip)
//	 device. It "talks" with the keyer by putting characters in the character
//	 riung buffer
//
////////////////////////////////////////////////////////////////////////////////////////

#include <EEPROM.h>
#define MAGIC  0xA5   // EEPROM magic byte

#ifdef TEENSYUSBAUDIOMIDI
#include "src/TeensyUSBAudioMidi/TeensyUSBAudioMidi.h"
#endif

//
// keyer state machine: the states
//
enum KSTAT {
	  CHECK=0,		  // idle
	  SENDDOT,		  // sending a dot
	  SENDDASH,		  // sending a dash
	  DOTDELAY,		  // pause following a dot
	  DASHDELAY,	  // pause following a dash
	  STARTDOT,		  // aquire PTT and start dot
	  STARTDASH,	  // aquire PTT and start dash
	  STARTSTRAIGHT,  // aquire PTT and send "key-down" message
	  SENDSTRAIGHT,   // wait for releasing the straight key and send "key-up" message
	  SNDCHAR_PTT,	  // aquire PTT, key-down
	  SNDCHAR_ELE,	  // wait until end of element (dot or dash), key-up
	  SNDCHAR_DELAY   // wait until end of delay (inter-element or inter-word)
	  } keyer_state=CHECK;

//
// WinKey state machine: the states
// The numerical (enum) values ADMIN ... BUFNOP must agree with the
// winkey command bytes.
//
#define WKVERSION 23  // This version is returned to the host


enum WKSTAT {
  ADMIN		  =0x00,
  SIDETONE	  =0x01,
  WKSPEED	  =0x02,
  WEIGHT	  =0x03,
  PTT		  =0x04,
  POTSET	  =0x05,
  PAUSE		  =0x06,
  GETPOT	  =0x07,
  BACKSPACE   =0x08,
  PINCONFIG   =0x09,
  CLEAR		  =0x0a,
  TUNE		  =0x0b,
  HSCW		  =0x0c,
  FARNS		  =0x0d,
  WK2MODE	  =0x0e,
  LOADDEF	  =0x0f,
  EXTENSION   =0x10,
  KEYCOMP	  =0x11,
  PADSW		  =0x12,
  NULLCMD	  =0x13,
  SOFTPAD	  =0x14,
  WKSTAT	  =0x15,
  POINTER	  =0x16,
  RATIO		  =0x17,
  SETPTT	  =0x18,
  KEYBUF	  =0x19,
  WAIT		  =0x1a,
  PROSIGN	  =0x1b,
  BUFSPD	  =0x1c,
  HSCWSPD	  =0x1d,
  CANCELSPD   =0x1e,
  BUFNOP	  =0x1f,
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

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EEPROM variables:
//
//
// bits used in PinConfig:
//	 b0:	PTT enable/disable bit, if PTT disabled, LeadIn times will have no effect
//	 b1:	Sidetone enable/disable bit
//	 b4/b5: PTT tail = 2 wordspaces (11), 1.67 wordspaces, 1.33 wordspaces, 1.00 wordspaces dit(00)
//
// bits used in ModeRegiser:
//	 b6:	echo characters entered by paddle or straight key (to serial line)
//	 b4/5:	Paddle mode (00 = Iambic-B, 01 = Iambic-A, 10 = Ultimatic, 11 = Bugmode)
//	 b3:	swap paddles
//	 b2:	echo characters received from the serial line as they are transmitted
//

static uint8_t ModeRegister=0x54;		// echos, no swap, Iambic-A by default
static uint8_t Speed=21;				// overridden by the speed pot in standalong mode
static uint8_t HostSpeed=0;				// if nonzero, it overrides other speed settings
static uint8_t Sidetone=5;				// 800 Hz
static uint8_t Weight=50;				// used to modify dit/dah length
static uint8_t LeadIn=15;				// PTT Lead-in time (in units of 10 ms)
static uint8_t Tail=0;					// PTT tail (in 10 ms), zero means "use hang bits"
static uint8_t MinWPM=8;				// CW speed when speed pot maximally CCW
static uint8_t WPMrange=20;				// CW speed range for SpeedPot
static uint8_t Extension=0;				// ignored
static uint8_t Compensation=0;			// Used to modify dit/dah lengths
static uint8_t Farnsworth=10;			// Farnsworth speed (10 means: no Farnsworth)
static uint8_t PaddlePoint;				// ignored
static uint8_t Ratio=50;				// dah/dit ratio = (3*ratio)/50
static uint8_t PinConfig=0x2F;			// PTT and side tone enabled, 1.67 word space hang time, keyer enabled

//
// Macros to read the ModeRegister
//
#define PADDLE_SWAP   (ModeRegister & 0x08)
#define PADDLE_ECHO   (ModeRegister & 0x40)
#define SERIAL_ECHO   (ModeRegister & 0x04)

#define IAMBIC_A	  ((ModeRegister & 0x30) == 0x10)
#define IAMBIC_B	  ((ModeRegister & 0x30) == 0x00)
#define BUGMODE		  ((ModeRegister & 0x30) == 0x30)
#define ULTIMATIC	  ((ModeRegister & 0x30) == 0x20)

//
// Macros to read PinConfig
//
#define SIDETONE_ENABLED (PinConfig & 0x02)
#define PTT_ENABLED		 (PinConfig & 0x01)
#define HANGBITS		 ((PinConfig & 0x30) >> 4)


//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// These settings are stored in the EEPROM upon program start when it is found "empty"
// (the two magic bytes were not found).
// If there is already data in the eprom, these variables are over-written upon program start
// with data from the EEPROM.
//
// EEPROM data is restored upon program start, and upon Admin:Close commands. Thus it governs
// the "standalone" settings.
//
// Note that if using a speed pot, the "speed" value is set to zero upon EEPROM read to make the
// speed pot "active".
//
// After Admin:Open (when entering "hostmode") the standalone
// settings remain effective until explicitly changed.
//
// The EEPROM can be changed through the WinKey protocol "ReadEEPROM" and "WriteEEPROM"
// commands, as implemented in WinKeyer programs available for Windows, MacOS, and Linux.
//
// Upon program start, the program default settings are stored in the EEPROM if is is still "blank".
//
// EEPROM layout:
//
// Byte  0:    MAGIC				 (magic byte)
// Byte  1:    ModeRegister
// Byte  2:    Speed
// Byte  3:    Sidetone
// Byte  4:    Weight
// Byte  5:    LeadIn
// Byte  6:    Tail
// Byte  7:    MinWPM
// Byte  8:    WPMrange
// Byte  9:    Extension
// Byte 10:    Compensation
// Byte 11:    Farnsworth
// Byte 12:    PaddlePoint
// Byte 13:    Ratio
// Byte 14:    PinConfig
// Byte 15:    0x00					(magic byte)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

static uint8_t WKstatus=0xC0;			// reported to host when it changes

static int inum;						// counter for number of bytes received in a given state

static uint8_t ps1;						// first byte of a pro-sign
static uint8_t pausing=0;				// "pause" state
static uint8_t breakin=1;				// breakin state
static uint8_t straight=0;				// state of the straight key (1 = pressed, 0 = released)
static uint8_t tuning=0;				// "Tune" mode active, deactivate paddle
static uint8_t hostmode  = 0;			// host mode
static uint8_t SpeedPot =  0;			// Speed value from the Potentiometer
static int	   myfreq=800;				// current side tone frequency
static uint8_t myspeed;					// current CW speed
static uint8_t prosign=0;				// set if we are in the middle of a prosign
static uint8_t num_elements=0;			// number of elements sent in a sequence

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ring buffer for characters queued for sending
//
#define BUFLEN 128	   // number of bytes in buffer
#define BUFMARGIN 85   // water mark for reporting "buffer almost full"

static unsigned char character_buffer[BUFLEN];	// circular buffer
static int bufrx=0;								// output (read) pointer
static int buftx=0;								// input (write) pointer
static int bufcnt=0;							// number of characters in buffer
//
//////////////////////////////////////////////////////////////////////////////////////////////////////


//
// "sending" encodes the actual character being sent from the character_buffer
// a value of 1 means "nothing to do"
//
static uint8_t sending=0x01;	// bitmap for current morse character to be sent
static uint8_t collecting=0;	// bitmap for character entered with the paddle
static uint8_t collpos=0;		// position for collecting
static uint8_t sentspace=1;		// space already sent for inter-word distance
static unsigned long wait;		// when "actual" reaches this value terminate current keyer state
static unsigned long last;		// time of last enddot/enddash
static unsigned long actual;	// time-stamp for this execution of loop()

//
// variables updated in interrupt service routine
//
static	uint8_t kdot = 0;	   // This variable reflects the value of the dot paddle
static	uint8_t kdash = 0;	   // This value reflects the value of the dash paddle
static	uint8_t memdot=0;	   // set, if dot paddle hit since the beginning of the last dash
static	uint8_t memdash=0;	   // set,	if dash paddle hit since the beginning of the last dot
static	uint8_t lastpressed=0; // Indicates which paddle was pressed last (for ULTIMATIC)
static	uint8_t eff_kdash;	   // effective kdash (may be different from kdash in BUG and ULTIMATIC mode)
static	uint8_t eff_kdot;	   // effective kdot  (may be different from kdot in ULTIMATIC mode)

static uint8_t dash_held=0;  // dot paddle state at the beginning of the last dash
static uint8_t dot_held=0;	 // dash paddle state at the beginning of the last dot

static uint8_t ptt_stat=0;	 // current PTT status
static uint8_t cw_stat=0;	 // current CW output line status

#ifdef TEENSYUSBAUDIOMIDI

void speed_set(int teensyspeed) {
  //
  // Interface to let the TeensyUSBAudioMidi class
  // set the CW speed. Such a change may arise from
  // a speed pot monitored by TeensyUSBAudioMidi or
  // from a MIDI message arriving there.
  //
  Speed=teensyspeed;
}

TeensyUSBAudioMidi teensyusbaudiomidi(AUDIO_OUTPUT,
									  SIDETONE_FREQ,
									  SIDETONE_VOLUME,
									  TEENSY_ANALOG_SIDEVOL,
									  TEENSY_ANALOG_SIDEFREQ,
									  TEENSY_ANALOG_MASTERVOL,
									  TEENSY_ANALOG_SPEED);
									  
#endif

void init_eeprom();


//////////////////////////////////////////////////////////////////////////////
//
// setup:
// Initialize serial port and hardware lines
// init eeprom or load settings from eeprom
// init Audio+MIDI

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
#ifdef PTT1			// active-high PTT output
  pinMode(PTT1, OUTPUT);
  digitalWrite(PTT1, LOW);
#endif
#ifdef PTT2			//active-low PTT output
  pinMode(PTT2, OUTPUT);
  digitalWrite(PTT2, HIGH);
#endif 
	
  init_eeprom();
#ifdef TEENSYUSBAUDIOMIDI
 teensyusbaudiomidi.setup();
//
// In what follows, if the constants are not #define'd,
// The default values hard-wired into TeensyUSBAudioMIDI
// are used.
// Note that these values (no matter if #define'd or default)
// can be over-written through *incoming* MIDI commands
//
#ifdef MY_TX_CHANNEL
  teensyusbaudiomidi.set_midi_tx_ch(MY_TX_CHANNEL);
#endif
#ifdef MY_RX_CHANNEL
  teensyusbaudiomidi.set_midi_rx_ch(MY_RX_CHANNEL);
#endif	   
#ifdef MY_KEYDOWN_NOTE
  teensyusbaudiomidi.set_midi_keydown_note(MY_KEYDOWN_NOTE);
#endif
#ifdef MY_CWPTT_NOTE 
  teensyusbaudiomidi.set_midi_cwptt_note(MY_CWPTT_NOTE);  
#endif
#ifdef MY_SPEED_CNTRL
  teensyusbaudiomidi.set_midi_speed_ctrl(MY_SPEED_CNTRL);
#endif
#ifdef MY_FREQ_CNTRL
  teensyusbaudiomidi.set_midi_freq_ctrl(MY_FREQ_CNTRL);
#endif
#ifdef MY_MUTE_OPTION
  teensyusbaudiomidi.set_cwptt_mute_option(1);
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
// If EEPROM contains valid data (magic byte at 0x00 and zero at 0x0F),
// init variables from EEPROM.
//
// If magic bytes are not found, nothing is done.
//
  uint8_t i;


  if (EEPROM.read(0) == MAGIC && EEPROM.read(15) == 0) {
	 ModeRegister = EEPROM.read( 1);
	 i			  = EEPROM.read( 2);
	 if (i >= 5 && i <= 40) Speed=i;  
	 Sidetone	  = EEPROM.read( 3);
	 Weight		  = EEPROM.read( 4);
	 LeadIn		  = EEPROM.read( 5);
	 Tail		  = EEPROM.read( 6);
	 MinWPM		  = EEPROM.read( 7);
	 WPMrange	  = EEPROM.read( 8);
	 Extension	  = EEPROM.read( 9);
	 Compensation = EEPROM.read(10);
	 Farnsworth   = EEPROM.read(11);
	 PaddlePoint  = EEPROM.read(12);
	 Ratio		  = EEPROM.read(13);
	 PinConfig	  = EEPROM.read(14);
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// write_to_eeprom:
// write magic bytes and current settings to eeprom
//
//////////////////////////////////////////////////////////////////////////////

void write_to_eeprom() {
	EEPROM.update( 0, MAGIC);
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
	EEPROM.update(15, 0x00);
}

//////////////////////////////////////////////////////////////////////////////
//
// init_eeprom: 
// if eeprom magic bytes are not set: write current setting to eeprom
// if eeprom magic bytes are	 set: read	settings from eeptorm
//
// called once upon startup
//////////////////////////////////////////////////////////////////////////////

void init_eeprom() {
  if (EEPROM.read(0) == MAGIC && EEPROM.read(15) == 0) {
	read_from_eeprom();
  } else {
	write_to_eeprom();
  }
}


//////////////////////////////////////////////////////////////////////////////
//
// MIDI might require to drain or process incoming MIDI messages
//
//
//////////////////////////////////////////////////////////////////////////////

void DrainMIDI() {
#ifdef TEENSYUSBAUDIOMIDI
	teensyusbaudiomidi.loop();
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
// for	MIDI events sent via serial line
//
//////////////////////////////////////////////////////////////////////////////
#ifdef MOCOLUFA
static void NoteOn(int note, int chan) {
  Serial.write(0x90 | (--chan & 0x0F));
  Serial.write(note);
  Serial.write(127);
}

static void NoteOff(int note, int chan) {
  Serial.write(0x80 | (--chan & 0x0F));
  Serial.write(note);
  Serial.write(0);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// for	MIDI events sent via USBMIDI
//
//////////////////////////////////////////////////////////////////////////////
#ifdef USBMIDI
static void NoteOn(int note, int chan) {
  usbMIDI.sendNoteOn(note, 127, chan);
  usbMIDI.send_now();
}

static void NoteOff(int note, int chan){
  usbMIDI.sendNoteOff(note, 0, chan);
  usbMIDI.send_now();
}
#endif

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
  // Actions: side tone on (if enabled), raise hardware line, send MIDI NoteOn message
  //
#ifdef TONEPIN											// square wave (if enabled)
  if (SIDETONE_ENABLED) {
	tone(TONEPIN, myfreq);
  }
#endif
#ifdef CW1												// active-high CW output
  digitalWrite(CW1,HIGH);
#endif
#ifdef CW2												//active-low CW line
  digitalWrite(CW2,LOW);
#endif
#if (MIDI_CW_NOTE >= 0 && MIDI_CW_CHANNEL >= 0)		   
#if defined(MOCOLUFA) || defined(USBMIDI)				// MIDI message
  NoteOn(MIDI_CW_NOTE, MIDI_CW_CHANNEL);
#endif
#endif	
#ifdef TEENSYUSBAUDIOMIDI								// MIDI and sidetone
 teensyusbaudiomidi.key(1);
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
#ifdef TONEPIN											// square wave line
  noTone(TONEPIN);
#endif
#ifdef CW1												// active-high CW output
  digitalWrite(CW1,LOW);
#endif
#ifdef CW2
  digitalWrite(CW2,HIGH);								// active-low CW output
#endif
#if (MIDI_CW_NOTE >= 0 && MIDI_CW_CHANNEL >= 0)		   
#if defined(MOCOLUFA) || defined(USBMIDI)				// MIDI message
  NoteOff(MIDI_CW_NOTE, MIDI_CW_CHANNEL);
#endif
#endif	  
#ifdef TEENSYUSBAUDIOMIDI								// MIDI and side tone
 teensyusbaudiomidi.key(0);
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
#ifdef PTT1												  // active high PTT line
  digitalWrite(PTT1,HIGH);
#endif
#ifdef PTT2												  // active low PTT line
  digitalWrite(PTT2,LOW);
#endif
#if (MIDI_PTT_NOTE >= 0 && MIDI_CW_CHANNEL >= 0)   
#if defined(MOCOLUFA) || defined(USBMIDI)				  // MIDI message
  NoteOn(MIDI_PTT_NOTE, MIDI_CW_CHANNEL);
#endif
#endif	  
#ifdef TEENSYUSBAUDIOMIDI	
  teensyusbaudiomidi.cwptt(1);
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
  digitalWrite(PTT1,LOW);								  // active high PTT line
#endif
#ifdef PTT2
  digitalWrite(PTT2,HIGH);								  // active low PTT line
#endif
#if (MIDI_PTT_NOTE >= 0 && MIDI_CW_CHANNEL >= 0)	
#if defined(MOCOLUFA) || defined(USBMIDI)				   // MIDI message
  NoteOff(MIDI_PTT_NOTE, MIDI_CW_CHANNEL);
#endif
#endif	 
#ifdef TEENSYUSBAUDIOMIDI	
  teensyusbaudiomidi.cwptt(0);
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
}

//////////////////////////////////////////////////////////////////////////////
//
// Insert zeroes, for pointer commands
// This goes to ABSOLUTE POSITION
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
   0x01,	 //33 !    not in ITU-R M.1677-1
   0x52,	 //34 "    .-..-.
   0x01,	 //35 #    not in ITU-R M.1677-1
   0x01,	 //36 $    not in ITU-R M.1677-1
   0x01,	 //37 %    not in ITU-R M.1677-1
   0x01,	 //38 &    not in ITU-R M.1677-1
   0x5e,	 //39 '    .----.
   0x36,	 //40 (    -.--.
   0x6d,	 //41 )    -.--.-
   0x01,	 //42 *    not in ITU-R M.1677-1
   0x2a,	 //43 +    .-.-.
   0x73,	 //44 ,    --..--
   0x61,	 //45 -    -....-
   0x6A,	 //46 .    .-.-.-
   0x29,	 //47 /    -..-.
   0x3f,	 //48 0    -----
   0x3e,	 //49 1    .----
   0x3c,	 //50 2    ..---
   0x38,	 //51 3    ...--
   0x30,	 //52 4    ....-
   0x20,	 //53 5    .....
   0x21,	 //54 6    -....
   0x23,	 //55 7    --...
   0x27,	 //56 8    ---..
   0x2f,	 //57 9    ----.
   0x78,	 //58 :    ---...
   0x01,	 //59 ;    not in ITU-R M.1677-1
   0x01,	 //60 <    not in ITU-R M.1677-1
   0x31,	 //61 =    -...-
   0x01,	 //62 >    not in ITU-R M.1677-1
   0x4c,	 //63 ?    ..--..
   0x56,	 //64 @    .--.-.
   0x06,	 //65 A    .-
   0x11,	 //66 B    .---
   0x15,	 //67 C    -.-.
   0x09,	 //68 D    -..
   0x02,	 //69 E    .
   0x14,	 //70 F    ..-.
   0x0b,	 //71 G    --.
   0x10,	 //72 H    ....
   0x04,	 //73 I    ..
   0x1e,	 //74 J    .---
   0x0d,	 //75 K    -.-
   0x12,	 //76 L    .-..
   0x07,	 //77 M    --
   0x05,	 //78 N    -.
   0x0f,	 //79 O    ---
   0x16,	 //80 P    .--.
   0x1b,	 //81 Q    --.-
   0x0a,	 //82 R    .-.
   0x08,	 //83 S    ...
   0x03,	 //84 T    -
   0x0c,	 //85 U    ..-
   0x18,	 //86 V    ...-
   0x0e,	 //87 W    .--
   0x19,	 //88 X    -..-
   0x1d,	 //89 Y    -.--
   0x13		 //90 Z    --..
};

///////////////////////////////////////
//
// This is the Keyer state machine
//
///////////////////////////////////////

void keyer_state_machine() {
  int i;						// general counter variable
  uint8_t byte;					// general one-byte variable
  int dotlen;					// length of dot (msec)
  int dashlen;					// length of dash (msec)
  int plen;						// length of delay between dits/dahs
  int clen;						// inter-character delay in addition to inter-element delay
  int wlen;						// inter-word delay in addition to inter-character delay
  int hang;						// PTT tail hang time

  static unsigned long straight_pressed;  // for timing straight key signals

  //
  // It is a little overkill to determine all these values at each heart beat,
  // but the uC should have enough horse-power to handle this
  //


  //
  // Standard Morse code timing
  //
  dotlen =1200 / myspeed;		   // dot length in milli-seconds
  dashlen=(3*Ratio*dotlen)/50;	   // dash length
  plen	 =dotlen;				   // inter-element pause
  clen	 =2*dotlen;				   // inter-character pause = 3 dotlen, one already done (inter-element)
  wlen	 =4*dotlen;				   // inter-word pause = 7 dotlen, 3 already done (inter-character)

  //
  // Farnsworth timing: strecht inter-character and inter-word pauses
  //
  if (Farnsworth > 10 && Farnsworth < myspeed) {
	i=3158/Farnsworth -(31*dotlen)/19;
	clen=3*i - dotlen;					  // stretched inter-character pause
	wlen=7*i - clen;					  // stretched inter-word pause
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
	dotlen	+= Compensation;
	dashlen += Compensation;
	plen	-= Compensation;
  }

  //
  // PTT hang time from PTTtail.
  // If it is zero, or PTT not enabled, use hang bits
  // Note: WK2.3 changed hang times
  //
  if (Tail != 0 && PTT_ENABLED) {
	hang=10*Tail;
  } else {
	switch (HANGBITS) {
	  case 0: hang =  8*dotlen; break;	   // word space + 1 dot
	  case 1: hang =  9*dotlen; break;	   // word space + 2 dots
	  case 2: hang = 11*dotlen; break;	   // word space + 4 dots
	  case 3: hang = 15*dotlen; break;	   // word space + 8 dots
	}
  }

  //
  // Abort sending buffered characters (and clear the buffer)
  // if a paddle is hit. Set "breakin" flag
  //
  if ((eff_kdash || eff_kdot || straight) && (keyer_state >= SNDCHAR_PTT)) {
	breakin=1;
	clearbuf();
	keyer_state=CHECK;
	wait=actual+hang;
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
		if (actual	> straight_pressed + 2*plen) {
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
		  // character sent completely
		  keyer_state=CHECK;
		  if (bufcnt > 0) wait=actual+3*dotlen; // when buffer empty, go RX immediately
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
  //
  if (bufcnt > 0 && keyer_state==CHECK && !pausing) {
	//
	// transfer next character to "sending"
	//
	byte=FromBuffer();
	switch (byte) {
	  case PROSIGN:
		prosign=1;
		break;
	  case BUFNOP:
		break;
	  case 32:	// space
		// send inter-word distance
		if (SERIAL_ECHO) ToHost(32);  // echo
		sending=1;
		wait=actual + wlen;
		keyer_state=SNDCHAR_DELAY;
		break;
	  default:
		if (byte >='a' && byte <= 'z') byte -= 32;	// convert to lower case
		if (byte > 32 && byte < 'Z') {
		  if (SERIAL_ECHO) ToHost(byte);  // echo
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
  static int OldWKstatus=-1;		   // this is to detect status changes
  static int OldSpeedPot=-1;		   // this is to detect Speed pot changes

  //
  // Now comes the WinKey state machine
  // First, handle commands that need no further bytes
  //
  switch (winkey_state) {
	case GETPOT:
	  // send immediately
	  ToHost(128+SpeedPot);
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
	case CANCELSPD:
	  // cancel buffered speed, ignored
	  winkey_state=FREE;
	  break;
	case BUFNOP:
	  // buffered no-op; ignored
	  winkey_state=FREE;
	  break;
	 case RDPROM:
	  // dump EEPROM command
	  // only dump bytes 0 through 15,
	  // report the others being zero
	  // Nothing must interrupt us, therefore no state machine
	  // at 1200 baud, each character has a mere transmission time
	  // of more than 8 msec, so do a pause of 12 msec after each
	  // character. Since a complete dump takes much time, check
	  // for incoming MIDI messages frequently.
	  //
	  for (inum=0; inum<16; inum++) {
		ToHost(EEPROM.read(inum));
		DrainMIDI();	  
		delay(12); // at 1200 baud, each character takes about 9 msec
	  }
	  for (inum=16; inum < 256; inum++) {
		  ToHost(0);
		  DrainMIDI();		  
		  delay(12);
	  }
	  winkey_state=FREE;
	  break;
	case WRPROM:
	  //
	  // Load EEPROM command
	  // nothing must interrupt us, hence no state machine
	  // no delay is required upon reading, but check for
	  // incoming MIDI messages frequently.
	  //
	  for (inum=0; inum<16; inum++) {
		while (!ByteAvailable()) ;
		byte=FromHost();
		EEPROM.update(inum, byte);
		DrainMIDI();	   
	  }
	  for (inum=16; inum<256; inum++) {
		while (!ByteAvailable()) ;
		byte=FromHost();
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
  // NOTE: process *all* ADMIN command even if hostmode is closed. For example,
  // fldigi sends "echo" first and then "open".
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
		  case 0:	  // Admin Calibrate
			// CALIBRATE
			winkey_state=SWALLOW;
			break;
		  case 1:	  // Admin Reset
			// RESET
			winkey_state=FREE;
			read_from_eeprom();
			hostmode=0;
			HostSpeed=0;
			break;
		  case 2:	  // Admin Open
			// OPEN
			hostmode = 1;
			ToHost(WKVERSION);
			winkey_state=FREE;
			break;
		  case 3:	  // Admin Close
			hostmode = 0;
			HostSpeed = 0; 
			// restore "standalone" settings from EEPROM
			read_from_eeprom();
			winkey_state=FREE;
			break;
		  case 4:	  // Admin Echo
			winkey_state=XECHO;
			break;
		  case 5:	  // Admin
		  case 6:
		  case 8:
		  case 9:
			// only for backwards compatibility
			ToHost(0);
			break;
		  case 7:	  // Admin DumpDefault
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
		  case 10:		// Admin set WK1 mode
			winkey_state=FREE;
			break;
		  case 11:		// Admin set WK2 mode
			winkey_state=FREE;
			break;
		  case 12:
			winkey_state=RDPROM;
			break;
		  case 13:
			winkey_state=WRPROM;
			break;
		  case 14:
			winkey_state=MESSAGE;
			break;
		}
		break;
	  case SIDETONE:
		Sidetone=byte;
		myfreq=4000/(Sidetone & 0x0F);		 
		winkey_state=FREE;
		break;
	  case WKSPEED:
		HostSpeed=byte;
#ifdef TEENSYUSBAUDIOMIDI		 
		if (HostSpeed != 0) {
		  teensyusbaudiomidi.cwspeed(HostSpeed);
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
			delay(150);
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
		  case	0:
			ModeRegister=byte;
			break;
		  case	1:
			HostSpeed=byte;
#ifdef TEENSYUSBAUDIOMIDI		 
			if (HostSpeed != 0) {
			  teensyusbaudiomidi.cwspeed(HostSpeed);
			}
#endif
			break;
		  case	2:
			Sidetone=byte;
			myfreq=4000/(Sidetone & 0x0F);		   
			break;
		  case	3:
			Weight=byte;
			break;
		  case	4:
			LeadIn=byte;
			break;
		  case	5:
			Tail=byte;
			break;
		  case	6:
			MinWPM=byte;
			break;
		  case	7:
			WPMrange=byte;
			if (WPMrange > 31) WPMrange=31;
			break;
		  case	8:
			Extension=byte;
			break;
		  case	9:
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
		  default: winkey_state=FREE;	   break;
		}
		break;
	  case RATIO:
		if (byte < 33) byte=33;
		if (byte > 66) byte=66;
		Ratio=byte;
		winkey_state=FREE;
		break;
	  case SETPTT:
		// buffered PTT ignored
		winkey_state=FREE;
		break;
	  case KEYBUF:
		//buffered key-down ignored
		winkey_state=FREE;
		break;
	  case WAIT:
		// buffered wait ignored
		winkey_state=FREE;
		break;
	  case PROSIGN:
		if (inum == 0) {
		  ps1=byte;
		  inum=1;
		} else {
		 // character_buffer prosign command
		 queue(3,0x1b,ps1,byte);
		 winkey_state=FREE;
		}
		break;
	  case BUFSPD:
		// buffered speed change ignored
		winkey_state=FREE;
		break;
	  case HSCWSPD:
		// HSCW speed change ignored
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
	ToHost(128+SpeedPot);
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
  static int SpeedPinValue=8000;		   // default value: mid position
  static unsigned long SpeedDebounce=0;    // used for "debouncing" speed pot
#endif
  static unsigned long DotDebounce=0;	   // used for "debouncing" dot paddle contact
  static unsigned long DashDebounce=0;	   // used for "debouncing" dash paddle contact
  static unsigned long StraightDebounce=0; // used for "debouncing" straight key contact
#ifdef TEENSYUSBAUDIOMIDI
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
  //	 logical-OR the dash to the straight keyer contact,
  //	 and let the effective dash contact always "open"
  //
  // ULTIMATIC MODE:
  //	 never report "both contacts closed" to the keyer.
  //	 in this case, only the last-pressed contact wins.
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
	   if (keyer_state == CHECK || num_elements > 5) myspeed=Speed; // do not change speed in the midst of a letter
	   SpeedPot = Speed - MinWPM; // maintain "virtual" value of speed pot
	   if (SpeedPot > 31) SpeedPot=31;
#endif		 
	   //
	   // HostMode speed overrides "local" speed
	   //
	   if (HostSpeed == 0) {
		 myspeed=Speed;
	   } else {
		 myspeed=HostSpeed;		  
	   }
	  break;
	case 2:
	   //
	   // Report side tone enable etc. to upstream software
	   //
#ifdef TEENSYUSBAUDIOMIDI	 
	   if (old_sidetone_enabled != SIDETONE_ENABLED) {
		  old_sidetone_enabled = SIDETONE_ENABLED;
		  teensyusbaudiomidi.sidetoneenable(old_sidetone_enabled);
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
