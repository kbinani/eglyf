#pragma once

namespace eglyf::gsub {

class Alternate : public Subtable {
public:
  struct AlternateSet {
    std::vector<uint16_t> alternateGlyphIDs;

    static std::optional<AlternateSet> Read(InputStream &in) {
      using namespace std;
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return nullopt;
      }
      AlternateSet r;
      if (!in.u16a(r.alternateGlyphIDs, glyphCount)) {
        return nullopt;
      }
      return r;
    }

    bool write(OutputStream &out) const {
      if (!out.sizeU16(alternateGlyphIDs.size())) {
        return false;
      }
      return out.u16a(alternateGlyphIDs);
    }
  };

public:
  static std::shared_ptr<Alternate> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format != 1) {
      return nullptr;
    }
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    uint16_t alternateSetCount;
    if (!in.u16(&alternateSetCount)) {
      return nullptr;
    }
    vector<Offset16> alternateSetOffsets;
    if (!in.u16a(alternateSetOffsets, alternateSetCount)) {
      return nullptr;
    }
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    auto r = make_shared<Alternate>();
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    for (auto offset : alternateSetOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto s = AlternateSet::Read(in); s) {
        r->alternateSets.push_back(*s);
      } else {
        return nullptr;
      }
    }
    return r;
  }

  bool write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return false;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.sizeU16(alternateSets.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> alternateSetOffsets;
    for (size_t i = 0; i < alternateSets.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return false;
      }
      alternateSetOffsets.push_back(offset);
    }
    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }
    for (size_t i = 0; i < alternateSets.size(); i++) {
      auto const &alternateSet = alternateSets[i];
      auto offset = alternateSetOffsets[i];
      if (!offset->mark()) {
        return false;
      }
      if (!alternateSet.write(out)) {
        return false;
      }
    }
    return writer->commit();
  }

public:
  std::vector<AlternateSet> alternateSets;
};

} // namespace eglyf::gsub
