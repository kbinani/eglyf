#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code73_raw[] = {
0,1,0,154,0,0,1,55,5,176,0,3,0,0,51,17,51,17,154,157,5,176,250,80,};
}

inline std::string_view const code73{(char const*)detail::code73_raw, 24};
inline uint16_t constexpr code73_advanceWidth = 444;

} // namespace eglyf::res
