#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code110_raw[] = {
0,1,0,129,0,0,3,188,4,78,0,22,0,0,33,35,17,51,21,62,1,51,50,18,25,1,35,17,52,46,2,35,34,14,1,21,1,25,152,152,28,164,113,169,201,151,29,60,102,68,89,123,53,4,49,181,89,121,254,233,254,230,253,227,2,29,90,150,118,66,120,186,119,};
}

inline std::string_view const code110{(char const*)detail::code110_raw, 73};
inline uint16_t constexpr code110_advanceWidth = 1064;

} // namespace eglyf::res
