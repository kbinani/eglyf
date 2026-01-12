#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code40_raw[] = {
0,1,0,41,255,172,1,201,6,14,0,12,0,0,5,38,2,53,52,18,55,51,0,17,20,18,23,1,23,110,128,123,115,178,254,248,147,117,84,169,1,168,230,200,1,175,180,254,130,254,73,216,254,85,170,};
}

inline std::string_view const code40{(char const*)detail::code40_raw, 55};
inline uint16_t constexpr code40_advanceWidth = 548;

} // namespace eglyf::res::tuff
