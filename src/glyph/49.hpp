#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code49_raw[] = {
0,1,1,168,0,0,2,227,5,176,0,5,0,0,1,35,55,51,17,35,2,70,158,223,92,157,4,182,250,250,80,};
}

inline std::string_view const code49{(char const*)detail::code49_raw, 31};
inline uint16_t constexpr code49_advanceWidth = 1136;

} // namespace eglyf::res
