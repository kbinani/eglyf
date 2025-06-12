#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code80_raw[] = {
0,2,0,121,0,0,4,8,5,176,0,11,0,27,0,0,1,51,32,17,52,39,46,3,43,1,39,59,1,50,4,23,22,21,20,14,2,43,1,17,35,1,23,206,1,129,6,9,44,83,145,98,206,158,158,206,230,1,21,32,8,66,130,213,138,206,158,3,6,1,17,35,25,41,69,63,35,141,176,148,45,40,87,148,115,66,253,137,};
}

inline std::string_view const code80{(char const*)detail::code80_raw, 88};
inline uint16_t constexpr code80_advanceWidth = 1099;

} // namespace eglyf::res
