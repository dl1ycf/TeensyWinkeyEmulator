//
// WinKey emulator
//
// (C) Christoph van Wüllen, DL1YCF, 2020/2021
//
//
// Designed for the Teensy 4, using the "Serial+MIDI+Audio" programming model
//
// Using MQS sound output at pin 12 for the side tone
//
// So far tests with applications on my Macintosh:
//
// - works with the "MacWinkeyer" application
// - works with SkookumLogger
// - works with fldigi
// - does key piHPSDR via MIDI messages
//
// MAIN FEATURES
// =============
//
//   LCD Display
//   ===========
//
// - If compiled with LCDDISPLAY #defined, it drives a 2*16 LCD display.
//   The first line contains
//   the characters just sent (both from Paddle or from serial line), and
//   the second line contains status info (speed and side tone frequency).
//
//   MIDI
//   ====
//
// - MIDI is enabled by default, the settings are in TeensyUSBAudioMIDI.h
//
//   Straight key connection
//   =======================
//
// - If compiled with STRAIGHT #defined, a straight key can be connected
//   (the key should connect this input with ground). Straight key takes 
//   precedence over the paddle.
//
//   The input and output pins for Paddle, Straight-Key, CW, PTT, the
//   side tone and digital outputs controlling the LCD are #defined
//   below.
//
//   High-quality side tone
//   ======================
//
//   A high-quality side tone is generated, using either I2S or MQS output
//   (an i2s audio shield is required in the latter case). The Teensy
//   shows up at the computer as a sound card which you can choose from
//   your SDR console application. In this case, the headphone connected
//   to the Teensy (both for MQS and I2S) will have RX audio as well as
//   a low-latency side tone.
//
//   K1EL Winkey (version 2.1) protocol
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
//   If LCD is enabled, the characters sent are displayed in the first line of the LCD
//   panel.
//
//

//
// COMPILE-TIME CONFIGURABLE OPTIONS
//

//#define LCDDISPLAY        //define LCDDISPLAY if you attach an LCD display
#define STRAIGHT 1        //define STRAIGHT if you want to support an additional straight key connected
//
// END OF COMPILE-TIME CONFIGURABLE OPTIONS
//

//
// Hardware lines.
// For simple Arduinos, you cannot use 0 and 1 (hardware serial line),
// and you can only use 2 and 3 for Paddle (since they should trigger interrupts)
//

#define PaddleRight         0      // input for right paddle
#define PaddleLeft          1      // input for left paddle
#define StraightKey         2      // input for straight key

#define TonePin             12     // digital output pin for square wave side tone or MQS
#define CWOUT               9      // CW keying output (active high)
#define PTTOUT              13     // PTT output (active high) (and built-in LED)

#define POTPIN              A0     // analog input pin for the Speed pot, do not define if not connected
#define VOLPIN              A1     // analog input pin for the volume pot, do not define if not connected

#define RSPIN               3      // LCD display RS
#define ENPIN               4      // LCD display EN
#define D4PIN               5      // LCD display D4
#define D5PIN               6      // LCD display D5
#define D6PIN               7      // LCD display D6
#define D7PIN               8      // LCD display D7


////////////////////////////////////////////////////////////////////////////////////////
//
// PC1602F and other displays: used pins (I put the info here so I do not loose it)
//
//  1: Gnd
//  2: +5V
//  3: to center pin of 10k pot connected to GND and +5V (Display contrast)
//  4: RSPIN
//  5: Gnd
//  6: ENPIN
//  7: n.c.
//  8: n.c.
//  9: n.c.
// 10: n.c.
// 11: D4PIN
// 12: D5PIN
// 13: D6PIN
// 14: D7PIH
// 15: to +5V via 220 Ohm resistor (Backlight supply)
// 16: GND
//
////////////////////////////////////////////////////////////////////////////////////////
//
//
// The whole thing is programmed as three (independent) state machines,
//
// keyer state machine:
//  - handles the paddles and produces the dits and dahs,
//  - if the paddle is free it transmits characters from the
//    
// WinKey state machine:
// - handles the serial line and emulates a Winkey (K1EL chip)
//   device. It "talks" with the keyer by putting characters in the character
//   character_buffer
//
// LCD state machine:
// - controls the LCD. It does a minimal amount of work upon each heart beat,
//   that is, it either calls lcd.print or lcd.setCursor *once* per invocation
//   of loop()
//
////////////////////////////////////////////////////////////////////////////////////////

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
      SNDCHAR_PTT,    // aquire PTT, key-down
      SNDCHAR_ELE,    // wait until end of element (dot or dash), key-up
      SNDCHAR_DELAY   // wait until end of delay (inter-element or inter-word)
      } keyer_state=CHECK;

