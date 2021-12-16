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

//
// This ifndef allows this module being compiled on an Arduino etc.,
// although it offers no function there. Why do we do this? If this
// module is in the src directory, the Arduino IDE will compile it even
// if it is not used.
//
#ifndef __AVR__

#include <Arduino.h>
#include "TeensyUSBAudioMidi.h"

void TeensyUSBAudioMidi::setup(void)
{
    AudioMemory(32);
    AudioNoInterrupts();

    if (Pin_SideToneFrequency >= 0) pinMode(Pin_SideToneFrequency, INPUT);
    if (Pin_SideToneVolume    >= 0) pinMode(Pin_SideToneVolume,    INPUT);
    if (Pin_MasterVolume      >= 0) pinMode(Pin_MasterVolume,      INPUT);
    if (Pin_Speed             >= 0) pinMode(Pin_Speed,             INPUT);

    sine.frequency(default_freq);
    sine_level=default_level;
    sine.amplitude(sine_level);

    if (wm8960) {
      wm8960->enable();
      wm8960->volume(0.8F);
    }
    if (sgtl5000) {
      sgtl5000->enable();
      sgtl5000->volume(0.8F);
    }

    AudioInterrupts();

}

void TeensyUSBAudioMidi::loop(void)
{
    uint cmd, data;
    static uint lsb_data = 0;
    unsigned long now;

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
                    sine_level=float(lsb_data)/16384.0F;
                    sine.amplitude(sine_level);
                    break;

                case 6 :
                    // Set sidetone frequency
                    lsb_data = (data << 7) | lsb_data;
                    sine.frequency(float(lsb_data));
                    break;

                case 16 :
                    // Set audio output  volume, multi transfer
                    lsb_data = (data << 7) | lsb_data;
		    if (wm8960) {
		      wm8960->volume(float(lsb_data)/16384.0F);
		    }
                    if (sgtl5000) {
                      sgtl5000->volume(float(lsb_data)/16384.0F);
                    }
                    break;

                default :
                    break;
            }
        }
    }
    //
    // handle analog lines, but only one analogRead every 5 msec
    // in case of overflow, trigger read.
    // Read all four input lines in round-robin fashin
    //
    now = millis();
    if (now < last_analog_read) last_analog_read = now; // overflow recovery
    if (now > last_analog_read +5) {
      last_analog_read = now;
      switch (last_analog_line++) {
        case 0:  // SideToneFrequency
          if (Pin_SideToneFrequency >= 0) {
            if (analogDenoise(Pin_SideToneFrequency, &Analog_SideFreq, &last_sidefreq)) {
              sidetonefrequency(400+30*last_sidefreq);  // 400 ... 1000 Hz in 30 Hz steps
            }      
          }
          break;
        case 1: // SideToneVolume
          if (Pin_SideToneVolume >= 0) {
            if (analogDenoise(Pin_SideToneVolume, &Analog_SideVol, &last_sidevol)) {
              sidetonevolume(last_sidevol);
            }
          }
          break;
        case 2: // Master Volume
          if (Pin_MasterVolume >= 0) {
            if (analogDenoise(Pin_MasterVolume, &Analog_MasterVol, &last_mastervol)) {
              mastervolume(last_mastervol);
            }
          }
          break;
        case 3: // Speed
          if (Pin_Speed >= 0) {
            if (analogDenoise(Pin_Speed, &Analog_Speed, &last_speed)) {
              cwspeed(10+last_speed);  // 10...30 wpm in steps of 1
            }
          }
          last_analog_line=0;   // roll over
          break;
      }
    }
}

bool TeensyUSBAudioMidi::analogDenoise(int pin, uint16_t *value, uint8_t *old) {
  //
  // low-pass the analog input through a moving exponential average
  // return TRUE if input value has changed
  // Note the value is in the range (0, 16384) for 10-bit analog reads
  //
  // The value is then converted to the scale 0-20 and the new scale value
  // is stored in *old, but only if the new value differs by at least 0.77 delta
  // from the midpoint of the interval of values that produced the old value.
  // In this way, for a pot positioned exactly at the borderline between two
  // readings, one needs to move the pot away from this border to produce a new
  // reading and trigger a change.
  //
  // The return value indicates whether there is a "new" value.
  // Note for 20 steps the interval midpoints are 390 + 780*STEP
  //
  //
  uint16_t newval, midpoint;

  if (pin < 0) return false; // paranoia

  newval = (15 * *value) / 16 + analogRead(pin);  // range 0 - 16368
  if (newval > 16368) newval=16368; // pure paranoia
  *value = newval;

  midpoint = 390 + 780 * *old;  // midpoint of interval corresponding to "old" value

  if (newval > midpoint + 600 || (midpoint > 780 && newval < midpoint - 600)) {
    *old = newval / 780; // range 0 ... 20
    return true;
  } else {
    return false;
  }
}

void TeensyUSBAudioMidi::sidetonefrequency(int freq)
{
    int val;
    sine.frequency((float)freq);

    //
    // Send MIDI "cw frequency" command to pihpsdr (or other SDR programs)
    //
    // pihpsdr maps the full range of controller values (0-127) onto
    // a pitch range from 400-1000, so we have to invert this relation
    //
    if (midi_pitch >= 0 && midi_chan >= 0) {
      val=(127L*freq-50500L)/600;
       if (val > 127) val=127;
      if (val < 0  ) val=0;
      usbMIDI.sendControlChange(midi_pitch, val, midi_chan);
    }
}
    
void TeensyUSBAudioMidi::cwspeed(int speed)
{
    //
    // a) inform keyer about new speed
    //
    speed_set(speed);
    //
    // b) Send MIDI "cw speed" command to pihpsdr (or other SDR programs)
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
    // If side tone is switched off (volume is zero), then
    // do not produce local side tone
    //
    if (sine_level > 0.001) {
      teensyaudiotone.setTone(state);
    }
    if (midi_cw >= 0 && midi_chan >= 0) {
       usbMIDI.sendNoteOn(midi_cw, state ? 127 : 0, midi_chan);
       usbMIDI.send_now();
    }
}

void TeensyUSBAudioMidi::ptt(int state)
{
    if (mute_on_ptt) {
      //
      // This mutes the audio from the PC but not the side tone
      //
      teensyaudiotone.muteAudioIn(state);
    }
    if (midi_ptt >= 0 && midi_chan >= 0) {
      usbMIDI.sendNoteOn(midi_ptt, state ? 127 : 0, midi_chan);
    }
}

void TeensyUSBAudioMidi::sidetonevolume(int level) 
{
  //
  // The input value (level) is in the range 0-20 and converted to
  // an amplitude using VolTab, such that a logarithmic pot is
  // simulated.
  //
  if (level <  0) level=0;
  if (level > 20) level=20;
  sine_level=VolTab[level];
  sine.amplitude(sine_level);
}

void TeensyUSBAudioMidi::mastervolume(int level) 
{
  //
  // The input value (level) is in the range 0-20 and converted to
  // a float parameter between 0 and 1
  //
  if (level <  0) level=0;
  if (level > 20) level=20;
  if (sgtl5000) {
    sgtl5000->volume((float) level * 0.05F);
  }
  if (wm8960) {
    wm8960->volume((float) level * 0.05F);
  }
}
#endif
