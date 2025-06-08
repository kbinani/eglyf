#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code94_raw[] = {
0,1,0,133,4,125,3,53,5,213,0,6,0,0,1,35,39,7,35,1,51,3,53,188,156,158,186,1,21,131,4,125,213,211,1,86,};
}

inline std::string_view const code94{(char const*)detail::code94_raw, 36};

} // namespace eglyf::res
