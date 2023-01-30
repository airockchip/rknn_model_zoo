/*
 * @Author: Rockchip AI Team
 * @Date: 2022-05-10 9:32:24
 * @LastEditTime: 2022-05-16 10:24:55
 * @Editors: linshuangjie
 * @Description: TODO
 */

#ifndef __RKNN_MINI_RT_DELEGATE_H_
#define __RKNN_MINI_RT_DELEGATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BF16_EXP_MAX (256 - 1)  //  2^8 - 1
#define FP16_EXP_MAX (32 - 1)   //  2^5 - 1
#define FP32_EXP_MAX (256 - 1)  //  2^8 - 1
#define FP64_EXP_MAX (2048 - 1) // 2^11 - 1

#define FP16_NAN ((FP16_EXP_MAX << 10) + 1)
#define FP16_INF ((FP16_EXP_MAX << 10) + 0)
#define BF16_NAN ((BF16_EXP_MAX << 7) + 1)
#define BF16_INF ((BF16_EXP_MAX << 7) + 0)
#define FP32_NAN ((FP32_EXP_MAX << 23) + 1)
#define FP32_INF ((FP32_EXP_MAX << 23) + 0)

#ifdef __GNUC__
#define PACKAGE_MARK __attribute__((packed))
#else
#define PACKAGE_MARK
#endif

struct fp16_pack
{
  unsigned short frac : 10;
  unsigned short  exp : 5;
  unsigned short  sign : 1;
} PACKAGE_MARK;

struct fp32_pack
{
  unsigned int  frac : 23;
  unsigned int exp : 8;
  unsigned int sign : 1;
} PACKAGE_MARK;

typedef struct fp16_pack float16;

static inline float fp16_to_fp32(float16 data)
{
  float             f;
  struct fp32_pack* fp32 = (struct fp32_pack*)&f;
  struct fp16_pack* fp16 = &data;

  int exp = fp16->exp;

  if (exp == 31 && fp16->frac != 0) {
    // return __builtin_inf()-__builtin_inf();
    fp32->sign = fp16->sign;
    fp32->exp  = 255;
    fp32->frac = 1;

    return f;
  }

  if (exp == 31)
    exp = 255;
  if (exp == 0)
    exp = 0;
  else
    exp = (exp - 15) + 127;

  fp32->exp  = exp;
  fp32->sign = fp16->sign;
  fp32->frac = ((int)fp16->frac) << 13;

  return f;
}

static inline float16 fp32_to_fp16(float data)
{
  struct fp32_pack* fp32 = (struct fp32_pack*)&data;
  struct fp16_pack  fp16;

  int exp = fp32->exp;

  if (fp32->exp == 255 && fp32->frac != 0) {
    // NaN
    fp16.exp  = 31;
    fp16.frac = 1;
    fp16.sign = fp32->sign;

    return fp16;
  }

  if ((exp - 127) < -14)
    exp = 0;
  else if ((exp - 127) > 15)
    exp = 31;
  else
    exp = exp - 127 + 15;

  fp16.exp  = exp;
  fp16.frac = fp32->frac >> 13;
  fp16.sign = fp32->sign;

  return fp16;
}

#ifdef __cplusplus
}
#endif
#endif
