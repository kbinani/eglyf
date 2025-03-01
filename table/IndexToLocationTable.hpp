#pragma once

namespace ksesh::otf {

// 'loca'
class IndexToLocationTable : public Table {
public:
  explicit IndexToLocationTable(int16_t indexToLocFormat) : indexToLocFormat(indexToLocFormat) {}

  static std::shared_ptr<IndexToLocationTable> Read(InputStream &in, int16_t indexToLocFormat, uint16_t numGlyphs) {
    using namespace std;
    if (indexToLocFormat == 0) {
      auto r = make_shared<IndexToLocationTable>(indexToLocFormat);
      r->offsets.resize(numGlyphs + 1);
      for (uint16_t i = 0; i <= numGlyphs; i++) {
        Offset16 t;
        if (!in.o16(&t)) {
          return nullptr;
        }
        r->offsets[i] = t * 2;
      }
      return r;
    } else if (indexToLocFormat == 1) {
      auto r = make_shared<IndexToLocationTable>(indexToLocFormat);
      r->offsets.resize(numGlyphs + 1);
      for (uint16_t i = 0; i <= numGlyphs; i++) {
        if (!in.o32(r->offsets.data() + i)) {
          return nullptr;
        }
      }
      return r;
    } else {
      return nullptr;
    }
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    if (indexToLocFormat == 0) {
      for (Offset32 o : offsets) {
        if (o % 2 != 0) {
          return nullopt;
        }
        o = o >> 1;
        if (static_cast<Offset32>(std::numeric_limits<Offset16>::max()) < o) {
          return nullopt;
        }
        if (!out.o16(static_cast<Offset16>(0xffff & o))) {
          return nullopt;
        }
      }
      return EncodeResult(out.data());
    } else if (indexToLocFormat == 1) {
      for (Offset32 o : offsets) {
        if (!out.o32(o)) {
          return nullopt;
        }
      }
      return EncodeResult(out.data());
    } else {
      return nullopt;
    }
  }

public:
  int16_t const indexToLocFormat;
  std::vector<Offset32> offsets;
};

} // namespace ksesh::otf
