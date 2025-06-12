#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code78_raw[] = {
0,1,0,121,0,0,4,147,5,176,0,9,0,0,51,17,51,1,17,51,17,35,1,17,121,153,2,228,157,145,253,21,5,176,251,119,4,137,250,80,4,166,251,90,};
}

inline std::string_view const code78{(char const*)detail::code78_raw, 44};
inline uint16_t constexpr code78_advanceWidth = 1271;

} // namespace eglyf::res
