#pragma once

namespace ksesh {

struct Tag {
  std::array<uint8_t, 4> values;

  static std::optional<Tag> Read(InputStream &in) {
    using namespace std;
    Tag t;
    for (int i = 0; i < t.values.size(); i++) {
      if (!in.u8(t.values.data() + i)) {
        return nullopt;
      }
    }
    return t;
  }

  static std::array<uint8_t, 4> FCC(char const fcc[5]) {
    std::array<uint8_t, 4> v;
    v[0] = *(uint8_t *)&fcc[0];
    v[1] = *(uint8_t *)&fcc[1];
    v[2] = *(uint8_t *)&fcc[2];
    v[3] = *(uint8_t *)&fcc[3];
    return v;
  }
};

} // namespace ksesh
