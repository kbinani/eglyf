#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code84_raw[] = {
0,1,0,88,0,0,4,156,5,176,0,7,0,0,19,33,21,33,17,35,17,33,88,4,68,254,55,158,254,35,5,176,141,250,221,5,35,};
}

inline std::string_view const code84{(char const*)detail::code84_raw, 37};
inline uint16_t constexpr code84_advanceWidth = 1243;

} // namespace eglyf::res::tuff
