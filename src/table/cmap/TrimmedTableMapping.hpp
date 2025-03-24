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
    if (!in.u16a(ret->glyphIdArray, entryCount)) {
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
    if (!out.sizeU16(glyphIdArray.size())) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(glyphIdArray)) {
      return EGLYF_ERROR;
    }
    if (auto st = lengthPos->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  Status map(uint32_t codepoint, uint16_t glyphId) {
    return EGLYF_ERROR;
  }

public:
  uint16_t language;
  uint16_t firstCode;
  std::vector<uint16_t> glyphIdArray;
};

} // namespace eglyf::cmap
