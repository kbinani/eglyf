#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code33_raw[] = {
0,2,0,103,255,243,1,66,5,203,0,13,0,29,0,0,1,20,6,35,46,1,53,3,52,54,51,50,22,21,3,52,54,51,50,22,23,22,21,20,6,35,34,38,39,38,1,25,35,23,25,34,41,59,39,40,61,219,64,44,39,58,8,2,63,44,39,58,8,3,1,184,25,34,1,33,25,3,181,39,55,55,37,250,239,45,63,49,36,8,15,44,63,48,37,10,};
}

inline std::string_view const code33{(char const*)detail::code33_raw, 96};
inline uint16_t constexpr code33_advanceWidth = 421;

} // namespace eglyf::res
