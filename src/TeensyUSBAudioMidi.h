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

#include "TeensyUSBAudioMidiConfiguration.h"

//
// Set defaults
//
#ifndef MIDI_CW_CHANNEL
#define MIDI_CW_CHANNEL 1   // use channel 1 as default
#endif
#ifndef MIDI_CONTROL_CHANNEL
#define MIDI_CONTROL_CHANNEL 2  // use channel 2 by default
#endif
#ifndef SIDETONE_VOLUME
#define SIDETONE_VOLUME 0.2
#endif
#ifndef SIDETONE_FREQ
#define SIDETONE_FREQ  800
#endif

class TeensyUSBAudioMidi
{
public:
    TeensyUSBAudioMidi() :
        usbaudioinput(),
        sine(),
        teensyaudiotone(),
        audioout(),
#ifndef AUDIO_MQS
        sgtl5000(),
#endif                
        patchinl (usbaudioinput,   0, teensyaudiotone, 0),
        patchinr (usbaudioinput,   1, teensyaudiotone, 1),
        patchwav (sine,            0, teensyaudiotone, 2),
        patchoutl(teensyaudiotone, 0, audioout,        0),
        patchoutr(teensyaudiotone, 1, audioout,        1)
    {
    }

    void setup(void);                     // to be executed once upon startup
    void loop(void);                      // to be executed at each heart beat
    void key(int state);                  // CW Key up/down event
    void ptt(int state);                  // PTT open/close event
    void sidetonevolume(int level);       // Change side tone volume
    void sidetonefrequency(int freq);     // Change side tone frequency


private:
    AudioInputUSB           usbaudioinput;
    AudioSynthWaveformSine  sine;
	    TeensyAudioTone         teensyaudiotone;
#ifdef AUDIO_MQS
    AudioOutputMQS          audioout;
#else
    AudioOutputI2S          audioout;
    AudioControlSGTL5000    sgtl5000;
#endif
    AudioConnection         patchinl;
    AudioConnection         patchinr;
    AudioConnection         patchwav;
    AudioConnection         patchoutl;
    AudioConnection         patchoutr;

    //
    // Side tone level (amplitude), in 20 steps from zero to one, about 2 dB per step
    // This is used to convert the value from the (linear) volume pot to an amplitude level
    //
    float VolTab[21]={0.000,0.0126,0.0158,0.0200,0.0251,0.0316,0.0398,0.0501,0.0631,0.0794,
                      0.100,0.1258,0.1585,0.1995,0.2511,0.3162,0.3981,0.5012,0.6309,0.7943,1.0000};
    
};

#endif
