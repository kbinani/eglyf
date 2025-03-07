#pragma once

namespace eglyf::gsub {

class Multiple : public Subtable {
public:
  struct Sequence {
    std::vector<uint16_t> substituteGlyphIDs;

    static std::optional<Sequence> Read(InputStream &in) {
      using namespace std;
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return nullopt;
      }
      Sequence r;
      r.substituteGlyphIDs.reserve(glyphCount);
      for (uint16_t i = 0; i < glyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.substituteGlyphIDs.push_back(v);
      }
      return r;
    }

    bool write(OutputStream &out) const {
      if (!out.sizeU16(substituteGlyphIDs.size())) {
        return false;
      }
      return out.u16a(substituteGlyphIDs);
    }
  };

public:
  static std::shared_ptr<Multiple> Read(InputStream &in) {
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
    uint16_t sequenceCount;
    if (!in.u16(&sequenceCount)) {
      return nullptr;
    }
    vector<Offset16> sequenceOffsets;
    sequenceOffsets.reserve(sequenceCount);
    for (uint16_t i = 0; i < sequenceCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      sequenceOffsets.push_back(v);
    }
    auto r = make_shared<Multiple>();
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    for (Offset16 offset : sequenceOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto seq = Sequence::Read(in); seq) {
        r->sequences.push_back(*seq);
      } else {
        return nullptr;
      }
    }
    return r;
  }

  bool write(OutputStream &out) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.sizeU16(sequences.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> sequenceOffsets;
    for (size_t i = 0; i < sequences.size(); i++) {
      auto offset = beginning->o16();
      if (!offset) {
        return false;
      }
      sequenceOffsets.push_back(offset);
    }
    for (size_t i = 0; i < sequences.size(); i++) {
      auto const &sequence = sequences[i];
      auto offset = sequenceOffsets[i];
      if (!offset->mark()) {
        return false;
      }
      if (!sequence.write(out)) {
        return false;
      }
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
  std::vector<Sequence> sequences;
};

} // namespace eglyf::gsub
