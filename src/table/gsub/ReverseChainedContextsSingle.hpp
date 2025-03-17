#pragma once

namespace eglyf::gsub {

class ReverseChainedContextsSingle : public Subtable {
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
    uint16_t backtrackGlyphCount;
    if (!in.u16(&backtrackGlyphCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> backtrackCoverageOffsets;
    if (!in.o16a(backtrackCoverageOffsets, backtrackGlyphCount)) {
      return EGLYF_ERROR;
    }
    uint16_t lookaheadGlyphCount;
    if (!in.u16(&lookaheadGlyphCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> lookaheadCoverageOffsets;
    if (!in.o16a(lookaheadCoverageOffsets, lookaheadGlyphCount)) {
      return EGLYF_ERROR;
    }
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<ReverseChainedContextsSingle>();
    if (!in.u16a(ret->substituteGlyphIDs, glyphCount)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (auto backtrackOffset : backtrackCoverageOffsets) {
      if (!in.seek(backtrackOffset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      ret->backtrackCoverages.push_back(cov);
    }
    for (auto lookaheadOffset : lookaheadCoverageOffsets) {
      if (!in.seek(lookaheadOffset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      ret->lookaheadCoverages.push_back(cov);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(backtrackCoverages.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> backtrackCoverageOffsets;
    for (size_t i = 0; i < backtrackCoverages.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      backtrackCoverageOffsets.push_back(offset);
    }
    if (!out.sizeU16(lookaheadCoverages.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> lookaheadCoverageOffsets;
    for (size_t i = 0; i < lookaheadCoverages.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      lookaheadCoverageOffsets.push_back(offset);
    }
    if (!out.sizeU16(substituteGlyphIDs.size())) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(substituteGlyphIDs)) {
      return EGLYF_ERROR;
    }

    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (size_t i = 0; i < backtrackCoverages.size(); i++) {
      auto const &cov = backtrackCoverages[i];
      auto offset = backtrackCoverageOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (size_t i = 0; i < lookaheadCoverages.size(); i++) {
      auto const &cov = lookaheadCoverages[i];
      auto offset = lookaheadCoverageOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t) + sizeof(Offset16);
    ret += coverage->size();
    ret += sizeof(uint16_t) + backtrackCoverages.size() * sizeof(Offset16);
    for (auto const &cov : backtrackCoverages) {
      ret += cov->size();
    }
    ret += sizeof(uint16_t) + lookaheadCoverages.size() * sizeof(Offset16);
    for (auto const &cov : lookaheadCoverages) {
      ret += cov->size();
    }
    ret += sizeof(uint16_t) + substituteGlyphIDs.size() * sizeof(uint16_t);
    return ret;
  }

public:
  std::vector<std::shared_ptr<Coverage>> backtrackCoverages;
  std::vector<std::shared_ptr<Coverage>> lookaheadCoverages;
  std::vector<uint16_t> substituteGlyphIDs;
};

} // namespace eglyf::gsub
