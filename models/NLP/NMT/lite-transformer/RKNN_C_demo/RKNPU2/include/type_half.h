typedef unsigned short half;
typedef unsigned short ushort;


uint as_uint(const float x) {
    return *(uint*)&x;
}
float as_float(const uint x) {
    return *(float*)&x;
}



float half_to_float(half x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    // printf("1\n");
    const uint e = (x&0x7C00)>>10; // exponent
    // printf("2\n");
    const uint m = (x&0x03FF)<<13; // mantissa
    // printf("3\n");
    const uint v = as_uint((float)m)>>23; // evil log2 bit hack to count leading zeros in denormalized format
    // printf("4\n");
    return as_float((x&0x8000)<<16 | (e!=0)*((e+112)<<23|m) | ((e==0)&(m!=0))*((v-37)<<23|((m<<(150-v))&0x007FE000))); // sign : normalized : denormalized
}
// half float_to_half(float x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
//     const uint b = as_uint(x)+0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
//     const uint e = (b&0x7F800000)>>23; // exponent
//     const uint m = b&0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
//     return (b&0x80000000)>>16 | (e>112)*((((e-112)<<10)&0x7C00)|m>>13) | ((e<113)&(e>101))*((((0x007FF000+m)>>(125-e))+1)>>1) | (e>143)*0x7FFF; // sign : normalized : denormalized : saturate
// }


typedef union suf32
{
  int      i;
  unsigned u;
  float    f;
} suf32;

half float_to_half(float x)
  {
    suf32 in;
    in.f          = x;
    unsigned sign = in.u & 0x80000000;
    in.u ^= sign;
    ushort w;

    if (in.u >= 0x47800000)
      w = (ushort)(in.u > 0x7f800000 ? 0x7e00 : 0x7c00);
    else {
      if (in.u < 0x38800000) {
        in.f += 0.5f;
        w = (ushort)(in.u - 0x3f000000);
      } else {
        unsigned t = in.u + 0xc8000fff;
        w          = (ushort)((t + ((in.u >> 13) & 1)) >> 13);
      }
    }

    w = (ushort)(w | (sign >> 16));

    return w;
}


// uint16_t float_to_half(float value)
// {
//     uint32_t f = *((uint32_t*)&value);
//     uint16_t h = (f >> 16) & 0x8000;  // 处理符号位
//     int32_t e = (f >> 23) & 0xFF;    // 处理指数位
//     int32_t m = f & 0x7FFFFF;        // 处理尾数位

//     if (e == 0xFF) // 处理特殊情况 NaN or Infinity
//     {
//         h |= (m ? 0x7FFF : 0x7C00);
//     }
//     else if (e > 0x71) // 处理溢出情况
//     {
//         h |= 0x7C00;
//     }
//     else if (e < 0x8) // 处理下溢情况
//     {
//         h |= (m >> 13) | ((e == 0 && (m & 0x1000)) ? 1 : 0);
//     }
//     else // 处理正常情况
//     {
//         e += (127 - 15);
//         m >>= 13;

//         if (m & 0x1) // 进位处理
//             m += 1;

//         if (m & 0x8000) // 舍去处理
//         {
//             m = 0;
//             e += 1;
//         }

//         if (e > 30) // 处理溢出情况
//         {
//             h |= 0x7C00;
//         }
//         else
//         {
//             h |= (e << 10) | (m & 0x3FF);
//         }
//     }

//     return h;
// }

// half float_to_half(float in) {
//     uint32_t inu = *((uint32_t * ) & in);
//     uint32_t t1;
//     uint32_t t2;
//     uint32_t t3;

//     t1 = inu & 0x7fffffffu;                 // Non-sign bits
//     t2 = inu & 0x80000000u;                 // Sign bit
//     t3 = inu & 0x7f800000u;                 // Exponent

//     t1 >>= 13u;                             // Align mantissa on MSB
//     t2 >>= 16u;                             // Shift sign bit into position

//     t1 -= 0x1c000;                         // Adjust bias

//     t1 = (t3 < 0x38800000u) ? 0 : t1;       // Flush-to-zero
//     t1 = (t3 > 0x8e000000u) ? 0x7bff : t1;  // Clamp-to-max
//     t1 = (t3 == 0 ? 0 : t1);               // Denormals-as-zero

//     t1 |= t2;                              // Re-insert sign bit

//     return (half)t1;
// };


// float half_to_float(half in) {
//     uint32_t t1;
//     uint32_t t2;
//     uint32_t t3;

//     t1 = in & 0x7fffu;                       // Non-sign bits
//     t2 = in & 0x8000u;                       // Sign bit
//     t3 = in & 0x7c00u;                       // Exponent

//     t1 <<= 13u;                              // Align mantissa on MSB
//     t2 <<= 16u;                              // Shift sign bit into position

//     t1 += 0x38000000;                       // Adjust bias

//     t1 = (t3 == 0 ? 0 : t1);                // Denormals-as-zero

//     t1 |= t2;                               // Re-insert sign bit

//     return *((float*)((void*)&t1));
//     // return 1.1;
// };

// float half_to_float(half h) {
//     unsigned int sign = (h >> 15) & 0x0001;
//     unsigned int exponent = (h >> 10) & 0x001f;
//     unsigned int mantissa = h & 0x03ff;
//     unsigned int f;

//     if (exponent == 0) {
//         f = (mantissa << 13) | (sign << 31);
//     } else if (exponent == 31) {
//         f = ((mantissa != 0) ? 0x7f800000 : 0x7f800000) | (sign << 31);
//     } else {
//         int new_exp = exponent - 15 + 127;
//         f = (new_exp << 23) | (mantissa << 13) | (sign << 31);
//     }

//     return *(float *)&f;
// }

void float_to_half_array(float *src, half *dst, int size)
{
    for (int i = 0; i < size; i++)
    {
        dst[i] = float_to_half(src[i]);
    }
    // printf("finish float_to_half_array\n");
    // for (int i = 0; i < 10; i++){
    //         float temp = half_to_float(dst[i]);
    //         float temp1 = src[i];
    //         // float temp = 1.0;
    //         // printf("%f\n", temp);
    //         printf("float[%d] = %f, fp16[%d] = %f\n", i, temp1, i, temp);        
    // }
}

void half_to_float_array(half *src, float *dst, int size)
{
    for (int i = 0; i < size; i++)
    {
        dst[i] = half_to_float(src[i]);
    }
}