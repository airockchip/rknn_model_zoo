#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>
using namespace std;

float decode_float16(uint16_t float16_value)
{
  // MSB -> LSB
  // float16=1bit: sign, 5bit: exponent, 10bit: fraction
  // float32=1bit: sign, 8bit: exponent, 23bit: fraction
  // for normal exponent(1 to 0x1e): value=2**(exponent-15)*(1.fraction)
  // for denormalized exponent(0): value=2**-14*(0.fraction)
  uint32_t sign = float16_value >> 15;
  uint32_t exponent = (float16_value >> 10) & 0x1F;
  uint32_t fraction = (float16_value & 0x3FF);
  uint32_t float32_value;
  if (exponent == 0)
  {
    if (fraction == 0)
    {
      // zero
      float32_value = (sign << 31);
    }
    else
    {
      // can be represented as ordinary value in float32
      // 2 ** -14 * 0.0101
      // => 2 ** -16 * 1.0100
      // int int_exponent = -14;
      exponent = 127 - 14;
      while ((fraction & (1 << 10)) == 0)
      {
        //int_exponent--;
        exponent--;
        fraction <<= 1;
      }
      fraction &= 0x3FF;
      // int_exponent += 127;
      float32_value = (sign << 31) | (exponent << 23) | (fraction << 13);  
    }    
  }
  else if (exponent == 0x1F)
  {
    /* Inf or NaN */
    float32_value = (sign << 31) | (0xFF << 23) | (fraction << 13);
  }
  else
  {
    /* ordinary number */
    float32_value = (sign << 31) | ((exponent + (127-15)) << 23) | (fraction << 13);
  }
  
  return *((float*)&float32_value);
}

float decode_u8(uint8_t uint8_value, float scale, int zp)
{
    return (float)((int)uint8_value - zp) * scale;
}


float decode_i8(int8_t int8_value, float scale)
{
    return (float)int8_value * scale;
}

float decode_i8(int8_t int8_value, int fl)
{
    float scale = pow(2, -fl);
    return decode_i8(int8_value, scale);
}

float decode_i16(int16_t int16_value, float scale)
{
    return (float)int16_value * scale;
}

float decode_i16(int16_t int16_value, int fl)
{
    float scale = pow(2, -fl);
    return decode_i16(int16_value, scale);
}