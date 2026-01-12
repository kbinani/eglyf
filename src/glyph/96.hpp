#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code96_raw[] = {
0,1,0,60,6,46,1,224,7,152,0,25,0,0,1,22,23,22,21,20,7,6,7,6,35,34,39,37,38,39,38,53,52,55,54,55,54,51,50,23,1,208,11,3,2,13,12,20,10,4,14,12,254,207,21,6,3,17,16,24,6,12,26,22,6,140,11,12,8,6,17,18,15,5,2,9,219,18,23,15,2,26,20,22,6,2,18,};
}

inline std::string_view const code96{(char const*)detail::code96_raw, 87};
inline uint16_t constexpr code96_advanceWidth = 550;

} // namespace eglyf::res::tuff
