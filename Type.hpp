#pragma once

namespace ksesh::otf {

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
  uint16_t data = 0;
};

template <class T>
struct Vec {
  T x;
  T y;

  Vec() : x(T()), y(T()) {}
  Vec(T x, T y) : x(x), y(y) {}
};

} // namespace ksesh::otf
