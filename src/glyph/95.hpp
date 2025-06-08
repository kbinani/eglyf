#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code95_raw[] = {
0,1,0,88,255,25,4,154,255,166,0,3,0,0,23,53,33,21,88,4,66,231,141,141,};
}

inline std::string_view const code95{(char const*)detail::code95_raw, 24};

} // namespace eglyf::res