//
// WinKey state machine: the states
// The numerical (enum) values ADMIN ... BUFNOP must agree with the
// winkey command bytes.
//
#define WKVERSION 21  // This version is returned to the host


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
  MESSAGE
} winkey_state=FREE;

#ifdef LCDDISPLAY
//
// state of the LCD machine
//
static uint8_t LCDstate  = 0;
static unsigned long LCDwait = 0;       // pause between two complete "sweeps" of the LCD states
#endif


//
// Internal variables of the winkey machine and the keyer
//


// bits used in PinConfig:
//   b1: side tone on/off
//   b4/b5: PTT tail = 8 dits (11), 4 dits (10), 2 dits (01), 1 dit(00) (in addition to word space)
//
// bits used in ModeRegiser:
//   b6: paddle echoback
//   b3: swap paddles
//   b2: serial echo
//

static uint8_t PinConfig=0x32;          // see above (side tone on/off and hang bits)
static uint8_t Ratio=50;                // dah/dit ratio = (3*ratio)/50
static uint8_t PaddlePoint;             // ignored
static uint8_t Compensation=0;          // Used to modify dit/dah lengths
static uint8_t Extension=0;             // ignored
static uint8_t WPMrange=25;             // CW speed range for SpeedPot
static uint8_t Farnsworth=10;           // Farnsworth speed (10 means: no Farnsworth)
static uint8_t MinWPM=5;                // CW speed when speed pot maximally CCW
static uint8_t Tail=0;                  // PTT tail (in 10 ms), zero means "use hang bits"
static uint8_t LeadIn=15;               // PTT Lead-in time (in units of 10 ms)
static uint8_t Weight=50;               // used to modify dit/dah length
static uint8_t Sidetone=5;              // encodes side tone frequency (b7, "only paddle", is ignored)
static uint8_t Speed=0;                 // CW speed (zero means: use speed pot)
static uint8_t ModeRegister=0;          // see above (echos and swap paddle)

  
static uint8_t WKstatus=0xC0;           // reported to host when it changes

static int inum;                        // counter for number of bytes received in a given state

static uint8_t ps1;                     // first byte of a pro-sign
static uint8_t pausing=0;               // "pause" state
static uint8_t breakin=1;               // breakin state
static uint8_t straight=0;              // state of the straight key (1 = pressed, 0 = released)
static unsigned long StraightDebounce;  // used to debounce straight key input
static unsigned long StraightHang;      // used to implement the straight key hang time
static uint8_t tuning=0;                // "Tune" mode active, deactivate paddle
static uint8_t hostmode  = 0;           // host mode
static uint8_t SpeedPot =  0;           // Speed value from the Potentiometer
static int     myfreq=800;              // current side tone frequency
static uint8_t myspeed;                 // current CW speed
static uint8_t prosign=0;               // set if we are in the middle of a prosign


//
// Ring buffer for characters queued for sending
//
#define BUFLEN 128     // number of bytes in buffer
#define BUFMARGIN 85   // water mark for reporting "buffer almost full"

static unsigned char character_buffer[BUFLEN];  // circular buffer
static int bufrx=0;                             // output (read) pointer
static int buftx=0;                             // input (write) pointer
static int bufcnt=0;                            // number of characters in buffer

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
static volatile int kdot = 0;     // This variable reflects the value of the dot paddle
static volatile int kdash = 0;    // This value reflects the value of the dash paddle
static volatile int memdot=0;     // set, if dot paddle hit since the beginning of the last dash
static volatile int memdash=0;    // set,  if dash paddle hit since the beginning of the last dot

static int dash_held=0;  // dot paddle state at the beginning of the last dash
static int dot_held=0;   // dash paddle state at the beginning of the last dot

static uint8_t ptt_stat=0;   // current PTT status
static uint8_t cw_stat=0;    // current CW output line status

#ifdef LCDDISPLAY
static char line1[17];   // first line of display - one extra byte so we can fill it with snprintf();
static char line2[17];   // second line of display - one extra byte so we can fill it with snprintf();

static int scroll=15;           // scroll counter for first line

#include <LiquidCrystal.h>
const int rs = RSPIN, en=ENPIN, d4=D4PIN, d5=D5PIN, d6=D6PIN, d7=D7PIN;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#endif

