#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code118_raw[] = {
0,1,0,82,0,0,3,199,4,49,0,6,0,0,33,1,51,9,1,51,1,1,190,254,148,160,1,24,1,31,158,254,143,4,49,252,146,3,110,251,207,};
}

inline std::string_view const code118{(char const*)detail::code118_raw, 41};
inline uint16_t constexpr code118_advanceWidth = 1024;

} // namespace eglyf::res
