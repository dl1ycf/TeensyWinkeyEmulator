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
#include "TeensyAudioTone.h"
#include "utility/dspinst.h"

#define WINDOW_TABLE_LENGTH 128


//import numpy as np
//import scipy.signal
//a = np.round((2**31)*scipy.signal.windows.hann(259))
//for i in range(1,129):
//  if (i-1)%8 == 0: print()
//  print("{:10d}".format(int(a[i])),end=", ")
const int32_t window_table[WINDOW_TABLE_LENGTH] = {
    318397,    1273398,    2864439,    5090574,    7950483,   11442471,   15564467,   20314026,
  25688331,   31684195,   38298062,   45526009,   53363751,   61806638,   70849664,   80487465,
  90714326,  101524182,  112910621,  124866892,  137385902,  150460228,  164082115,  178243486,
 192935941,  208150767,  223878941,  240111135,  256837722,  274048782,  291734109,  309883213,
 328485332,  347529432,  367004221,  386898147,  407199413,  427895979,  448975571,  470425686,
 492233605,  514386393,  536870912,  559673828,  582781618,  606180576,  629856827,  653796328,
 677984882,  702408144,  727051629,  751900723,  776940687,  802156673,  827533725,  853056793,
 878710740,  904480353,  930350348,  956305383,  982330065, 1008408960, 1034526600, 1060667498,
1086816150, 1112957048, 1139074688, 1165153583, 1191178265, 1217133300, 1243003295, 1268772908,
1294426855, 1319949923, 1345326975, 1370542961, 1395582925, 1420432019, 1445075504, 1469498766,
1493687320, 1517626821, 1541303072, 1564702030, 1587809820, 1610612736, 1633097255, 1655250043,
1677057962, 1698508077, 1719587669, 1740284235, 1760585501, 1780479427, 1799954216, 1818998316,
1837600435, 1855749539, 1873434866, 1890645926, 1907372513, 1923604707, 1939332881, 1954547707,
1969240162, 1983401533, 1997023420, 2010097746, 2022616756, 2034573027, 2045959466, 2056769322,
2066996183, 2076633984, 2085677010, 2094119897, 2101957639, 2109185586, 2115799453, 2121795317,
2127169622, 2131919181, 2136041177, 2139533165, 2142393074, 2144619209, 2146210250, 2147165251,
};


void TeensyAudioTone::update(void)
{
    audio_block_t *block_sine, *block_inl, *block_inr;
    static audio_block_t *block_sidetone=NULL;
    int16_t i, t;

    if (!block_sidetone) {
       block_sidetone=allocate();
       if (!block_sidetone) return;
    }

    block_sine = receiveReadOnly(2);
    if (!block_sine) return;

    block_inl = receiveReadOnly(0);
    block_inr = receiveReadOnly(1);

    if (tone || windowindex) {

        if (tone) {
            // Apply ramp up window and/or send tone to both outputs
            for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                if (windowindex < WINDOW_TABLE_LENGTH) {
                    t = multiply_32x32_rshift32( (block_sine->data[i]) << 1, window_table[windowindex++]);
                } else {
                    t = block_sine->data[i];
                }
                block_sidetone->data[i]=t;
            }
        } else {
            // Apply ramp down until 0 window index
            if (windowindex > WINDOW_TABLE_LENGTH) windowindex = WINDOW_TABLE_LENGTH;

            for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                if (windowindex) {
                    t = multiply_32x32_rshift32( (block_sine->data[i]) << 1, window_table[--windowindex]);
                } else {
                    t = 0;
                }
                block_sidetone->data[i] = t;
            }
        }

        transmit(block_sidetone,0);
        transmit(block_sidetone,1);

    } else {

        windowindex = 0;
        if (block_inl) transmit(block_inl,0);
        if (block_inr) transmit(block_inr,1);
    }

    release(block_sine);
    if (block_inl) release(block_inl);
    if (block_inr) release(block_inr);

}


#undef WINDOW_TABLE_LENGTH


