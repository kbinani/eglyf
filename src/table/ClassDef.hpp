#pragma once

namespace eglyf {

class ClassDef {
public:
  Status write(OutputStream &out) const {
    using namespace std;
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    auto sizePos = out.position();
    if (!out.sizeU16(0)) {
      return EGLYF_ERROR;
    }
    struct Record {
      Record(uint16_t startGlyphID, uint16_t classValue) : startGlyphID(startGlyphID), endGlyphID(startGlyphID), classValue(classValue) {}

      Status write(OutputStream &out) const {
        if (!out.u16(startGlyphID)) {
          return EGLYF_ERROR;
        }
        if (!out.u16(endGlyphID)) {
          return EGLYF_ERROR;
        }
        if (!out.u16(classValue)) {
          return EGLYF_ERROR;
        }
        return Status::Ok();
      }

      uint16_t startGlyphID;
      uint16_t endGlyphID;
      uint16_t classValue;
    };
    optional<Record> last;
    size_t count = 0;
    for (auto [gid, classValue] : classValues) {
      if (last) {
        if (last->classValue == classValue && last->endGlyphID + 1 == gid) {
          last->endGlyphID = gid;
        } else {
          if (auto st = last->write(out); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          count++;
          Record r(gid, classValue);
          last = r;
        }
      } else {
        Record r(gid, classValue);
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

  size_t size() const {
    return sizeof(uint16_t) * (1 + classValues.size());
  }

  Status add(uint16_t glyphID, uint16_t classValue) {
    classValues[glyphID] = classValue;
    return Status::Ok();
  }

  void enumerateClassValues(std::function<void(uint16_t gid, uint16_t classValue)> cb) const {
    for (auto [gid, classValue] : classValues) {
      cb(gid, classValue);
    }
  }

  static Status Read(InputStream &in, std::shared_ptr<ClassDef> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR_WHAT("Failed to read format");
    }
    auto ret = make_unique<ClassDef>();
    if (format == 1) {
      uint16_t startGlyphID;
      if (!in.u16(&startGlyphID)) {
        return EGLYF_ERROR_WHAT("Failed to read startGlyphID");
      }
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_ERROR_WHAT("Failed to read glyphCount");
      }
      vector<uint16_t> classValues;
      if (!in.u16a(classValues, glyphCount)) {
        return EGLYF_ERROR_WHAT("Failed to read classValues");
      }
      for (uint16_t i = 0; i < glyphCount; i++) {
        uint16_t gid = startGlyphID + i;
        ret->classValues[gid] = classValues[i];
      }
      out.reset(ret.release());
      return Status::Ok();
    } else if (format == 2) {
      uint16_t classRangeCount;
      if (!in.u16(&classRangeCount)) {
        return EGLYF_ERROR_WHAT("Failed to read classRangeCount");
      }
      for (uint16_t i = 0; i < classRangeCount; i++) {
        uint16_t startGlyphID;
        if (!in.u16(&startGlyphID)) {
          return EGLYF_ERROR_WHAT("Failed to read startGlyphID");
        }
        uint16_t endGlyphID;
        if (!in.u16(&endGlyphID)) {
          return EGLYF_ERROR_WHAT("Failed to read endGlyphID");
        }
        uint16_t classValue;
        if (!in.u16(&classValue)) {
          return EGLYF_ERROR_WHAT("Failed to read classValue");
        }
        for (uint16_t gid = startGlyphID; gid <= endGlyphID; gid++) {
          ret->classValues[gid] = classValue;
        }
      }
      out.reset(ret.release());
      return Status::Ok();
    } else {
      return EGLYF_ERROR_WHAT("Unsupported format: " + std::to_string(format));
    }
  }

public:
  std::map<uint16_t, uint16_t> classValues;
};

} // namespace eglyf
