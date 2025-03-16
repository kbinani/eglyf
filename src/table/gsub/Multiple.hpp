#pragma once

namespace eglyf::gsub {

class Multiple : public Subtable {
public:
  struct Sequence {
    std::vector<uint16_t> substituteGlyphIDs;

    static Optional<Sequence> Read(InputStream &in) {
      using namespace std;
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_NULLOPT;
      }
      Sequence r;
      r.substituteGlyphIDs.reserve(glyphCount);
      for (uint16_t i = 0; i < glyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return EGLYF_NULLOPT;
        }
        r.substituteGlyphIDs.push_back(v);
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(substituteGlyphIDs.size())) {
        return EGLYF_ERROR;
      }
      if (out.u16a(substituteGlyphIDs)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }

    size_t size() const {
      return sizeof(uint16_t) * (1 + substituteGlyphIDs.size());
    }
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
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
    uint16_t sequenceCount;
    if (!in.u16(&sequenceCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> sequenceOffsets;
    sequenceOffsets.reserve(sequenceCount);
    for (uint16_t i = 0; i < sequenceCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return EGLYF_ERROR;
      }
      sequenceOffsets.push_back(v);
    }
    auto r = make_unique<Multiple>();
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (Offset16 offset : sequenceOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      if (auto seq = Sequence::Read(in); seq) {
        r->sequences.push_back(*seq);
      } else {
        return EGLYF_STATUS_PUSH(seq.status());
      }
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(sequences.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> sequenceOffsets;
    for (size_t i = 0; i < sequences.size(); i++) {
      auto offset = beginning->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      sequenceOffsets.push_back(offset);
    }
    for (size_t i = 0; i < sequences.size(); i++) {
      auto const &sequence = sequences[i];
      auto offset = sequenceOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = sequence.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(beginning->commit());
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t) + (1 + sequences.size()) * sizeof(Offset16);
    ret += coverage->size();
    for (auto const &seq : sequences) {
      ret += seq.size();
    }
    return ret;
  }

public:
  std::vector<Sequence> sequences;
};

} // namespace eglyf::gsub
