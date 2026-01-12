#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code87_raw[] = {
0,1,0,84,0,0,5,236,5,176,0,12,0,0,19,51,19,1,51,1,19,51,3,35,9,1,35,84,160,184,1,18,195,1,18,185,160,230,219,254,246,254,246,224,5,176,251,6,4,213,251,43,4,250,250,80,4,205,251,51,};
}

inline std::string_view const code87{(char const*)detail::code87_raw, 60};
inline uint16_t constexpr code87_advanceWidth = 1576;

} // namespace eglyf::res::tuff
