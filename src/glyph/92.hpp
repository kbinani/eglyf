#pragma once
// clang-format off
namespace eglyf::res::tuffy {

namespace detail {
inline constexpr unsigned char code92_raw[] = {
0,1,255,227,0,0,3,92,6,8,0,3,0,0,33,1,51,1,2,178,253,49,172,2,205,6,8,249,248,};
}

inline std::string_view const code92{(char const*)detail::code92_raw, 29};
inline uint16_t constexpr code92_advanceWidth = 811;

} // namespace eglyf::res::tuff
