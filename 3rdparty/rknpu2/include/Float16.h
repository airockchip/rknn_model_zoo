#ifndef _RKNPU2_RKNN_MATMUL_API_DEMO_H_
#define _RKNPU2_RKNN_MATMUL_API_DEMO_H_

namespace rknpu2 {

using ushort = unsigned short;

typedef union suf32
{
  int      i;
  unsigned u;
  float    f;
} suf32;

class float16
{
public:
  float16() {}
  explicit float16(float x) { w = bits(x); }

  operator float() const
  {
    suf32 out;

    unsigned t    = ((w & 0x7fff) << 13) + 0x38000000;
    unsigned sign = (w & 0x8000) << 16;
    unsigned e    = w & 0x7c00;

    out.u = t + (1 << 23);
    out.u = (e >= 0x7c00 ? t + 0x38000000 : e == 0 ? (static_cast<void>(out.f -= 6.103515625e-05f), out.u) : t) | sign;
    return out.f;
  }

  static float16 fromBits(ushort b)
  {
    float16 result;
    result.w = b;
    return result;
  }
  static float16 zero()
  {
    float16 result;
    result.w = (ushort)0;
    return result;
  }
  ushort bits() const { return w; }

  static ushort bits(float x)
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

  float16& operator=(float x)
  {
    w = bits(x);
    return *this;
  }

  float16& operator+=(float x)
  {
    w = bits(float() + x);
    return *this;
  }

  float16& operator/(float x)
  {
    w = bits(float() / x);
    return *this;
  }

  inline bool is_nan() const { return ((w & 0x7c00u) == 0x7c00u) && ((w & 0x03ffu) != 0x0000u); }

  inline bool greater(const float16& x) const
  {
    bool sign   = w & 0x8000;
    bool sign_x = x.w & 0x8000;
    if (sign) {
      if (sign_x)
        return w < x.w;
      else
        return false;
    } else {
      if (sign_x)
        /* Signed zeros are equal, have to check for it */
        return (w != 0 || x.w != 0x8000);
      else
        return w > x.w;
    }
    return false;
  }

  inline bool less(const float16& x) const
  {
    bool sign   = w & 0x8000;
    bool sign_x = x.w & 0x8000;
    if (sign) {
      if (sign_x)
        return w > x.w;
      else
        /* Signed zeros are equal, have to check for it */
        return (w != 0x8000 || x.w != 0);
    } else {
      if (sign_x)
        return false;
      else
        return w < x.w;
    }
    return false;
  }

  inline bool operator>(const float16& x) const
  {
    if (is_nan() || x.is_nan()) {
      return false;
    }
    return greater(x);
  }

  inline bool operator<(const float16& x) const
  {
    if (is_nan() || x.is_nan()) {
      return false;
    }
    return less(x);
  }

  inline bool operator>=(const float16& x) const
  {
    if (is_nan() || x.is_nan()) {
      return false;
    }
    return !less(x);
  }

  inline bool operator<=(const float16& x) const
  {
    if (is_nan() || x.is_nan()) {
      return false;
    }
    return !greater(x);
  }

  inline bool operator==(const float16& x) const
  {
    /*
     * The equality cases are as follows:
     *   - If either value is NaN, never equal.
     *   - If the values are equal, equal.
     *   - If the values are both signed zeros, equal.
     */
    if (is_nan() || x.is_nan()) {
      return false;
    }
    return (w == x.w || ((w | x.w) & 0x7fff) == 0);
  }

  inline bool operator!=(const float16& x) const { return !((*this) == x); }

protected:
  ushort w = 0;
};

} // namespace rknn

#endif /* _RKNPU2_RKNN_MATMUL_API_DEMO_H_ */