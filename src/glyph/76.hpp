#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code76_raw[] = {
0,1,0,121,0,0,4,20,5,176,0,5,0,0,19,51,17,33,21,33,121,158,2,253,252,101,5,176,250,221,141,};
}

inline std::string_view const code76{(char const*)detail::code76_raw, 31};
inline uint16_t constexpr code76_advanceWidth = 1105;

} // namespace eglyf::res::tuff
