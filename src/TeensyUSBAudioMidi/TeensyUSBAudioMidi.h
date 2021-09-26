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

class TeensyUSBAudioMidi
{
public:
    TeensyUSBAudioMidi(int cw, int ptt, int spd, int chan, int ctrl,
                       int pttmute, int tx1, int tx2, int i2s,
                       int freq, double vol) :
    sine(),
    usbaudioinput(),
    teensyaudiotone(),
    patchinl (usbaudioinput,   0, teensyaudiotone, 0),
    patchinr (usbaudioinput,   1, teensyaudiotone, 1),
    patchwav (sine,            0, teensyaudiotone, 2) 
    {
      //
      // Copy arguments to local variables
      //
      midi_cw      = cw;
      midi_ptt     = ptt;
      midi_speed   = spd;
      midi_chan    = chan;
      midi_ctrl    = ctrl;

      mute_on_ptt  = pttmute;
      tx1_line     = tx1;
      tx2_line     = tx2;

      default_freq  = freq;
      default_level = vol;

      //
      // Set up I2S (SGTL5000) or MQS audio output
      //
      if (i2s) {
        audioout  = new AudioOutputI2S;
        sgtl5000  = new AudioControlSGTL5000;
      } else {
        audioout  = new AudioOutputMQS;
      }
      //
      // Solder cables from teensyaudioton to the just-initialized audio output
      //
      patchoutl = new AudioConnection(teensyaudiotone, 0, *audioout,        0);
      patchoutr = new AudioConnection(teensyaudiotone, 1, *audioout,        1);
    }

    void setup(void);                           // to be executed once upon startup
    void loop(void);                            // to be executed at each heart beat
    void cwspeed(int speed);                    // send CW speed event
    void key(int state);                        // CW Key up/down event
    void ptt(int state);                        // PTT open/close event
    void sidetonevolume(int level);             // Change side tone volume
    void sidetonefrequency(int freq);           // Change side tone frequency


private:
    AudioInputUSB           usbaudioinput;      // Audio in from Computer
    AudioSynthWaveformSine  sine;               // free-running side tone oscillator
    TeensyAudioTone         teensyaudiotone;    // Side tone mixer
    AudioConnection         patchinl;           // Cable "L" from Audio-in to side tone mixer
    AudioConnection         patchinr;           // Cable "R" from Audio-in to side tone mixer
    AudioConnection         patchwav;           // Mono-Cable from Side tone oscillator to side tone mixer
    //
    // These are dynamically created, depending on whether
    // we use I2S or MQS audio output
    //
    AudioStream             *audioout=NULL;     // Audio output to headphone
    AudioControlSGTL5000    *sgtl5000=NULL;     // I2S output controller
    AudioConnection         *patchoutl=NULL;    // Cable "L" from side tone mixer to headphone
    AudioConnection         *patchoutr=NULL;    // Cable "R" from side tone mixer to headphone

    float sine_level;                           // store this to detect "no side tone volume"

    //
    // MIDI note/channel values, TX lines.
    // A negative value indicates "do not use this feature"
    //
    int midi_cw    = -1;                       // MIDI (key) note for CW up/down event
    int midi_ptt   = -1;                       // MIDI (key) note for PTT on/off event
    int midi_speed = -1;                       // MIDI (controller) note for reporting CW speed
    int midi_chan  = -1;                       // MIDI channel for output to SDR program
    int midi_ctrl  = -1;                       // MIDI channel for input from Computer

    int tx1_line   = -1;                       // if use this for active-high CW
    int tx2_line   = -1;                       // if use this for active-low CW

    //
    // Initial side tone frequency and volume.
    // In normal circumstances, these will be set very soon by the
    // caller of this class
    //
    int  default_freq = 800;                   // default side tone frequency
    float default_level= 0.2;                  // default side tone volume

    int mute_on_ptt  = 0;                      // If set, Audio from PC is muted while PTT is set

    //
    // Side tone level (amplitude), in 20 steps from zero to one, about 2 dB per step
    // This is used to convert the value from the (linear) volume pot to an amplitude level
    //
    // Note: sidetonevolume() takes an integer argument between 0 and 20, and this data
    //       is then used to convert to an amplitude for the side tone oscillator
    //

    float VolTab[21]={0.000,0.0126,0.0158,0.0200,0.0251,0.0316,0.0398,0.0501,0.0631,0.0794,
                      0.100,0.1258,0.1585,0.1995,0.2511,0.3162,0.3981,0.5012,0.6309,0.7943,1.0000};
};

#endif
