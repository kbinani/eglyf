#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code90_raw[] = {
0,1,0,100,0,0,4,221,5,176,0,7,0,0,19,33,1,33,21,33,1,33,129,4,80,252,160,3,108,251,135,3,105,252,180,5,176,250,221,141,5,35,};
}

inline std::string_view const code90{(char const*)detail::code90_raw, 42};
inline uint16_t constexpr code90_advanceWidth = 1325;

} // namespace eglyf::res::tuff
