#pragma once

namespace eglyf::gsub {

class Alternate : public Subtable {
public:
  struct AlternateSet {
    std::vector<uint16_t> alternateGlyphIDs;

    static Optional<AlternateSet> Read(InputStream &in) {
      using namespace std;
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_NULLOPT;
      }
      AlternateSet r;
      if (!in.u16a(r.alternateGlyphIDs, glyphCount)) {
        return EGLYF_NULLOPT;
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(alternateGlyphIDs.size())) {
        return EGLYF_ERROR;
      }
      if (out.u16a(alternateGlyphIDs)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t alternateSetCount;
    if (!in.u16(&alternateSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> alternateSetOffsets;
    if (!in.o16a(alternateSetOffsets, alternateSetCount)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<Alternate>();
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (auto offset : alternateSetOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      if (auto s = AlternateSet::Read(in); s) {
        r->alternateSets.push_back(*s);
      } else {
        return EGLYF_STATUS_PUSH(s.status());
      }
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(alternateSets.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> alternateSetOffsets;
    for (size_t i = 0; i < alternateSets.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      alternateSetOffsets.push_back(offset);
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (size_t i = 0; i < alternateSets.size(); i++) {
      auto const &alternateSet = alternateSets[i];
      auto offset = alternateSetOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = alternateSet.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  std::vector<AlternateSet> alternateSets;
};

} // namespace eglyf::gsub
