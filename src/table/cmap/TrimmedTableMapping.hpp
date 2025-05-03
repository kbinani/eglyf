#pragma once

namespace eglyf::cmap {

// format 6
class TrimmedTableMapping : public CmapSubtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<TrimmedTableMapping> &out) {
    using namespace std;
    uint16_t length;
    if (!in.u16(&length)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<TrimmedTableMapping>();
    if (!in.u16(&ret->language)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&ret->firstCode)) {
      return EGLYF_ERROR;
    }
    uint16_t entryCount;
    if (!in.u16(&entryCount)) {
      return EGLYF_ERROR;
    }
    if (!in.u16a(ret->glyphIDArray, entryCount)) {
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(6)) {
      return EGLYF_ERROR;
    }
    auto const lengthPos = writer->o16();
    if (!lengthPos) {
      return EGLYF_ERROR;
    }
    if (!out.u16(language)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(firstCode)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(glyphIDArray.size())) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(glyphIDArray)) {
      return EGLYF_ERROR;
    }
    if (auto st = lengthPos->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  Status map(uint32_t codepoint, uint16_t glyphID) {
    using namespace std;
    if (codepoint > 0xffff) {
      return EGLYF_ERROR;
    }
    if (codepoint < firstCode) {
      int num = firstCode - codepoint;
      glyphIDArray.resize(glyphIDArray.size() + num);
      for (int i = glyphIDArray.size() - 1; i >= num; i--) {
        glyphIDArray[i] = glyphIDArray[i - num];
      }
      glyphIDArray[0] = glyphID;
      for (int i = 1; i < num; i++) {
        glyphIDArray[i] = 0;
      }
      firstCode = codepoint;
      return Status::Ok();
    }
    size_t index = codepoint - firstCode;
    if (index < glyphIDArray.size()) {
      glyphIDArray[index] = glyphID;
      return Status::Ok();
    } else {
      glyphIDArray.resize(index + 1, 0);
      glyphIDArray[index] = glyphID;
      return Status::Ok();
    }
  }

  bool writeMayFail() const {
    using namespace std;
    return sizeof(uint16_t) * 2 + sizeof(Offset16) + sizeof(uint16_t) * glyphIDArray.size() > (size_t)numeric_limits<uint16_t>::max();
  }

  Status convertToFormat4(std::shared_ptr<SegmentMappingToDeltaValues> &out) const {
    using namespace std;
    auto ret = make_unique<SegmentMappingToDeltaValues>();
    ret->language = language;
    uint32_t code = firstCode;
    for (auto gid : glyphIDArray) {
      ret->map(code, gid);
      code++;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  void forEach(std::function<void(uint32_t codepoint, uint32_t gid)> cb) const {
    if (!cb) {
      return;
    }
    for (size_t i = 0; i < glyphIDArray.size(); i++) {
      cb(firstCode + i, glyphIDArray[i]);
    }
  }

public:
  uint16_t language;
  uint16_t firstCode;
  std::vector<uint16_t> glyphIDArray;
};

} // namespace eglyf::cmap
