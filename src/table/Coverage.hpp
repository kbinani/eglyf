#pragma once

namespace eglyf {

class Coverage {
  struct RangeRecord {
    uint16_t startGlyphID;
    uint16_t endGlyphID;
    uint16_t startCoverageIndex;

    RangeRecord(uint16_t startGlyphID, uint16_t startCoverageIndex) : startGlyphID(startGlyphID), endGlyphID(startGlyphID), startCoverageIndex(startCoverageIndex) {}

    Status write(OutputStream &out) const {
      if (!out.u16(startGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(endGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(startCoverageIndex)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

public:
  static constexpr size_t Npos = -1;

public:
  Status write(OutputStream &out) const {
    using namespace std;
    if (!out.u16(2)) {
      return EGLYF_ERROR_WHAT("Failed to write format");
    }
    auto sizePos = out.position();
    if (!out.sizeU16(0)) {
      return EGLYF_ERROR_WHAT("Failed to write rangeRecords size");
    }
    optional<RangeRecord> last;
    uint16_t currentCoverageIndex = 0;
    size_t count = 0;
    for (uint16_t gid : glyphIDs) {
      if (last) {
        if (last->endGlyphID + 1 == gid) {
          last->endGlyphID = gid;
        } else {
          if (auto st = last->write(out); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          count++;
          currentCoverageIndex += (last->endGlyphID - last->startGlyphID + 1);
          RangeRecord r(gid, currentCoverageIndex);
          last = r;
        }
      } else {
        RangeRecord r(gid, currentCoverageIndex);
        last = r;
      }
    }
    if (last) {
      if (auto st = last->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      count++;
    }
    auto pos = out.position();
    if (!out.seek(sizePos)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(count)) {
      return EGLYF_ERROR;
    }
    if (!out.seek(pos)) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

  size_t index(uint16_t gid) const {
    using namespace std;
    if (auto found = glyphIDs.find(gid); found != glyphIDs.end()) {
      return distance(glyphIDs.begin(), found);
    } else {
      return Npos;
    }
  }

  size_t size() const {
    return glyphIDs.size();
  }

  void insert(uint16_t gid) {
    glyphIDs.insert(gid);
  }

  static Status Read(InputStream &in, std::shared_ptr<Coverage> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR_WHAT("Failed to read coverage format");
    }
    if (format == 1) {
      auto ret = make_unique<Coverage>();
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_ERROR_WHAT("Failed to read glyphCount");
      }
      vector<uint16_t> glyphIDList;
      if (!in.u16a(glyphIDList, glyphCount)) {
        return EGLYF_ERROR_WHAT("Failed to read glyphIDList");
      }
      for (uint16_t gid : glyphIDList) {
        ret->glyphIDs.insert(gid);
      }
      out.reset(ret.release());
      return Status::Ok();
    } else if (format == 2) {
      auto ret = make_unique<Coverage>();
      uint16_t rangeCount;
      if (!in.u16(&rangeCount)) {
        return EGLYF_ERROR_WHAT("Failed to read rangeCount");
      }
      for (uint16_t i = 0; i < rangeCount; i++) {
        uint16_t startGlyphID;
        uint16_t endGlyphID;
        uint16_t startCoverageIndex;
        if (!in.u16(&startGlyphID)) {
          return EGLYF_ERROR_WHAT("Failed to read startGlyphID");
        }
        if (!in.u16(&endGlyphID)) {
          return EGLYF_ERROR_WHAT("Failed to read endGlyphID");
        }
        if (!in.u16(&startCoverageIndex)) {
          return EGLYF_ERROR_WHAT("Failed to read startCoverageIndex");
        }
        for (uint16_t gid = startGlyphID; gid <= endGlyphID; gid++) {
          ret->glyphIDs.insert(gid);
        }
      }
      out.reset(ret.release());
      return Status::Ok();
    } else {
      return EGLYF_ERROR_WHAT("Unsupported coverage format: " + std::to_string(format));
    }
  }

public:
  std::set<uint16_t> glyphIDs;
};

} // namespace eglyf
