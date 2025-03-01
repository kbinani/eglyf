#pragma once

namespace eglyf {

class Coverage1 {
public:
  static std::optional<Coverage1> Read(InputStream &in) {
    using namespace std;
    Coverage1 r;
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return nullopt;
    }
    r.glyphArray.reserve(glyphCount);
    for (uint16_t i = 0; i < glyphCount; i++) {
      uint16_t v;
      if (!in.u16(&v)) {
        return nullopt;
      }
      r.glyphArray.push_back(v);
    }
    return r;
  }

public:
  std::vector<uint16_t> glyphArray;
};

} // namespace eglyf
