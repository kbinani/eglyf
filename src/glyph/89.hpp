#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code89_raw[] = {
0,1,0,76,0,0,4,100,5,176,0,8,0,0,19,51,9,1,51,1,17,35,17,76,168,1,100,1,100,168,254,66,158,5,176,253,168,2,88,253,60,253,20,2,236,};
}

inline std::string_view const code89{(char const*)detail::code89_raw, 45};

} // namespace eglyf::res
