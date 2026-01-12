#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code100_raw[] = {
0,2,0,73,255,227,3,151,5,174,0,22,0,32,0,0,1,51,17,35,53,14,1,35,34,46,2,53,52,62,2,51,50,30,2,23,4,16,22,51,50,54,53,52,38,34,2,255,152,152,42,137,119,101,157,92,46,46,92,156,101,62,100,70,48,19,253,228,154,115,116,160,160,232,5,174,250,82,184,97,116,96,164,198,110,112,197,161,93,35,65,71,45,147,254,112,233,233,198,199,230,};
}

inline std::string_view const code100{(char const*)detail::code100_raw, 101};
inline uint16_t constexpr code100_advanceWidth = 1044;

} // namespace eglyf::res::tuff
