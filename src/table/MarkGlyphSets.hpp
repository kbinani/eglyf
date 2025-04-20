#pragma once

namespace eglyf {

class MarkGlyphSets {
public:
  static Optional<MarkGlyphSets> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_NULLOPT_WHAT("Failed to read format");
    }
    if (format != 1) {
      return EGLYF_NULLOPT_WHAT("Unsupported format: " + std::to_string(format));
    }
    uint16_t markGlyphSetCount;
    if (!in.u16(&markGlyphSetCount)) {
      return EGLYF_NULLOPT_WHAT("Failed to read markGlyphSetCount");
    }
    vector<Offset32> coverageOffsets;
    if (!in.o32a(coverageOffsets, markGlyphSetCount)) {
      return EGLYF_NULLOPT_WHAT("Failed to read coverageOffsets");
    }
    MarkGlyphSets ret;
    for (auto offset : coverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_NULLOPT_WHAT("Failed to seek to coverage offset");
      }
      shared_ptr<Coverage> cov;
      if (auto st = Coverage::Read(in, cov); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      ret.coverages.push_back(cov);
    }
    return ret;
  }

  Status write(OutputStream &stream) const {
    using namespace std;
    auto writer = make_shared<DataFragmentWriter>(&stream);
    if (!writer->u16(1)) {
      return EGLYF_ERROR_WHAT("Failed to write format");
    }
    if (!writer->sizeU16(coverages.size())) {
      return EGLYF_ERROR_WHAT("Failed to write coverages size");
    }
    vector<DataFragmentWriter::Marker32> coverageOffsets;
    for (size_t i = 0; i < coverages.size(); i++) {
      auto offset = writer->o32();
      if (!offset) {
        return EGLYF_ERROR_WHAT("Failed to create coverage offset marker");
      }
      coverageOffsets.push_back(offset);
    }
    for (size_t i = 0; i < coverages.size(); i++) {
      auto const &coverage = coverages[i];
      auto offset = coverageOffsets[i];
      if (auto st = writer->writeDataFragment(offset, *coverage); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  std::vector<std::shared_ptr<Coverage>> coverages;
};

} // namespace eglyf
