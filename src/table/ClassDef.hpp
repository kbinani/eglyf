#pragma once

namespace eglyf {

class ClassDef {
public:
  virtual ~ClassDef() {}
  virtual Status write(OutputStream &out) const = 0;
  virtual size_t size() const = 0;
  virtual Status add(uint16_t glyphId, uint16_t classValue) = 0;
  virtual void enumerateClassValues(std::function<void(uint16_t gid, uint16_t classValue)> cb) const = 0;
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

  Status add(uint16_t glyphId, uint16_t classValue) override {
    if (glyphId < startGlyphID) {
      int num = startGlyphID - glyphId;
      classValues.resize(classValues.size() + num);
      for (int i = (int)classValues.size() - 1; i >= num; i++) {
        classValues[i] = classValues[i - num];
      }
      for (int i = 0; i < num; i++) {
        classValues[i] = 0;
      }
      classValues[0] = classValue;
      return Status::Ok();
    } else {
      classValues[glyphId - startGlyphID] = classValue;
      return Status::Ok();
    }
  }

  void enumerateClassValues(std::function<void(uint16_t gid, uint16_t classValue)> cb) const override {
    for (size_t i = 0; i < classValues.size(); i++) {
      auto gid = i + startGlyphID;
      cb(gid, classValues[i]);
    }
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

  Status add(uint16_t glyphId, uint16_t classValue) override {
    using namespace std;
    ClassRange single;
    single.startGlyphID = glyphId;
    single.endGlyphID = glyphId;
    single.classValue = classValue;

    if (classRanges.empty()) {
      classRanges.push_back(single);
      return Status::Ok();
    }
    auto found = ranges::find_if(classRanges, [=](auto const &r) {
      return glyphId <= r.endGlyphID;
    });
    if (found == classRanges.end()) {
      if (!classRanges.empty()) {
        ClassRange &r = classRanges.back();
        if (r.endGlyphID + 1 == glyphId && r.classValue == classValue) {
          r.endGlyphID = glyphId;
          return Status::Ok();
        }
      }
      classRanges.push_back(single);
      return Status::Ok();
    }
    size_t const index = distance(classRanges.begin(), found);
    auto &center = classRanges[index];
    if (glyphId + 1 == center.startGlyphID && center.classValue == classValue) {
      center.startGlyphID = glyphId;
      return Status::Ok();
    } else if (center.endGlyphID + 1 == glyphId && center.classValue == classValue) {
      center.endGlyphID = glyphId;
      return Status::Ok();
    } else if (glyphId < center.startGlyphID) {
      classRanges.insert(classRanges.begin() + index, single);
      return Status::Ok();
    } else if (glyphId == center.startGlyphID) {
      center.startGlyphID = glyphId + 1;
      classRanges.insert(classRanges.begin() + index, single);
      return Status::Ok();
    } else if (glyphId == center.endGlyphID) {
      center.endGlyphID = glyphId - 1;
      classRanges.insert(classRanges.begin() + index + 1, single);
      return Status::Ok();
    } else {
      ClassRange copy = center;
      ClassRange left;
      left.startGlyphID = copy.startGlyphID;
      left.endGlyphID = glyphId - 1;
      left.classValue = copy.classValue;
      center.startGlyphID = glyphId;
      center.endGlyphID = glyphId;
      center.classValue = classValue;
      ClassRange right;
      right.startGlyphID = glyphId + 1;
      right.endGlyphID = copy.endGlyphID;
      right.classValue = copy.classValue;
      classRanges.insert(classRanges.begin() + index, left);
      classRanges.insert(classRanges.begin() + index + 2, right);
      return Status::Ok();
    }
  }

  void enumerateClassValues(std::function<void(uint16_t gid, uint16_t classValue)> cb) const override {
    for (auto const &range : classRanges) {
      for (uint16_t gid = range.startGlyphID; gid <= range.endGlyphID; gid++) {
        cb(gid, range.classValue);
      }
    }
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
