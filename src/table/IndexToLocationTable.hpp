#pragma once

namespace eglyf {

// 'loca'
class IndexToLocationTable : public Table {
public:
  explicit IndexToLocationTable(int16_t indexToLocFormat) : indexToLocFormat(indexToLocFormat) {}

  static Status Read(InputStream &in, int16_t indexToLocFormat, uint16_t numGlyphs, std::shared_ptr<IndexToLocationTable> &out) {
    using namespace std;
    if (indexToLocFormat == 0) {
      auto r = make_unique<IndexToLocationTable>(indexToLocFormat);
      r->offsets.resize(numGlyphs + 1);
      for (uint16_t i = 0; i <= numGlyphs; i++) {
        Offset16 t;
        if (!in.o16(&t)) {
          return EGLYF_ERROR;
        }
        Offset32 v = t;
        r->offsets[i] = v << 1;
      }
      out.reset(r.release());
      return Status::Ok();
    } else if (indexToLocFormat == 1) {
      auto r = make_unique<IndexToLocationTable>(indexToLocFormat);
      if (!in.o32a(r->offsets, numGlyphs + 1)) {
        return EGLYF_ERROR;
      }
      out.reset(r.release());
      return Status::Ok();
    } else {
      return EGLYF_ERROR;
    }
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (indexToLocFormat == 0) {
      for (Offset32 o : offsets) {
        if (o % 2 != 0) {
          return EGLYF_NULLOPT;
        }
        o = o >> 1;
        if (static_cast<Offset32>(std::numeric_limits<Offset16>::max()) < o) {
          return EGLYF_NULLOPT;
        }
        if (!out.o16(static_cast<Offset16>(0xffff & o))) {
          return EGLYF_NULLOPT;
        }
      }
      return EncodeResult(out.data());
    } else if (indexToLocFormat == 1) {
      for (Offset32 o : offsets) {
        if (!out.o32(o)) {
          return EGLYF_NULLOPT;
        }
      }
      return EncodeResult(out.data());
    } else {
      return EGLYF_NULLOPT;
    }
  }

public:
  int16_t const indexToLocFormat;
  std::vector<Offset32> offsets;
};

} // namespace eglyf
