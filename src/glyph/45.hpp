#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code45_raw[] = {
0,1,0,109,2,51,3,18,2,193,0,3,0,0,19,33,21,33,109,2,165,253,91,2,193,142,};
}

inline std::string_view const code45{(char const*)detail::code45_raw, 26};
inline uint16_t constexpr code45_advanceWidth = 866;

} // namespace eglyf::res
