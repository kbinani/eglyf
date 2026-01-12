#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code82_raw[] = {
0,2,0,121,0,0,4,80,5,176,0,11,0,26,0,0,1,51,50,62,3,53,52,38,43,1,9,1,35,17,35,17,59,1,50,4,21,20,6,7,1,1,23,206,94,142,82,50,17,186,199,206,2,132,254,83,215,158,158,206,242,1,51,198,166,1,178,3,6,34,53,76,70,40,124,144,250,221,2,119,253,137,5,176,217,192,149,220,30,253,120,};
}

inline std::string_view const code82{(char const*)detail::code82_raw, 91};
inline uint16_t constexpr code82_advanceWidth = 1101;

} // namespace eglyf::res::tuff
