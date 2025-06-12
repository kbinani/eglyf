#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code41_raw[] = {
0,1,0,90,255,172,1,250,6,14,0,13,0,0,5,35,54,18,53,52,2,39,51,22,18,21,20,2,1,10,176,127,137,148,116,176,106,134,121,84,187,1,169,199,208,1,189,170,166,254,74,219,210,254,93,};
}

inline std::string_view const code41{(char const*)detail::code41_raw, 55};
inline uint16_t constexpr code41_advanceWidth = 536;

} // namespace eglyf::res
