#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code53_raw[] = {
0,1,0,135,255,227,4,25,5,176,0,38,0,0,1,34,14,5,7,39,19,33,21,33,3,54,51,50,22,23,22,21,20,14,1,35,34,38,39,55,22,51,50,54,53,52,39,46,1,2,92,33,59,49,37,36,17,27,2,145,94,2,160,253,227,66,91,155,163,234,35,13,107,209,135,170,247,46,156,84,223,132,163,9,24,159,3,39,7,16,14,23,12,23,1,75,2,158,141,254,84,55,194,164,63,62,130,221,137,197,162,28,245,193,155,44,39,114,149,};
}

inline std::string_view const code53{(char const*)detail::code53_raw, 119};
inline uint16_t constexpr code53_advanceWidth = 1136;

} // namespace eglyf::res::tuff
