#pragma once

namespace eglyf::gsub {

class SingleFormat1 : public Subtable {
public:
  static std::shared_ptr<Subtable> Read(InputStream &in) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    auto r = make_shared<SingleFormat1>();
    if (!in.i16(&r->deltaGlyphID)) {
      return nullptr;
    }
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    return r;
  }

  bool write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.i16(deltaGlyphID)) {
      return false;
    }
    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }
    return beginning->commit();
  }

public:
  int16_t deltaGlyphID;
};

class SingleFormat2 : public Subtable {
public:
  static std::shared_ptr<Subtable> Read(InputStream &in) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return nullptr;
    }
    auto r = make_shared<SingleFormat2>();
    r->substituteGlyphIDs.reserve(glyphCount);
    for (uint16_t i = 0; i < glyphCount; i++) {
      uint16_t v;
      if (!in.u16(&v)) {
        return nullptr;
      }
      r->substituteGlyphIDs.push_back(v);
    }
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    return r;
  }

  bool write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(2)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.sizeU16(substituteGlyphIDs.size())) {
      return false;
    }
    if (!out.u16a(substituteGlyphIDs)) {
      return false;
    }
    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }
    return beginning->commit();
  }

public:
  std::vector<uint16_t> substituteGlyphIDs;
};

class Single {
  Single() = delete;

public:
  static std::shared_ptr<Subtable> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format == 1) {
      return SingleFormat1::Read(in);
    } else if (format == 2) {
      return SingleFormat2::Read(in);
    } else {
      return nullptr;
    }
  }
};

} // namespace eglyf::gsub
