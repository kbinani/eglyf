#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code39_raw[] = {
0,1,0,129,4,20,1,23,6,6,0,18,0,0,19,3,52,54,59,1,50,22,23,22,21,3,14,1,35,34,38,39,38,150,21,43,29,4,25,41,6,2,21,1,34,20,19,29,4,1,4,68,1,118,36,40,29,27,12,8,254,138,22,26,20,19,3,};
}

inline std::string_view const code39{(char const*)detail::code39_raw, 67};
inline uint16_t constexpr code39_advanceWidth = 382;

} // namespace eglyf::res::tuff
