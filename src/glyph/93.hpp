#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code93_raw[] = {
0,1,0,129,255,160,2,12,5,250,0,7,0,0,23,53,51,17,35,53,33,17,129,246,246,1,139,96,141,5,64,141,249,166,};
}

inline std::string_view const code93{(char const*)detail::code93_raw, 34};
inline uint16_t constexpr code93_advanceWidth = 638;

} // namespace eglyf::res
