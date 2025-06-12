#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code69_raw[] = {
0,1,0,121,0,0,4,18,5,176,0,11,0,0,19,33,21,33,17,33,21,33,17,33,21,33,121,3,153,253,5,2,208,253,48,2,251,252,103,5,176,141,254,14,141,253,233,141,};
}

inline std::string_view const code69{(char const*)detail::code69_raw, 48};
inline uint16_t constexpr code69_advanceWidth = 1142;

} // namespace eglyf::res
