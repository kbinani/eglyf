#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code58_raw[] = {
0,2,0,113,255,243,1,72,3,163,0,15,0,31,0,0,19,52,54,51,50,22,23,22,21,20,6,35,34,38,39,38,17,52,54,51,50,22,23,22,21,20,6,35,34,38,39,38,113,64,44,39,58,8,2,63,44,39,58,8,3,64,44,39,58,8,2,63,44,39,58,8,3,3,55,45,63,49,36,8,15,44,63,48,37,10,253,51,45,63,49,36,8,15,44,63,48,37,10,};
}

inline std::string_view const code58{(char const*)detail::code58_raw, 99};
inline uint16_t constexpr code58_advanceWidth = 423;

} // namespace eglyf::res
