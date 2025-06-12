#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code101_raw[] = {
0,2,0,98,255,227,3,157,4,70,0,7,0,34,0,0,1,33,46,1,35,34,14,1,7,30,2,51,50,54,55,23,14,1,35,34,46,1,16,62,1,51,50,30,2,21,20,22,21,1,1,2,10,9,140,112,76,119,60,6,8,60,117,76,72,141,32,133,51,209,118,130,193,97,98,192,130,97,158,99,52,1,2,92,145,209,108,157,224,96,160,107,77,60,55,99,118,153,254,1,62,250,148,91,151,186,101,11,73,12,};
}

inline std::string_view const code101{(char const*)detail::code101_raw, 110};
inline uint16_t constexpr code101_advanceWidth = 1013;

} // namespace eglyf::res
