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

#include <Arduino.h>
#include "TeensyUSBAudioMidi.h"
#include "utility/dspinst.h"

void TeensyUSBAudioMidi::setup(void)
{
    AudioMemory(16);
    AudioNoInterrupts();

    sine.frequency(OPTION_SIDETONE_FREQ);
    sine.amplitude(OPTION_SIDETONE_VOLUME);
#ifndef OPTION_AUDIO_MQS    
    sgtl5000.enable();
    sgtl5000.volume(0.8);
#endif    

    AudioInterrupts();

}

void TeensyUSBAudioMidi::loop(void)
{
    uint cmd, data;
    static uint lsb_data = 0;

    //
    // "swallow" incoming MIDI messages on ANY channel,
    // but only process those on MIDI_CONTROL_CHANNEL.
    // This is to prevent overflows if MIDI messages are
    // sent on the "wrong" channel.
    //
    while (usbMIDI.read()) {
        if (usbMIDI.getType() == usbMIDI.ControlChange && usbMIDI.getChannel() == OPTION_MIDI_CONTROL_CHANNEL) {
            cmd  = usbMIDI.getData1();
            data = usbMIDI.getData2();

            switch(cmd) {
                case 0 :
                    // Set lsb_data for multi transfer
                    lsb_data = data;
                    //Serial.print("lsb ");
                    //Serial.println(lsb_data);
                    break;

                case 4 :
                    // Set WPM
                    speed_set(data);
                    //winkey_port_write(((data - pot_wpm_low_value)|128),0);
                    break;

                case 5 :
                    // Set sidetone amplitude
                    lsb_data = (data << 7) | lsb_data;
                    sine.amplitude(float(lsb_data)/16384.0);
                    //Serial.print("ampl ");
                    //Serial.print(data);
                    //Serial.print(" ");
                    //Serial.println(lsb_data);
                    break;

                case 6 :
                    // Set sidetone frequency
                    lsb_data = (data << 7) | lsb_data;
                    sine.frequency(float(lsb_data));
                    //Serial.print("freq ");
                    //Serial.print(data);
                    //Serial.print(" ");
                    //Serial.println(lsb_data);
                    break;

                case 16 :
                    // Set audio output  volume, multi transfer
                    lsb_data = (data << 7) | lsb_data;
#ifndef OPTION_AUDIO_MQS                    
                    sgtl5000.volume(float(lsb_data)/16384.0);
#endif                    
                    break;

                default :
                    //Serial.print("Unrecognized MIDI command ");
                    //Serial.println(cmd);
                    break;
            }
        }
    }
}

void TeensyUSBAudioMidi::key(int state)
{
    teensyaudiotone.setTone(state);
    if (state) {
        usbMIDI.sendNoteOn(OPTION_MIDI_CW_NOTE, 99, OPTION_MIDI_CW_CHANNEL);
    } else {
        usbMIDI.sendNoteOff(OPTION_MIDI_CW_NOTE, 0, OPTION_MIDI_CW_CHANNEL);
    }
    // These messages are time-critical so flush buffer
    usbMIDI.send_now();
}

void TeensyUSBAudioMidi::ptt(int state)
{
#ifdef OPTION_MIDI_PTT_NOTE
    if (state) {
        usbMIDI.sendNoteOn(OPTION_MIDI_PTT_NOTE, 127, OPTION_MIDI_CW_CHANNEL);
    } else {
        usbMIDI.sendNoteOff(OPTION_MIDI_PTT_NOTE, 0, OPTION_MIDI_CW_CHANNEL);
    }
    // These messages are time-critical so flush buffer
    usbMIDI.send_now();
#endif
}

void TeensyUSBAudioMidi::sidetonevolume(int level) 
{
  //
  // level is the analog value (0-1023) produced by the volume pot.
  // it is converted to the range 0-20 and then the amplitude is
  // taken from VolTab
  //
  if (level <  0) level=0;
  if (level > 20) level=20;
  sine.amplitude(VolTab[level]);
}

void TeensyUSBAudioMidi::sidetonefrequency(int freq) 
{
    sine.frequency(freq);
}
