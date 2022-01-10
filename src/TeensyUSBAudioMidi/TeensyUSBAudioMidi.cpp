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

#ifndef DL1YCF_POTS
    analogReadRes(12);
    analogReadAveraging(40);
#endif

}

void TeensyUSBAudioMidi::loop(void)
{
    pots();
    midi();
}


void TeensyUSBAudioMidi::midi(void)
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
                    // Set WPM, inform SDR program via MIDI
                    cwspeed(data);
                    break;

                case 5 :
                    // Set sidetone amplitude
                    lsb_data = (data << 7) | lsb_data;
                    sine_level=float(lsb_data)/16384.0F;
                    sine.amplitude(sine_level);
                    break;

                case 6 :
                    // Set sidetone frequency, inform SDR program via MIDI
                    lsb_data = (data << 7) | lsb_data;
		    sidetonefrequency(lsb_data),
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
}


#ifdef DL1YCF_POTS

void TeensyUSBAudioMidi::pots() {
    unsigned long now;
    //
    // handle analog lines, but only one analogRead every 5 msec
    // in case of overflow, trigger read.
    // Read all four input lines in round-robin fashion
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
  // Read analog input from pin #pin, convert value to a scale 0-20.
  // "old" and "value" need to be conserved between calls.
  // "value" is needed for low-passing, and "old" is given the new
  // reading in the range 0-20 if the analog input value has changed.
  // This function returns "true" if the value has changed, and "false"
  // if there is no update.
  //
  // Here comes DL1YCF's "black magic" to de-noise the analog input:
  //
  // The result of analogRead() is low-passed through a moving exponential average.
  // The result of the low-passing is stored in "value" and is in the range
  // (0, 163368) for 10-bit analog reads (16368 = 16*1023).
  //
  // The value is then converted to the scale 0-20 and the new scale value
  // is stored in *old. The conversion is done as follows:
  //
  // value =     0 ...   779   ==> reading =  0
  // value =   780 ...  1559   ==> reading =  1
  // ...
  // value = 15600 ... 16368   ==> reading = 20
  //
  // In this section: VALUE is the low-passed analog reading in the
  // range 0...16386, while READING is the returned value in the range
  // 0..20.
  // While the exponential averaging implements a low-pass filter so we get
  // rid of high-frequency oscillations in VALUE, we have to guard
  // against small-amplitude low-frequency oscillations as well. These can
  // occur if VALUE is close to a multiple of 780 (that is, close to the
  // border of two intervals leading to different READINGs).
  // So we remember the previous READING, compute the mid-point of the
  // interval of VALUEs that possibly lead to this READING, and require that
  // the new VALUE differs from that midpoint by at least 600.
  // This means, if the pot is close to the borderline, we have to move the pot
  // away from this borderline before reporting a new READING.
  //
  // We have to do this, because slowly varying READINGs for the same
  // pot position are quite unpleasant, especially if it affects the
  // side tone frequency.
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

#else

void TeensyUSBAudioMidi::pots()
{
    uint16_t analog_data;
    unsigned long m = millis();

    // Call every 10 ms, will handle rollover as both m and last_analog_read are unsigned long
    if ((m - last_analog_read) > 10) {

        if (last_analog_line == 0) {
            // Master volume
            analog_data = analogRead(Pin_MasterVolume);
            // At 12 bit this averaging will produce a value in the 0 to 8191 range, 13 bits
            Analog_MasterVol = (Analog_MasterVol >> 1) + analog_data;

            // Volume is 80 values, so only care if more than about 2**7 bit change
            if (abs(Analog_MasterVol - last_mastervol) > 64) {
                mastervolume( ((float)(8191-Analog_MasterVol)) / 8191.0);
                last_mastervol = Analog_MasterVol;
                //Serial.print("MV ");
                //Serial.println(analog_data);
            }
        } else if (last_analog_line == 1) {

            // Sidetone volume
            analog_data = analogRead(Pin_SideToneVolume);
            Analog_SideVol = (Analog_SideVol >> 1) + analog_data;
            // Sidetone volume has 32 entries so 5 bits
            if (abs(Analog_SideVol - last_sidevol) > 256) {
                sidetonevolume((8191-Analog_SideVol)>>8);
                last_sidevol = Analog_SideVol;
                //Serial.print("SV ");
                //Serial.println(analog_data);
            }
        } else if (last_analog_line == 2) {

            // Sidetone frequency
            analog_data = analogRead(Pin_SideToneFrequency);
            Analog_SideFreq = (Analog_SideFreq >> 1) + analog_data;
            // Range between 250 and 1274, 10 bits
            if (abs(Analog_SideFreq - last_sidefreq) > 64) {
                // Range between 100 and 2147 hz
                sidetonefrequency(((8191-Analog_SideFreq) >> 3) + 250);
                last_sidefreq = Analog_SideFreq;
                //Serial.print("SF ");
                //Serial.println(analog_data);
            }
        } else if (last_analog_line == 3) {

            // Speed
            analog_data = analogRead(Pin_Speed);
            Analog_Speed = (Analog_Speed >> 1) + analog_data;
            // range between 3 and 67 WPM, 6 bits
            if (abs(Analog_Speed - last_speed) > 128) {
                // range between 1 and 65 WPM
                cwspeed( ((8191-Analog_Speed)>>7) + 3);
                last_speed = Analog_Speed;
                //Serial.print("SP ");
                //Serial.println(pot_speed);
                //Serial.print(" ");
                //Serial.println(analog_data);
            }
        }

        last_analog_read = m;
        last_analog_line = (last_analog_line + 1) & 0x3;
    }
}

#endif

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
    teensyaudiotone.setTone(state);
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
  // The input value (level) is in the range 0-31 and converted to
  // an amplitude using VolTab, such that a logarithmic pot is
  // simulated.
  //
  if (level <  0) level=0;
  if (level > 31) level=31;
  sine_level=VolTab[level];
  sine.amplitude(sine_level);
}

void TeensyUSBAudioMidi::mastervolume(float level)
{
// level 0.0 to 1.0
  if (sgtl5000) {
    sgtl5000->volume(level);
  }
  if (wm8960) {
    wm8960->volume(level);
  }
}
#endif
