#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code35_raw[] = {
0,2,0,129,0,53,5,199,5,123,0,3,0,31,0,0,1,17,33,17,5,17,35,17,33,17,35,17,33,53,33,17,33,53,33,17,51,17,33,17,51,17,33,21,33,17,33,21,3,201,254,170,1,235,149,254,170,150,254,164,1,92,254,164,1,92,150,1,86,149,1,105,254,151,1,105,2,35,1,82,254,174,141,254,159,1,97,254,159,1,97,141,1,82,139,1,123,254,133,1,123,254,133,139,254,174,141,};
}

inline std::string_view const code35{(char const*)detail::code35_raw, 107};
inline uint16_t constexpr code35_advanceWidth = 1589;

} // namespace eglyf::res
