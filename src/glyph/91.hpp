#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code91_raw[] = {
0,1,0,131,255,160,2,14,5,250,0,7,0,0,5,33,17,33,21,35,17,51,2,14,254,117,1,139,245,245,96,6,90,141,250,190,};
}

inline std::string_view const code91{(char const*)detail::code91_raw, 36};

} // namespace eglyf::res