#include "src/TeensyUSBAudioMIDI.h"

TeensyUSBAudioMidi teensyusbaudiomidi;

//
// Initialize hardware lines and serial port
// set up interrupt handler for paddle
//
void setup() {  
  Serial.begin(1200);  // baud rate has no meaning on 32U4 and Teensy systems

#ifdef STRAIGHT
  pinMode(StraightKey, INPUT_PULLUP);
#endif    
  pinMode(PaddleLeft,  INPUT_PULLUP);
  pinMode(PaddleRight, INPUT_PULLUP);
  
  pinMode(CWOUT,   OUTPUT);
  digitalWrite(CWOUT,  LOW);  // pull key-down line down
  
  pinMode(PTTOUT,  OUTPUT);
  digitalWrite(PTTOUT, LOW);
  
  attachInterrupt(digitalPinToInterrupt(PaddleLeft),  PaddleHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PaddleRight), PaddleHandler, CHANGE); 

#ifdef LCDDISPLAY
  lcd.begin(16,2);
  lcd.clear();
  lcd.display();
  snprintf_P(line1,17,PSTR("WinKey Emulator    "));
#endif

  teensyusbaudiomidi.setup();
}

//
// "hook" for teensyusbaudio to set the speed
//
void speed_set(int val) 
{
  Speed=val;
}

//
// Interrupt handler for the paddles
//
void PaddleHandler() {
  //
  // Take care of "swap" mode
  //
  if (ModeRegister & 0x08) {
    kdot=!digitalRead(PaddleRight);
    kdash=!digitalRead(PaddleLeft);
  } else { 
    kdot=!digitalRead(PaddleLeft);
    kdash=!digitalRead(PaddleRight);

  }
  if (kdot) memdot=1;
  if (kdash) memdash=1;
}

//
// Read one byte from host
//
int FromHost() {
  int c=Serial.read();
  return c;
}

//
// Write one byte to the host
//
void ToHost(int c) {
  Serial.write(c);
}

//
// "key down" action. 
//
void keydown() {
  if (cw_stat) return;
  cw_stat=1;
  //
  // Actions: side tone on (if enabled), raise hardware line, send MIDI NoteOn message
  //  
  teensyusbaudiomidi.key(1);
  digitalWrite(CWOUT, HIGH);
}

//
// "key up" action.
//
void keyup() {
  if (!cw_stat) return;
  cw_stat=0;
  //
  // Actions: side tone off, drop hardware line, send MIDI NoteOff message
  //  
  teensyusbaudiomidi.key(0);
  digitalWrite(CWOUT, LOW);
}  

//
// "PTT on" action
//
void ptt_on() {
  if (ptt_stat) return;
  ptt_stat=1;
  //
  // Actions: raise hardware line, send MIDI NoteOn message
  //
  digitalWrite(PTTOUT, HIGH);
  teensyusbaudiomidi.ptt(1);
}

//
// "PTT off" action
//
void ptt_off() {
  if (!ptt_stat) return;
  ptt_stat=0;
  //
  // Actions: drop hardware line, send MIDI NoteOff message
  //
  digitalWrite(PTTOUT, LOW);
  teensyusbaudiomidi.ptt(0);
}

//
// clear ring buffer, clear pausing state
//
void clearbuf() {
  bufrx=buftx=bufcnt=0;
  pausing=0;
}

//
// queue up to 3 chars in character_buffer
//
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

//
// remove last queued character
//
void backspace() {
  if (bufcnt < 1) return;
  buftx--;
  if (buftx < 0) buftx=BUFLEN-1;
  bufcnt--;  
  if (bufcnt <= BUFMARGIN) WKstatus &= 0xFE;
}

//
// get next character from character_buffer
//
int FromBuffer() {
  int c;
  if (bufcnt < 1) return 0;
  c=character_buffer[bufrx++];
  if (bufrx >= BUFLEN) bufrx=0;
  bufcnt--;
  if (bufcnt <= BUFMARGIN) WKstatus &= 0xFE;
  return c;
}

//
// Put one character into first line of LCD display
// To this end, the character is put into a cyclic
// (ring) buffer, and the display routine always
// puts the last-entered character at the rightmost
// position of the Display.
//
// This function is defined but a no-op if there is no display.
//
void ToLCD1(char c) {
#ifdef LCDDISPLAY  
  scroll++;
  if (scroll >= 16) scroll=0;
  line1[scroll]=c;
#endif  
}

