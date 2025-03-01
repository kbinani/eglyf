#pragma once

namespace eglyf {

using Tag = std::array<uint8_t, 4>;

inline std::optional<Tag> ReadTag(InputStream &in) {
  Tag t;
  for (int i = 0; i < t.size(); i++) {
    if (!in.u8(t.data() + i)) {
      return std::nullopt;
    }
  }
  return t;
}

inline Tag FCC(char const fcc[5]) {
  std::array<uint8_t, 4> v;
  v[0] = *(uint8_t *)&fcc[0];
  v[1] = *(uint8_t *)&fcc[1];
  v[2] = *(uint8_t *)&fcc[2];
  v[3] = *(uint8_t *)&fcc[3];
  return v;
}

} // namespace eglyf
