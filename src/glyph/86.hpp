#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code86_raw[] = {
0,1,0,80,0,0,4,162,5,176,0,6,0,0,1,51,1,35,1,51,1,3,254,164,254,71,219,254,66,170,1,129,5,176,250,80,5,176,250,248,};
}

inline std::string_view const code86{(char const*)detail::code86_raw, 40};

} // namespace eglyf::res
