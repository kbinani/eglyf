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
  static Status Read(InputStream &in, std::shared_ptr<Coverage> &out) {
    using namespace std;
    auto r = make_unique<Coverage2>();
    uint16_t rangeCount;
    if (!in.u16(&rangeCount)) {
      return EGLYF_ERROR;
    }
    r->rangeRecords.reserve(rangeCount);
    for (uint16_t i = 0; i < rangeCount; i++) {
      RangeRecord rr;
      if (!in.u16(&rr.startGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!in.u16(&rr.endGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!in.u16(&rr.startCoverageIndex)) {
        return EGLYF_ERROR;
      }
      r->rangeRecords.push_back(rr);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) override {
    using namespace std;
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(rangeRecords.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &rangeRecord : rangeRecords) {
      if (!out.u16(rangeRecord.startGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(rangeRecord.endGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(rangeRecord.startCoverageIndex)) {
        return EGLYF_ERROR;
      }
    }
    return Status::Ok();
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t);
    ret += (3 * sizeof(uint16_t)) * rangeRecords.size();
    return ret;
  }

public:
  std::vector<RangeRecord> rangeRecords;
};

} // namespace eglyf
