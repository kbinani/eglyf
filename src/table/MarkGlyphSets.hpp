#pragma once

namespace eglyf {

class MarkGlyphSets {
public:
  static Optional<MarkGlyphSets> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_NULLOPT;
    }
    if (format != 1) {
      return EGLYF_NULLOPT;
    }
    uint16_t markGlyphSetCount;
    if (!in.u16(&markGlyphSetCount)) {
      return EGLYF_NULLOPT;
    }
    vector<Offset16> coverageOffsets;
    if (!in.o16a(coverageOffsets, markGlyphSetCount)) {
      return EGLYF_NULLOPT;
    }
    MarkGlyphSets ret;
    for (auto offset : coverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_NULLOPT;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      ret.coverages.push_back(cov);
    }
    return ret;
  }

public:
  std::vector<std::shared_ptr<Coverage>> coverages;
};

} // namespace eglyf
