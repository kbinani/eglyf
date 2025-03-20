#pragma once

namespace eglyf {

class ClassDef {
public:
  virtual ~ClassDef() {}
  virtual Status write(OutputStream &out) const = 0;
  virtual size_t size() const = 0;
};

class ClassDef1 : public ClassDef {
public:
  static Status Read(InputStream &in, std::shared_ptr<ClassDef> &out) {
    using namespace std;
    auto r = make_unique<ClassDef1>();
    if (!in.u16(&r->startGlyphID)) {
      return EGLYF_ERROR;
    }
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR;
    }
    if (!in.u16a(r->classValues, glyphCount)) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(startGlyphID)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(classValues.size())) {
      return EGLYF_ERROR;
    }
    if (out.u16a(classValues)) {
      return Status::Ok();
    } else {
      return EGLYF_ERROR;
    }
  }

  size_t size() const override {
    return 2 * sizeof(uint16_t) + sizeof(Offset16) + classValues.size() * sizeof(uint16_t);
  }

public:
  uint16_t startGlyphID;
  std::vector<uint16_t> classValues;
};

class ClassDef2 : public ClassDef {
public:
  struct ClassRange {
    uint16_t startGlyphID;
    uint16_t endGlyphID;
    uint16_t classValue;

    static Optional<ClassRange> Read(InputStream &in) {
      using namespace std;
      ClassRange r;
      if (!in.u16(&r.startGlyphID)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.endGlyphID)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.classValue)) {
        return EGLYF_NULLOPT;
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.u16(startGlyphID)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(endGlyphID)) {
        return EGLYF_ERROR;
      }
      if (out.u16(classValue)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<ClassDef> &out) {
    using namespace std;
    uint16_t classRangeCount;
    if (!in.u16(&classRangeCount)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<ClassDef2>();
    r->classRanges.reserve(classRangeCount);
    for (uint16_t i = 0; i < classRangeCount; i++) {
      if (auto cr = ClassRange::Read(in); cr) {
        r->classRanges.push_back(*cr);
      } else {
        return EGLYF_STATUS_PUSH(cr.status());
      }
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(classRanges.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &range : classRanges) {
      if (auto st = range.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return Status::Ok();
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t) + sizeof(Offset16);
    ret += 3 * sizeof(uint16_t) * classRanges.size();
    return ret;
  }

public:
  std::vector<ClassRange> classRanges;
};

class ClassDefReader {
  ClassDefReader() = delete;

public:
  static Status Read(InputStream &in, std::shared_ptr<ClassDef> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      auto st = ClassDef1::Read(in, out);
      return EGLYF_STATUS_PUSH(st);
    } else if (format == 2) {
      auto st = ClassDef2::Read(in, out);
      return EGLYF_STATUS_PUSH(st);
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
