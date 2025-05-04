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

using uint24 = std::array<uint8_t, 3>;

inline std::optional<uint24> UInt24FromUInt32(uint32_t v) {
  using namespace std;
  if (v > 0xffffff) {
    return nullopt;
  }
  array<uint8_t, 3> ret;
  ret[0] = (uint8_t)(0xff & (v >> 16));
  ret[1] = (uint8_t)(0xff & (v >> 8));
  ret[2] = (uint8_t)(0xff & v);
  return ret;
}

inline uint24 UInt24FromUInt16(uint16_t v) {
  using namespace std;
  array<uint8_t, 3> ret;
  ret[0] = 0;
  ret[1] = (uint8_t)(0xff & (v >> 8));
  ret[2] = (uint8_t)(0xff & v);
  return ret;
}

struct Version16Dot16 {
  uint16_t major;
  uint16_t minor;
};

static_assert(sizeof(Version16Dot16) == sizeof(uint32_t));

namespace impl {

template <size_t Bits>
constexpr auto _IntAtLeast() {
  if constexpr (Bits <= 8) {
    return int8_t{};
  } else if constexpr (Bits <= 16) {
    return int16_t{};
  } else if constexpr (Bits <= 32) {
    return int32_t{};
  } else if constexpr (Bits <= 64) {
    return int64_t{};
  } else {
    static_assert(false);
  }
}

template <size_t Bits>
using IntAtLeast = decltype(_IntAtLeast<Bits>());

template <std::integral Int, size_t Bits, std::floating_point Float>
struct FixedFloat {
private:
  using Support = IntAtLeast<sizeof(Int) * 8 + 1>;
  static constexpr Float ToFloatScale = (Float)1 / (1 << Bits);
  static constexpr Float FromFloatScale = 1 << Bits;

  static_assert(sizeof(Support) > sizeof(Int));
  static_assert(std::is_signed_v<Int>);
  static_assert(std::is_signed_v<Support>);
  static_assert(sizeof(Int) * 8 > Bits);

public:
  Float toFloat() const {
    return static_cast<Support>(data) * ToFloatScale;
  }

  static FixedFloat<Int, Bits, Float> FromFloat(Float v) {
    FixedFloat<Int, Bits, Float> r;
    r.data = static_cast<Int>(std::round(v * FromFloatScale));
    return r;
  }

  Int data = 0;
};

} // namespace impl

using F2DOT14 = impl::FixedFloat<int16_t, 14, float>;

using WxH = int;

inline int WidthFromWxH(WxH v) {
  return v / 10;
}

inline int HeightFromWxH(WxH v) {
  return v % 10;
}

inline double AspectRatioFromWxH(WxH v) {
  double w = WidthFromWxH(v);
  double h = HeightFromWxH(v);
  return w / h;
}

inline int AreaFromWxH(WxH v) {
  int w = WidthFromWxH(v);
  int h = HeightFromWxH(v);
  return w * h;
}

} // namespace eglyf
