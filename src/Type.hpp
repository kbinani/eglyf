#pragma once

namespace eglyf {

using Offset16 = uint16_t;
using Offset32 = uint32_t;

struct Fixed {
  uint32_t value;
};

using LONGDATETIME = int64_t;
using FWORD = int16_t;
using UFWORD = uint16_t;

struct Version16Dot16 {
  uint16_t major;
  uint16_t minor;
};

static_assert(sizeof(Version16Dot16) == sizeof(uint32_t));

struct F2DOT14 {
  static constexpr float ToFloatScale = 1.0f / (1 << 14);
  static constexpr float FromFloatScale = 1 << 14;

  float toFloat() const {
    return static_cast<int32_t>(data) * ToFloatScale;
  }

  static F2DOT14 FromFloat(float v) {
    F2DOT14 r;
    r.data = static_cast<int16_t>(roundf(v * FromFloatScale));
    return r;
  }

  int16_t data = 0;
};

template <class T>
struct Vec {
  T x;
  T y;

  Vec() : x(T()), y(T()) {}
  Vec(T x, T y) : x(x), y(y) {}
};

} // namespace eglyf
