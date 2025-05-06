#pragma once

namespace eglyf {

enum class Endian {
  Big,
  Little,
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
  Native = Big,
#else
  Native = Little,
#endif
};

template <std::unsigned_integral T>
constexpr T BigEndian(T v) {
#if defined(_MSC_VER)
  if constexpr (sizeof(T) == 2 && Endian::Native != Endian::Big) {
    return _byteswap_ushort(v);
  }
  if constexpr (sizeof(T) == 4 && Endian::Native != Endian::Big) {
    return _byteswap_ulong(v);
  }
  if constexpr (sizeof(T) == 8 && Endian::Native != Endian::Big) {
    return _byteswap_uint64(v);
  }
#elif defined(__clang__) || defined(__GNUC__)
  if constexpr (sizeof(T) == 2 && Endian::Native != Endian::Big) {
    return __builtin_bswap16(v);
  }
  if constexpr (sizeof(T) == 4 && Endian::Native != Endian::Big) {
    return __builtin_bswap32(v);
  }
  if constexpr (sizeof(T) == 8 && Endian::Native != Endian::Big) {
    return __builtin_bswap64(v);
  }
#else
  if constexpr (sizeof(T) == 2 && Endian::Native != Endian::Big) {
    return static_cast<T>((v >> 8) | (v << 8));
  }
  if constexpr (sizeof(T) == 4 && Endian::Native != Endian::Big) {
    return static_cast<T>((v >> 24) |
                          ((v >> 8) & 0x0000'FF00u) |
                          ((v << 8) & 0x00FF'0000u) |
                          (v << 24));
  }
  if constexpr (sizeof(T) == 8 && Endian::Native != Endian::Big) {
    return static_cast<T>((v >> 56) |
                          ((v >> 40) & 0x0000'0000'FF00'00FFull) |
                          ((v >> 24) & 0x0000'00FF'0000'FF00ull) |
                          ((v >> 8) & 0x0000'FF00'0000'FF00ull) |
                          ((v << 8) & 0x00FF'0000'FF00'0000ull) |
                          ((v << 24) & 0xFF00'0000'FF00'0000ull) |
                          (v << 56));
  }
#endif
  return v;
}

} // namespace eglyf
