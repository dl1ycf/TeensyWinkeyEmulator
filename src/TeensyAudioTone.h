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

#ifndef TeensyAudioTone_h_
#define TeensyAudioTone_h_

#include "Arduino.h"
#include "Audio.h"
#include "AudioStream.h"
#include "arm_math.h"

#define SAMPLES_PER_MSEC (AUDIO_SAMPLE_RATE_EXACT/1000.0)

void speed_set(int);

class TeensyAudioTone : public AudioStream
{
public:
    TeensyAudioTone() : AudioStream(3, inputQueueArray) {
        tone = 0;
        hangtime = milliseconds2count(6.0);
        windowindex = 0;
    }

    virtual void update(void);

    void setTone(uint8_t state) {
        tone = state;
    }

    void setHangTime(float milliseconds) {
        hangtime = milliseconds2count(milliseconds);
        speed_set(13);
    }

private:
    uint16_t milliseconds2count(float milliseconds) {
        if (milliseconds < 0.0) milliseconds = 0.0;
        uint32_t c = ((uint32_t)(milliseconds*SAMPLES_PER_MSEC)+7)>>3;
        if (c > 65535) c = 65535; // allow up to 11.88 seconds
        return c;
    }
    audio_block_t *inputQueueArray[3];

    uint8_t tone;         // tone on/off flag

    uint16_t hangtime;
    uint8_t windowindex;  // pointer into the "ramp"
};

#undef SAMPLES_PER_MSEC
#endif
