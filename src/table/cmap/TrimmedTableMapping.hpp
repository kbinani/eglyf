#pragma once

namespace eglyf::cmap {

// format 6
class TrimmedTableMapping : public CmapSubtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<CmapSubtable> &out) {
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

public:
  uint16_t language;
  uint16_t firstCode;
  std::vector<uint16_t> glyphIdArray;
};

} // namespace eglyf::cmap
