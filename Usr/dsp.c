/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "stm32f1xx.h"
// #error "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"

#include <arm_math.h>
#include "nanovna.h"

int16_t samp_buf[SAMPLE_LEN];  // 48
int16_t ref_buf[SAMPLE_LEN];  // 48

/*

����Ҷ�任֮��õ���ÿ���㶼�Ǹ�������a+bi
�����ǣ������£�a^2+b^2��
��λ�ǣ�arctan(b/a)
ʵ���ǣ�a
�鲽�ǣ�b
���Ⱥ���λ�����һ�𣬾�����ȫ��ʾ����Ҷ�任�Ľ����
ʵ�����鲽�����һ��Ҳ����ȫ��ʾ��

a+bi�ı�﷽ʽȷʵ�����Ѻã��任�ɵȼ۵�r�Ϧȵ���ʽ�ǲ��Ǿ�˳�۶��ˣ�
������ֱ�۵�״����r����ĳ�������źŵķ��ȣ�������������λ��

P1_pi=asin(10533/32767);  % P1_pi/2/pi*360 18.7507��
P2_pi=acos(31029/32767);  % P1_pi/2/pi*360 18.7449��

*/
/* Fs = 48000 */
const int16_t sincos_tbl[48][2] = {
  { 10533,  31029 }, { 27246,  18205 }, { 32698,  -2143 }, { 24636, -21605 },
  {  6393, -32138 }, {-14493, -29389 }, {-29389, -14493 }, {-32138,   6393 },
  {-21605,  24636 }, { -2143,  32698 }, { 18205,  27246 }, { 31029,  10533 },
  { 31029, -10533 }, { 18205, -27246 }, { -2143, -32698 }, {-21605, -24636 },
  {-32138,  -6393 }, {-29389,  14493 }, {-14493,  29389 }, {  6393,  32138 },
  { 24636,  21605 }, { 32698,   2143 }, { 27246, -18205 }, { 10533, -31029 },
  {-10533, -31029 }, {-27246, -18205 }, {-32698,   2143 }, {-24636,  21605 },
  { -6393,  32138 }, { 14493,  29389 }, { 29389,  14493 }, { 32138,  -6393 },
  { 21605, -24636 }, { 2143,  -32698 }, {-18205, -27246 }, {-31029, -10533 },
  {-31029,  10533 }, {-18205,  27246 }, {  2143,  32698 }, { 21605,  24636 },
  { 32138,   6393 }, { 29389, -14493 }, { 14493, -29389 }, { -6393, -32138 },
  {-24636, -21605 }, {-32698,  -2143 }, {-27246,  18205 }, {-10533,  31029 }
};

int32_t acc_samp_s;
int32_t acc_samp_c;
int32_t acc_ref_s;
int32_t acc_ref_c;

void
dsp_process(int16_t *capture, size_t length)  // length = 96
{
  uint32_t *p = (uint32_t*)capture;
  uint32_t len = length / 2;
  uint32_t i;
  int32_t samp_s = 0;
  int32_t samp_c = 0;
  int32_t ref_s = 0;
  int32_t ref_c = 0;

  for (i = 0; i < len; i++) {  // 48��
    uint32_t sr = *p++;
    int16_t ref = sr & 0xffff;  // ������
    int16_t smp = (sr>>16) & 0xffff;  // ������
    ref_buf[i] = ref;
    samp_buf[i] = smp;
    int32_t s = sincos_tbl[i][0];
    int32_t c = sincos_tbl[i][1];
    samp_s += smp * s / 16;
    samp_c += smp * c / 16;
    ref_s += ref * s / 16;
    ref_c += ref * c / 16;
#if 0
    uint32_t sc = *(uint32_t)&sincos_tbl[i];
    samp_s = __SMLABB(sr, sc, samp_s);
    samp_c = __SMLABT(sr, sc, samp_c);
    ref_s = __SMLATB(sr, sc, ref_s);
    ref_c = __SMLATT(sr, sc, ref_c);
#endif
  }
  acc_samp_s = samp_s;  // Accumulate �ۼ� I·
  acc_samp_c = samp_c;  // Accumulate �ۼ� Q·
  acc_ref_s = ref_s;
  acc_ref_c = ref_c;
}

// GammaԴ��CRT(��ʾ��/���ӻ�)����Ӧ����,���������������ѹ�ķ����Թ�ϵ
//����ϵ��= sample /reference
void
calculate_gamma(float gamma[2])  // gamma ����ϵ������˼
{
  float rs = acc_ref_s;  // �ο� sin
  float rc = acc_ref_c;  // �ο� cos
  float rr = rs * rs + rc * rc;
  //rr = sqrtf(rr) * 1e8;
  float ss = acc_samp_s;  // �ź� sin
  float sc = acc_samp_c;  // �ź� cos
  // ��������
  //(a+bi) /(c+di) = ((ac+bd) + (bc-ad)i)  / (c*c + d*d)
  //                        = (ac+bd)/(c*c+d*d) + (bc-ad)i/(c*c+d*d)
  gamma[REAL_PART] =  (sc * rc + ss * rs) / rr;  // ʵ����
  gamma[IMAG_PART] =  (ss * rc - sc * rs) / rr;  // �鲿��
}

