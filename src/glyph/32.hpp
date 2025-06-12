#pragma once
// clang-format off
namespace eglyf::res {

namespace detail {
inline constexpr unsigned char code32_raw[] = {
};
}

inline std::string_view const code32{(char const*)detail::code32_raw, 0};
inline uint16_t constexpr code32_advanceWidth = 616;

} // namespace eglyf::res
