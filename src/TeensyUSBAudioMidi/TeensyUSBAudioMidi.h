/* TeensyKeyer for Teensy 4.X
 * Copyright (c) 2021, kf7o, Steve Haynal, steve@softerhardware.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TeensyUSBAudioMidi_h_
#define TeensyUSBAudioMidi_h_

#include "Arduino.h"
#include "Audio.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "TeensyAudioTone.h"

enum midi_control_selection {
  MIDI_SET_ACCUM           = 0,
  MIDI_SHIFT_ACCUM         = 1,

  MIDI_MASTER_VOLUME       = 4,
  MIDI_SIDETONE_VOLUME     = 5,
  MIDI_SIDETONE_FREQUENCY  = 6,
  MIDI_CW_SPEED            = 7,
  MIDI_ENABLE_POTS         = 8,

  MIDI_RX_CH               = 16,
  MIDI_TX_CH               = 17,
  MIDI_KEYDOWN_NOTE        = 18,
  MIDI_PTT_MIC_NOTE        = 19,
  MIDI_PTT_IN_NOTE         = 20,
  MIDI_CW_HEADTAIL_NOTE    = 21,
  MIDI_CW_HEADTAIL_ON_RING = 22,
  MIDI_RESPONSE            = 23
};


class TeensyUSBAudioMidi
{
public:
  TeensyUSBAudioMidi(int cw_headtail_mute, int i2s, int freq, double vol,
                     int pin_sidevol, int pin_sidefreq, int pin_mastervol, int pin_speed) :
  sine(),
  usbaudioinput(),
  teensyaudiotone(),
  patchinl (usbaudioinput,   0, teensyaudiotone, 0),
  patchinr (usbaudioinput,   1, teensyaudiotone, 1),
  patchwav (sine,            0, teensyaudiotone, 2)
  {

    mute_on_cw_headtail  = cw_headtail_mute;

    default_freq  = freq;
    default_level = vol;

    Pin_SideToneFrequency = pin_sidefreq;
    Pin_SideToneVolume    = pin_sidevol;
    Pin_MasterVolume      = pin_mastervol;
    Pin_Speed             = pin_speed;

    //
    // Audio output. The audio output method is encoded in the i2s variable:
    //
    // i2s = 0: MQS audio output, no master volume control
    // i2s = 1: I2S audio output, assuming a WM8960   device
    // i2s = 2:       I2S audio output, assuming a SGTL5100 device
    //
    // use MQS as the default if an illegal value has been given
    //
    switch (i2s) {
      case 0:
      default:
        audioout = new AudioOutputMQS;
        break;
      case 1:
        audioout  = new AudioOutputI2S;
        audioin   = new AudioInputI2S;
        wm8960    = new AudioControlWM8960;
        patchusboutl = new AudioConnection(*audioin, 0, usbaudiooutput, 0);
        patchusboutr = new AudioConnection(*audioin, 1, usbaudiooutput, 1);
        break;
      case 2:
        audioout  = new AudioOutputI2S;
        sgtl5000  = new AudioControlSGTL5000;
        break;
    }
    //
    // Solder cables from teensyaudiotone to the just-initialized audio output
    //
    patchoutl = new AudioConnection(teensyaudiotone, 0, *audioout,        0);
    patchoutr = new AudioConnection(teensyaudiotone, 1, *audioout,        1);
  }

  void setup(void);                                         // to be executed once upon startup
  void loop(void);                                          // to be executed at each heart beat
  void midi(void);                                          // MIDI loop
  void pots(void);                                          // Potentiometer loop
  void key(int state);                                      // CW Key up/down event
  void cw_headtail(int state);                              // CW head/tail event
#ifdef POTS_DL1YCF
    bool analogDenoise(int pin, uint16_t *val, uint8_t *old); // De-Noise analog input
#endif
  void mastervolume(uint16_t level);                        // set master volume
  void sidetonevolume(uint16_t level);                      // Change side tone volume
  void sidetonefrequency(uint16_t freq);                    // Change side tone frequency
  void cwspeed(uint16_t speed);                             // send CW speed event
  void sidetoneenable(int onoff) {                          // enable/disable side tone
    teensyaudiotone.sidetoneenable(onoff);
  }

  void set_midi_keydown_note(int v) { midi_keydown_note = v; }
  void set_midi_ptt_mic_note(int v) { midi_ptt_mic_note = v; }
  void set_midi_ptt_in_note(int v) { midi_ptt_in_note = v; }
  void set_midi_cw_headtail_note(int v) { midi_cw_headtail_note = v; }

  void set_midi_rx_ch(int v) { midi_rx_ch = v; }
  void set_midi_tx_ch(int v) { midi_tx_ch = v; }
  void set_cw_headtail_on_ring(int v) { cw_headtail_on_ring = v; }


private:
  AudioSynthWaveformSine  sine;               // free-running side tone oscillator
  AudioInputUSB           usbaudioinput;      // Audio in from Computer
  AudioOutputUSB          usbaudiooutput;     // Audio out to Computer
  TeensyAudioTone         teensyaudiotone;    // Side tone mixer
  AudioConnection         patchinl;           // Cable "L" from Audio-in to side tone mixer
  AudioConnection         patchinr;           // Cable "R" from Audio-in to side tone mixer
  AudioConnection         patchwav;           // Mono-Cable from Side tone oscillator to side tone mixer
  AudioConnection         *patchusboutl=NULL;
  AudioConnection         *patchusboutr=NULL;
  //
  // These are dynamically created, since they depend on the actual
  // audio output device
  //
  AudioStream             *audioout=NULL;     // Audio output to headphone
  AudioStream             *audioin=NULL;
  AudioControlSGTL5000    *sgtl5000=NULL;     // SGTL5000 output controller
  AudioControlWM8960      *wm8960=NULL;       // WM8960 output controller
  AudioConnection         *patchoutl=NULL;    // Cable "L" from side tone mixer to headphone
  AudioConnection         *patchoutr=NULL;    // Cable "R" from side tone mixer to headphone

  float sine_level;                           // store this to detect "no side tone volume"

  //
  // MIDI note/channel values, TX lines.
  // Channel of 0 is disabled as Teensy uses 1-16
  int midi_rx_ch = 1;
  int midi_tx_ch = 1;

  int midi_keydown_note     =  1;
  int midi_ptt_mic_note     = -1;
  int midi_ptt_in_note      = -1;
  int midi_cw_headtail_note = -1;

  // Enable/disable MIDI responses back to software
  int midi_response         = 0;

  // Enable/disable POTS
  uint8_t enable_pots       = 1;

  // For the direct connection to the HL2, do we want the ring asserted (PTT) for head and tail hangs?
  int cw_headtail_on_ring  = 0;


  //
  // (Analog) inputs to monitor. A negative value indicates "do not use this feature"
  //
  int Pin_SideToneFrequency = -1;
  int Pin_SideToneVolume    = -1;
  int Pin_MasterVolume      = -1;
  int Pin_Speed             = -1;

  //
  // current states of the analog input lines,
  // kept for de-noising.
  //
  uint16_t Analog_SideFreq  = 0;
  uint16_t Analog_SideVol   = 0;
  uint16_t Analog_MasterVol = 0;
  uint16_t Analog_Speed     = 0;

  uint16_t last_sidefreq         = 0;
  uint16_t last_sidevol          = 0;
  uint16_t last_mastervol        = 0;
  uint16_t last_speed            = 0;
  //
  // Initial side tone frequency and volume.
  // In normal circumstances, these will be set very soon by the
  // caller of this class
  //
  int  default_freq     = 800;    // default side tone frequency
  float default_level   = 0.2F;   // default side tone volume

  int mute_on_cw_headtail  = 0;     // If set, Audio from PC is muted while PTT is set

  unsigned long last_analog_read = 0;  // time of last analog read
  unsigned int last_analog_line=0;     // which line was read last time

  //
  // Side tone level (amplitude), in 32 steps from zero to one, about 2 dB per step
  // This is used to convert the value from the (linear) volume pot to an amplitude level
  //
  // Note: sidetonevolume() takes an integer argument between 0 and 31, and this data
  //       is then used to convert to an amplitude for the side tone oscillator
  //
  // math.pow(10,-3+x/10.3333) where x is integer value, -3 is number of decades, and 10.3333 is maxx/decades
  // for i in range(0,32): print(math.pow(10, -2+i/15.5))
  //
  // Set first entry to zero to allow for "complete muting"


  // 3 decades
  //float VolTab[32]={0.000,0.00125,0.00156,0.00195,0.00244,0.00305,0.00381,0.00476,
  //                  0.00595,0.00743,0.00928,0.0116,0.0145,0.01812,0.02264,0.02829,
  //                  0.03535,0.04417,0.0552,0.06898,0.0862,0.10771,0.1346,0.1682,
  //                  0.21018,0.26264,0.3282,0.41012,0.51249,0.64041,0.80027,1.0000};

  //2 decades
  float VolTab[32] = {0.00,0.0116,0.0135,0.0156,0.0181,0.021,0.0244,0.0283,
                      0.0328,0.0381,0.0442,0.0512,0.0595,0.069,0.08,0.0928,
                      0.1077,0.125,0.145,0.1682,0.1951,0.2264,0.2626,0.3047,
                      0.3535,0.4101,0.4758,0.552,0.6404,0.743,0.862,1.0};

};

#endif
