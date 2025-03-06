#pragma once

namespace eglyf {

class Coverage1 : public Coverage {
public:
  static std::shared_ptr<Coverage1> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<Coverage1>();
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return nullptr;
    }
    r->glyphArray.reserve(glyphCount);
    for (uint16_t i = 0; i < glyphCount; i++) {
      uint16_t v;
      if (!in.u16(&v)) {
        return nullptr;
      }
      r->glyphArray.push_back(v);
    }
    return r;
  }

  bool write(OutputStream &out) override {
    using namespace std;
    if (!out.u16(1)) {
      return false;
    }
    if (!out.sizeU16(glyphArray.size())) {
      return false;
    }
    for (uint16_t glyph : glyphArray) {
      if (!out.u16(glyph)) {
        return false;
      }
    }
    return true;
  }

public:
  std::vector<uint16_t> glyphArray;
};

} // namespace eglyf