// Morse code for ASCII characters 33 - 90, from ITU-R M.1677-1
// read from bit0 to bit7
// 0 = dot, 1 = dash
// highest bit set indicated end-of-character (not to be sent as a dash!)

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
  int dotlen;                   // length of dot (msec)
  int dashlen;                  // length of dash (msec)
  int plen;                     // length of delay between dits/dahs
  int clen;                     // inter-character delay in addition to inter-element delay
  int wlen;                     // inter-word delay in addition to inter-character delay
  int hang;                     // PTT tail hang time

  //
  // It is a little overkill to determine all these values at each heart beat,
  // but the uC should have enough horse-power to handle this
  //


  //
  // Standard Morse code timing
  //
  dotlen =1200 / myspeed;          // dot length in milli-seconds
  dashlen=(3*Ratio*dotlen)/50;     // dash length
  plen   =dotlen;                  // inter-element pause
  clen   =2*dotlen;                // inter-character pause = 3 dotlen, one already done (inter-element)
  wlen   =4*dotlen;                // inter-word pause = 7 dotlen, 3 already done (inter-character)

  //
  // PTT hang time from PTTtail or (if this is zero) from the hang bits
  //
  if (Tail != 0) {
    hang=10*Tail;
  } else {
    switch (PinConfig & 0x30) {
      case 0x00: hang = 5*dotlen; break;
      case 0x10: hang = 6*dotlen; break;
      case 0x20: hang = 8*dotlen; break;
      case 0x30: hang =12*dotlen; break;
      default:   hang = 5*dotlen; break;  // cannot happen but makes compiler happy
    }
  }
  
  //
  // Farnsworth timing
  //
  if (Farnsworth > 10 && Farnsworth < myspeed) {
    i=3158/Farnsworth -(31*dotlen)/19;
    clen=3*i - dotlen;                    // stretched inter-character pause
    wlen=7*i - clen;                      // stretched inter-word pause
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
  // Abort sending buffered characters (and clear the buffer)
  // if a paddle is hit. Set "breakin" flag
  //
  if ((kdash || kdot) && (keyer_state >= SNDCHAR_PTT)) {
    breakin=1;
    clearbuf();
    keyer_state=CHECK;
    wait=actual+hang;
  }  

  switch (keyer_state) {
    case CHECK:
      // wait = time when PTT is switched off
      if (actual >= wait) ptt_off();
      if (collpos > 0 && actual > last + 2 * dotlen) {
        // a morse code pattern has been entered and the character is complete
        // echo it in ASCII on the display and on the serial line
        collecting |= 1 << collpos;
        for (i=33; i<= 90; i++) {
          if (collecting == morse[i-33]) {
             if (ModeRegister & 0x40) ToHost(i);           
             ToLCD1(i);         
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
         if (ModeRegister & 0x40) ToHost(32);      
         ToLCD1(32);     
         sentspace=1;
      }
      if (kdash) {
        wait=actual;
        sentspace=0;
        keyer_state=STARTDASH;
        collecting |= (1 << collpos++);
        if (!ptt_stat) {
          ptt_on();
          wait=actual+LeadIn*10;
        }
      }
      // If dot and dash paddle are hit at the same time,
      // dot "wins"
      if (kdot) {
        keyer_state=STARTDOT;
        collpos++;
        wait=actual;
        sentspace=0;
        if (!ptt_stat) {
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
        dash_held=kdash;
        wait=actual+dotlen;
        keydown();
      }
      break;
    case STARTDASH:
      // wait = end of PTT lead-in time
      if (actual >= wait) {
        keyer_state=SENDDASH;
        memdot=0;
        dot_held=kdot;
        wait=actual+dashlen;
        keydown();
      }
      break;     
    case SENDDOT:
      // wait = end of the dot
      if (actual >= wait) {
        last=actual;
        keyup();
        wait=wait+plen;
        keyer_state=DOTDELAY;
      }
      break;
    case DOTDELAY:
      // wait = end of the pause following a dot
      if (actual >= wait) {
        if (!kdot && !kdash) dash_held=0;       
        if (memdash || kdash || dash_held) {
          collecting |= (1 << collpos++);
          keyer_state=STARTDASH;
        } else if (kdot) {
          collpos++;
          keyer_state=STARTDOT;
        } else {
          keyer_state=CHECK;
          wait=actual+hang;
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
        if (!kdot && !kdash) dot_held=0;
        if (memdot || kdot || dot_held) {
          collpos++;
          keyer_state=STARTDOT;
        } else if (kdash) {
          collecting |= (1 << collpos++);
          keyer_state=STARTDASH;
        } else {
          keyer_state=CHECK;
          wait=actual+hang;
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
      case 32:  // space
        // send inter-word distance
        if (ModeRegister & 0x04) ToHost(32);  // echo      
        ToLCD1(32);
        sending=1;
        wait=actual + wlen;
        keyer_state=SNDCHAR_DELAY;
        break;
      default:  
        if (byte >='a' && byte <= 'z') byte -= 32;  // convert to lower case
        if (byte > 32 && byte < 'Z') {
          if (ModeRegister & 0x04) ToHost(byte);  // echo      
          ToLCD1(byte);
          sending=morse[byte-33];
          if (sending != 1) {
            wait=actual;  // no lead-in wait by default
            if (!ptt_stat) {
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

#ifdef LCDDISPLAY
///////////////////////////////////////
//
// This is the LCD state machine
//
///////////////////////////////////////
void LCD_state_machine() {
  uint8_t row, col;
  int i;
  //
  // The state of the LCD engine counts from 0 to 63 in a cyclic fashion.
  // The state variable means:
  //
  // b0 = 0/1: do setCursor or print
  // b1 = 0/1: indicates which row to do
  // b2-b5   : encodes the column
  //
  // This "state machine" programming ensures that we only do a minimal amount of
  // work here during one execution of loop(). If a complete sweep through the
  // display positions has been done, there is a pause of 50 msec.
  //
  if (actual > LCDwait) {
    row=(LCDstate & 0x02) >> 1;
    col=(LCDstate & 0x3C) >> 2;
    if (LCDstate & 0x01) {
      switch (row) {
        case 0:
          i=((col + scroll +1) & 0x0F);
          lcd.print(line1[i]);
          break;
        case 1:
          lcd.print(line2[col]);
          break;
      }
    } else {
      lcd.setCursor(col,row);
    }
    LCDstate++;
    if (LCDstate > 63) {
      LCDstate=0;
      LCDwait=actual+50; // do a complete update sweep every 50 msec
      //
      // Update status line, containing speed, effective speed, and side tone freq
      //
      if (Farnsworth > 10) {
        snprintf_P(line2,17,PSTR("S%d F%d T%d          "), myspeed, Farnsworth, myfreq);
      } else {
        snprintf_P(line2,17,PSTR("S%d F%d T%d          "), myspeed, myspeed, myfreq);
      }
    }
  }
}
#endif

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
  // First, handle one-byte commands
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
    case RDPROM:
      // dump EEPROM command
      ToHost(0);
      inum++;
      if (inum > 255) winkey_state=FREE;
      break;   
    case CANCELSPD:
      // cancel buffered speed, ignored
      winkey_state=FREE;
      break;
    case BUFNOP:
      // buffered no-op; ignored
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
  if (Serial.available()) {
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
      case WRPROM:
        // write EEPROM dummy command
        inum++;
        if (inum > 255) winkey_state=FREE;
        break;
      case MESSAGE:
        // output stored message as CW
        winkey_state=FREE;
        break;
      case ADMIN:
        switch (byte) {
          case 0:     // Admin Calibrate
            // CALIBRATE
            winkey_state=SWALLOW;
            break;
          case 1:     // Admin Reset
            // RESET
            winkey_state=FREE;
            hostmode=0;
            break;
          case 2:     // Admin Open
            // OPEN
            hostmode = 1;
            ToHost(WKVERSION);
            winkey_state=FREE;
            break;
          case 3:     // Admin Close
            hostmode = 0;
            Speed = 0;  // use Speed Pot
            winkey_state=FREE;
            break;
          case 4:     // Admin Echo
            winkey_state=XECHO;
            break;
          case 5:     // Admin 
          case 6:
          case 8:
          case 9:
            // only for backwards compatibility
            ToHost(0);
            break;
          case 7:     // Admin DumpDefault
            // never used so I refrain from making an own "state" for this.
            ToHost(ModeRegister);
            ToHost(Speed);
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
          case 10:      // Admin set WK1 mode
            winkey_state=FREE;
            break;
          case 11:      // Admin set WK2 mode
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
        teensyusbaudiomidi.sidetonefrequency(myfreq);     
        winkey_state=FREE;
        break;
      case WKSPEED:
        Speed=byte;
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
        // Do not bother about lead-in and tail times, but DO switch PTT
        if (byte) {
          clearbuf();
          tuning=1;
          ptt_on();
          delay(50);
          keydown();
        } else {
          keyup();
          delay(50);
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
            Speed=byte;
            break;
          case  2:
            Sidetone=byte;
            myfreq=4000/(Sidetone & 0x0F);
            teensyusbaudiomidi.sidetonefrequency(myfreq);
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
      case POINTER:
        // I have no idea what this *should* do
        // It seems that the number of bytes to "swallow" depends
        // on the second byte of this sequence, but even this is not
        // clear. Need to do an experiment with an original K1EL chip.
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

//
// This is executed again and again at a high rate.
// Most of the time, there will be nothing to do.
//
void loop() {
  int i;
  static uint8_t LoopCounter=0;
  static int SpeedPinValue=0;   // current value of the speed pot
  static int VolPinValue=0;   // current value of the volume pot
  
  //
  //
  // This sets the "now" time
  //
  actual=millis();

#ifdef STRAIGHT
  //
  // handle straight key. This overrides everything else.
  // Use the currently active PTT lead-in time, and for
  // PTT hang use 8 dot length of the currently active speed
  //
  
  if (actual >= StraightDebounce) {
    i=!digitalRead(StraightKey);
    if (i != straight) {
      //
      // status has changed
      //
      straight=i;
      StraightDebounce=actual+10;
      StraightHang=actual+9999;
      if (straight) {
        // key down action
        if (!ptt_stat) {
          ptt_on();
          delay(10*LeadIn);
          //
          // if key already released do not key-down
          // so a very short dit will activate PTT but not produce RF
          //
          if (!digitalRead(StraightKey)) keydown();            
        } else {
          keydown();
        }  
      } else {
        // key up action: do key-up and (re-)set hang time
        keyup();
        StraightHang = actual + 9600/myspeed;
      }
    }  
  } else {
    //
    // While waiting for debounce: read MIDI and drain serial
    //
    teensyusbaudiomidi.loop();
    if (Serial.available()) Serial.read();    
  }
  //
  // If still in "hang" time, do not proceed
  //
  if (actual < StraightHang) return;
  //
  // First time after hang time over: de-activate PTT
  //
  if (StraightHang != 0) {
    ptt_off();
    StraightHang=0;
  }
#endif  

//
// Distribute the remaining work across different executions of loop()
//
  switch (LoopCounter++) {
    case 0:
#ifdef POTPIN
      //
      // read speed pot. Do not act if change is minimal
      // since analog reads cost something
      // Since I experienced instabilities for small valuies,
      // we only use the range 200 - 1000 and re-map this to
      // 0-1000
      //
      i=analogRead(POTPIN); // 0 - 1023
      if (i < 200) i=200;
      if (i > 1000) i=1000;
      i = (10*i - 2000)/8;
      if (i > SpeedPinValue + 50 || i < SpeedPinValue - 50) {
        SpeedPinValue=i;
        SpeedPot=(i*WPMrange)/1000;
      }
#endif      
      break;
    case 2:
#ifdef VOLPIN   
      //
      // read volume pot. Do not act if change is minimal
      // since analog reads cost something.
      // Since I experienced instabilities for small valuies,
      // we only use the range 200 - 1000 and re-map this to
      // 0-1000
      //
      i=analogRead(VOLPIN); // 0 - 1023
      if (i < 200) i=200;
      if (i > 1000) i=1000;
      i = (10*i -2000)/8;
      if (i > VolPinValue + 50 || i < VolPinValue - 50) {
        VolPinValue=i;
        // Convert to level, 0-20
        teensyusbaudiomidi.sidetonevolume((VolPinValue+25)/50);
      }
#endif      
      break;
   case 4:
      //
      // Set speed accordingt to speed pot, send MIDI if speed changed
      //  
      myspeed= Speed;
#ifdef POTPIN      
      if (Speed == 0) myspeed=MinWPM+SpeedPot;
#endif      
      break;
    case 6:
      //
      // one heart-beat of the LCD state machine
      //
#ifdef LCDDISPLAY
      LCD_state_machine();
#endif
      break;
    case 8:
      //
      // One heart-beat of WinKey state machine
      //
      WinKey_state_machine();
      break;
    case 10:
      //
      // This is for checking incoming MIDI messages
      //
      teensyusbaudiomidi.loop();
      break;  
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
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
