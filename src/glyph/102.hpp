#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code102_raw[] = {
0,1,0,125,0,0,2,211,5,201,0,26,0,0,1,17,35,17,35,53,51,53,52,62,2,51,50,22,23,7,46,2,35,34,6,29,1,51,21,1,160,152,139,138,59,88,96,39,58,77,43,109,14,18,40,28,40,57,195,3,170,252,86,3,170,135,135,60,106,68,39,41,41,105,18,18,16,52,45,176,135,};
}

inline std::string_view const code102{(char const*)detail::code102_raw, 81};
inline uint16_t constexpr code102_advanceWidth = 716;

} // namespace eglyf::res::tuff
