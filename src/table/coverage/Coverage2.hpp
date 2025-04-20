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
      return EGLYF_ERROR_WHAT("Failed to read rangeCount");
    }
    r->rangeRecords.reserve(rangeCount);
    for (uint16_t i = 0; i < rangeCount; i++) {
      RangeRecord rr;
      if (!in.u16(&rr.startGlyphID)) {
        return EGLYF_ERROR_WHAT("Failed to read startGlyphID");
      }
      if (!in.u16(&rr.endGlyphID)) {
        return EGLYF_ERROR_WHAT("Failed to read endGlyphID");
      }
      if (!in.u16(&rr.startCoverageIndex)) {
        return EGLYF_ERROR_WHAT("Failed to read startCoverageIndex");
      }
      r->rangeRecords.push_back(rr);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    if (!out.u16(2)) {
      return EGLYF_ERROR_WHAT("Failed to write format");
    }
    if (!out.sizeU16(rangeRecords.size())) {
      return EGLYF_ERROR_WHAT("Failed to write rangeRecords size");
    }
    for (auto const &rangeRecord : rangeRecords) {
      if (!out.u16(rangeRecord.startGlyphID)) {
        return EGLYF_ERROR_WHAT("Failed to write startGlyphID");
      }
      if (!out.u16(rangeRecord.endGlyphID)) {
        return EGLYF_ERROR_WHAT("Failed to write endGlyphID");
      }
      if (!out.u16(rangeRecord.startCoverageIndex)) {
        return EGLYF_ERROR_WHAT("Failed to write startCoverageIndex");
      }
    }
    return Status::Ok();
  }

  size_t index(uint16_t gid) const override {
    using namespace std;
    auto found = ranges::find_if(rangeRecords, [=](auto const &record) {
      return record.startGlyphID <= gid && gid <= record.endGlyphID;
    });
    if (found == rangeRecords.end()) {
      return Npos;
    } else {
      return found->startCoverageIndex + (gid - found->startGlyphID);
    }
  }

  size_t count() const override {
    if (rangeRecords.empty()) {
      return 0;
    }
    auto const &back = rangeRecords.back();
    return back.startCoverageIndex + (back.endGlyphID - back.startGlyphID + 1);
  }

public:
  std::vector<RangeRecord> rangeRecords;
};

} // namespace eglyf
