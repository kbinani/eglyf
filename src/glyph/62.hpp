#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code62_raw[] = {
0,1,0,92,0,20,3,29,4,211,0,5,0,0,37,35,9,1,51,1,1,35,199,1,252,254,4,199,1,250,20,2,101,2,90,253,168,};
}

inline std::string_view const code62{(char const*)detail::code62_raw, 37};
inline uint16_t constexpr code62_advanceWidth = 856;

} // namespace eglyf::res::tuff
