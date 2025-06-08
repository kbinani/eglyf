#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code124_raw[] = {
0,1,0,125,0,0,1,12,5,174,0,3,0,0,51,17,51,17,125,143,5,174,250,82,};
}

inline std::string_view const code124{(char const*)detail::code124_raw, 24};

} // namespace eglyf::res
