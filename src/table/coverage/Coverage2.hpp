#pragma once

namespace eglyf {

class Coverage2 : public Coverage {
public:
  struct RangeRecord {
    uint16_t startGlyphID;
    uint16_t endGlyphID;
    uint16_t startCoverageIndex;
  };

public:
  static std::shared_ptr<Coverage2> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<Coverage2>();
    uint16_t rangeCount;
    if (!in.u16(&rangeCount)) {
      return nullptr;
    }
    r->rangeRecords.reserve(rangeCount);
    for (uint16_t i = 0; i < rangeCount; i++) {
      RangeRecord rr;
      if (!in.u16(&rr.startGlyphID)) {
        return nullptr;
      }
      if (!in.u16(&rr.endGlyphID)) {
        return nullptr;
      }
      if (!in.u16(&rr.startCoverageIndex)) {
        return nullptr;
      }
      r->rangeRecords.push_back(rr);
    }
    return r;
  }

public:
  std::vector<RangeRecord> rangeRecords;
};

} // namespace eglyf
