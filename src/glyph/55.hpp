#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code55_raw[] = {
0,1,0,111,0,0,4,55,5,176,0,5,0,0,9,1,35,1,33,53,4,55,253,213,182,2,4,253,21,5,176,250,80,5,27,149,};
}

inline std::string_view const code55{(char const*)detail::code55_raw, 36};
inline uint16_t constexpr code55_advanceWidth = 1136;

} // namespace eglyf::res
