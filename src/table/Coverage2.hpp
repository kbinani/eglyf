#pragma once

namespace eglyf {

class Coverage2 {
public:
  struct RangeRecord {
    uint16_t startGlyphID;
    uint16_t endGlyphID;
    uint16_t startCoverageIndex;
  };

public:
  static std::optional<Coverage2> Read(InputStream &in) {
    using namespace std;
    Coverage2 r;
    uint16_t rangeCount;
    if (!in.u16(&rangeCount)) {
      return nullopt;
    }
    r.rangeRecords.reserve(rangeCount);
    for (uint16_t i = 0; i < rangeCount; i++) {
      RangeRecord rr;
      if (!in.u16(&rr.startGlyphID)) {
        return nullopt;
      }
      if (!in.u16(&rr.endGlyphID)) {
        return nullopt;
      }
      if (!in.u16(&rr.startCoverageIndex)) {
        return nullopt;
      }
      r.rangeRecords.push_back(rr);
    }
    return r;
  }

public:
  std::vector<RangeRecord> rangeRecords;
};

} // namespace eglyf
