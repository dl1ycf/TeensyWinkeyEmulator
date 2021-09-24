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

#ifndef __AVR__

#include <Arduino.h>
#include "TeensyUSBAudioMidi.h"

void TeensyUSBAudioMidi::setup(void)
{
    AudioMemory(32);
    AudioNoInterrupts();

    sine.frequency(default_freq);
    sine_level=default_level;
    sine.amplitude(sine_level);

    if (sgtl5000) {
      sgtl5000->enable();
      sgtl5000->volume(0.8);
    }

    if (tx1_line > 0) {
      pinMode (tx1_line, OUTPUT);
      digitalWrite (tx1_line, LOW);
    }

    if (tx2_line > 0) {
      pinMode (tx2_line, OUTPUT);
      digitalWrite (tx2_line, HIGH);
    }

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
        if (usbMIDI.getType() == usbMIDI.ControlChange && usbMIDI.getChannel() == midi_ctrl) {
            cmd  = usbMIDI.getData1();
            data = usbMIDI.getData2();

            switch(cmd) {
                case 0 :
                    // Set lsb_data for 16-bit values
                    lsb_data = data;
                    break;

                case 4 :
                    // Set WPM
                    speed_set(data);
                    break;

                case 5 :
                    // Set sidetone amplitude
                    lsb_data = (data << 7) | lsb_data;
                    sine_level=float(lsb_data)/16384.0;
                    sine.amplitude(sine_level);
                    break;

                case 6 :
                    // Set sidetone frequency
                    lsb_data = (data << 7) | lsb_data;
                    sine.frequency(float(lsb_data));
                    break;

                case 16 :
                    // Set audio output  volume, multi transfer
                    if (sgtl5000) {
                      lsb_data = (data << 7) | lsb_data;
                      sgtl5000->volume(float(lsb_data)/16384.0);
                    }
                    break;

                default :
                    break;
            }
        }
    }
}

void TeensyUSBAudioMidi::cwspeed(int speed)
{
    //
    // Send MIDI "cw speed" command to pihpsdr (or other SDR programs)
    //
    // pihpsdr maps the full range of controller values (0-127) onto
    // a speed range 1-61, so we have to invert this relation
    //
    int val;

    if (midi_speed >= 0 && midi_chan >=0) {
      val = (127*speed-97)/59;
      if (val > 127) val=127;
      if (val < 0  ) val=0;
      usbMIDI.sendControlChange(midi_speed, val, midi_chan);
    }
}

void TeensyUSBAudioMidi::key(int state)
{
    //
    // Do not set the tone if volume is essentially zero
    //
    int do_midi=midi_cw >= 0 && midi_chan >= 0;

    if (sine_level > 0.001) {
      teensyaudiotone.setTone(state);
    }
    if (state) {
        if (do_midi) {
          usbMIDI.sendNoteOn(midi_cw, 127, midi_chan);
          usbMIDI.send_now();
        }
        if (tx1_line > 0) {
          digitalWrite (tx1_line,HIGH);
        }
        if (tx2_line > 0) {
          digitalWrite (tx1_line, LOW);
        }
    } else {
        if (do_midi) {
          usbMIDI.sendNoteOff(midi_cw, 0, midi_chan);
          usbMIDI.send_now();
        }
        if (tx1_line > 0) {
          digitalWrite (tx1_line, LOW);
        }
        if (tx2_line > 0) {
          digitalWrite (tx2_line, HIGH);
        }
    }
}

void TeensyUSBAudioMidi::ptt(int state)
{
    if (mute_on_ptt) {
      //
      // This mutes the audio from the PC, not the side tone
      //
      teensyaudiotone.muteAudioIn(state);
    }
    if (midi_ptt >= 0 && midi_chan >= 0) {
      if (state) {
        usbMIDI.sendNoteOn(midi_ptt, 127, midi_chan);
      } else {
        usbMIDI.sendNoteOff(midi_ptt, 0, midi_chan);
      }
      usbMIDI.send_now();
    }
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
  sine_level=VolTab[level];
  sine.amplitude(sine_level);
}

void TeensyUSBAudioMidi::sidetonefrequency(int freq) 
{
    sine.frequency((float)freq);
}
#endif