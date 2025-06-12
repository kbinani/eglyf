#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code72_raw[] = {
0,1,0,121,0,0,4,74,5,176,0,11,0,0,19,51,17,33,17,51,17,35,17,33,17,35,121,158,2,149,158,158,253,107,158,5,176,253,127,2,129,250,80,2,162,253,94,};
}

inline std::string_view const code72{(char const*)detail::code72_raw, 47};
inline uint16_t constexpr code72_advanceWidth = 1198;

} // namespace eglyf::res
