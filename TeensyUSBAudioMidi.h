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

//
// Define some MIDI stuff
// Note:no PTT MIDI messages are sent if MIDI_PTT_NOTE is not defined
//
#define MIDI_CW_CHANNEL      5 // channel for sending key-down and ptt messages
#define MIDI_CW_NOTE         1 // Note for key-up/down
#define MIDI_PTT_NOTE        2 // Note for PTT on/off
#define MIDI_CONTROL_CHANNEL 2 // channel for receiving parameters
#define AUDIO_MQS              // define if using MQS audio out instead of I2S

#ifndef AUDIO_MQS
#define AUDIO_I2S              // use I2S if AUDIO_MQS is not defined
#endif

class TeensyUSBAudioMidi
{
public:
    TeensyUSBAudioMidi() :
        usbaudioinput(),
        sine(),
        teensyaudiotone(),
        i2s(),
#ifdef AUDIO_I2S
        sgtl5000(),
#endif                
        patchinl (usbaudioinput,   0, teensyaudiotone, 0),
        patchinr (usbaudioinput,   1, teensyaudiotone, 1),
        patchwav (sine,            0, teensyaudiotone, 2),
        patchoutl(teensyaudiotone, 0, i2s,             0),
        patchoutr(teensyaudiotone, 1, i2s,             1)
    {
    }

    void setup(void);
    void loop(void);
    void key(int state);
    void ptt(int state);
    void sidetonevolume(int level);
    void sidetonefrequency(int freq);


private:
    AudioInputUSB           usbaudioinput;
    AudioSynthWaveformSine  sine;
    TeensyAudioTone         teensyaudiotone;
#ifdef AUDIO_I2S
    AudioOutputI2S          i2s;
    AudioControlSGTL5000    sgtl5000;
#endif
#ifdef AUDIO_MQS
    AudioOutputMQS          i2s;
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
