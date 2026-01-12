#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code121_raw[] = {
0,1,0,120,254,63,3,179,4,49,0,36,0,0,5,20,14,2,35,34,39,55,22,51,50,62,2,61,1,14,1,35,34,2,25,1,51,17,20,30,2,51,50,62,1,53,17,51,3,179,76,123,151,77,213,150,106,103,154,49,94,82,50,28,164,113,169,201,151,29,60,102,68,89,123,53,152,18,98,167,107,59,174,85,123,35,65,111,69,214,89,121,1,23,1,26,2,29,253,227,90,150,118,66,120,186,119,2,28,};
}

inline std::string_view const code121{(char const*)detail::code121_raw, 109};
inline uint16_t constexpr code121_advanceWidth = 1064;

} // namespace eglyf::res::tuff
