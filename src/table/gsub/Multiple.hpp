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

public:
  std::vector<Sequence> sequences;
};

} // namespace eglyf::gsub
