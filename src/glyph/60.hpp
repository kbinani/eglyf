#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code60_raw[] = {
0,1,0,84,0,23,3,23,4,211,0,5,0,0,37,9,1,51,9,1,2,80,254,4,1,252,199,254,2,1,254,23,2,100,2,88,253,166,253,158,};
}

inline std::string_view const code60{(char const*)detail::code60_raw, 40};

} // namespace eglyf::res
