#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code85_raw[] = {
0,1,0,121,255,227,4,141,5,176,0,23,0,0,1,51,17,16,0,35,34,36,39,38,53,17,51,17,20,23,30,1,51,50,62,2,53,3,236,161,254,236,246,196,254,253,45,22,162,19,35,184,122,75,128,101,57,5,176,252,213,254,195,254,155,231,212,103,128,3,43,252,213,114,86,166,166,63,126,208,135,};
}

inline std::string_view const code85{(char const*)detail::code85_raw, 81};

} // namespace eglyf::res
