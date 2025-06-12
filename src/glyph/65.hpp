#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code65_raw[] = {
0,2,0,72,0,0,4,223,5,176,0,2,0,11,0,0,1,3,33,23,33,3,35,1,48,51,1,35,2,147,204,1,159,37,254,15,162,176,2,12,127,2,12,180,4,190,253,175,136,254,27,5,176,250,80,};
}

inline std::string_view const code65{(char const*)detail::code65_raw, 55};
inline uint16_t constexpr code65_advanceWidth = 1302;

} // namespace eglyf::res
