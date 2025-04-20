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

  Status write(OutputStream &stream, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<DataFragmentWriter>(&stream);
    if (!writer->u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!writer->sizeU16(backtrackCoverages.size())) {
      return EGLYF_ERROR;
    }
    vector<DataFragmentWriter::Marker16> backtrackCoverageOffsets;
    for (size_t i = 0; i < backtrackCoverages.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      backtrackCoverageOffsets.push_back(offset);
    }
    if (!writer->sizeU16(lookaheadCoverages.size())) {
      return EGLYF_ERROR;
    }
    vector<DataFragmentWriter::Marker16> lookaheadCoverageOffsets;
    for (size_t i = 0; i < lookaheadCoverages.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      lookaheadCoverageOffsets.push_back(offset);
    }
    if (!writer->sizeU16(substituteGlyphIDs.size())) {
      return EGLYF_ERROR;
    }
    if (!writer->u16a(substituteGlyphIDs)) {
      return EGLYF_ERROR;
    }

    if (auto st = writer->writeDataFragment(coverageOffset, *coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (size_t i = 0; i < backtrackCoverages.size(); i++) {
      auto const &cov = backtrackCoverages[i];
      auto offset = backtrackCoverageOffsets[i];
      if (auto st = writer->writeDataFragment(offset, *cov); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (size_t i = 0; i < lookaheadCoverages.size(); i++) {
      auto const &cov = lookaheadCoverages[i];
      auto offset = lookaheadCoverageOffsets[i];
      if (auto st = writer->writeDataFragment(offset, *cov); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  std::vector<std::shared_ptr<Coverage>> backtrackCoverages;
  std::vector<std::shared_ptr<Coverage>> lookaheadCoverages;
  std::vector<uint16_t> substituteGlyphIDs;
};

} // namespace eglyf::gsub
