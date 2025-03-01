#pragma once

namespace ksesh {

struct Tag {
  std::array<uint8_t, 4> values;

  static std::optional<Tag> Read(InputStream &in) {
    Tag t;
    for (int i = 0; i < t.values.size(); i++) {
      t.values[i] = in.u8();
    }
    if (in.ok()) {
      return t;
    } else {
      return std::nullopt;
    }
  }

  static constexpr std::array<uint8_t, 4> FCC(char a, char b, char c, char d) {
    std::array<uint8_t, 4> v;
    v[0] = *(uint8_t *)&a;
    v[1] = *(uint8_t *)&b;
    v[2] = *(uint8_t *)&c;
    v[3] = *(uint8_t *)&d;
    return v;
  }
};

} // namespace ksesh
