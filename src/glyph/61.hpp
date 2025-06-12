#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code61_raw[] = {
0,2,0,66,1,102,3,98,3,98,0,3,0,7,0,0,19,33,21,33,17,33,21,33,66,3,32,252,224,3,32,252,224,1,244,142,1,252,141,};
}

inline std::string_view const code61{(char const*)detail::code61_raw, 39};
inline uint16_t constexpr code61_advanceWidth = 907;

} // namespace eglyf::res
