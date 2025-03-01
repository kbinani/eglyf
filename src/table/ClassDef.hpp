#pragma once

namespace eglyf {

class ClassDef {
public:
  virtual ~ClassDef() {}
};

class ClassDef1 : public ClassDef {
public:
  static std::shared_ptr<ClassDef1> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<ClassDef1>();
    if (!in.u16(&r->startGlyphID)) {
      return nullptr;
    }
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return nullptr;
    }
    r->classValues.reserve(glyphCount);
    for (uint16_t i = 0; i < glyphCount; i++) {
      uint16_t v;
      if (!in.u16(&v)) {
        return nullptr;
      }
      r->classValues.push_back(v);
    }
    return r;
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

    static std::optional<ClassRange> Read(InputStream &in) {
      using namespace std;
      ClassRange r;
      if (!in.u16(&r.startGlyphID)) {
        return nullopt;
      }
      if (!in.u16(&r.endGlyphID)) {
        return nullopt;
      }
      if (!in.u16(&r.classValue)) {
        return nullopt;
      }
      return r;
    }
  };

public:
  static std::shared_ptr<ClassDef2> Read(InputStream &in) {
    using namespace std;
    uint16_t classRangeCount;
    if (!in.u16(&classRangeCount)) {
      return nullptr;
    }
    auto r = make_shared<ClassDef2>();
    r->classRanges.reserve(classRangeCount);
    for (uint16_t i = 0; i < classRangeCount; i++) {
      if (auto cr = ClassRange::Read(in); cr) {
        r->classRanges.push_back(*cr);
      } else {
        return nullptr;
      }
    }
    return r;
  }

public:
  std::vector<ClassRange> classRanges;
};

class ClassDefReader {
  ClassDefReader() = delete;

public:
  static std::shared_ptr<ClassDef> Read(InputStream &in) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format == 1) {
      return ClassDef1::Read(in);
    } else if (format == 2) {
      return ClassDef2::Read(in);
    } else {
      return nullptr;
    }
  }
};

} // namespace eglyf
