#pragma once

namespace eglyf::cmap {

// format 0
class ByteEncoding : public CmapSubtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<CmapSubtable> &out) {
    using namespace std;
    auto ret = make_unique<ByteEncoding>();
    uint16_t length;
    if (!in.u16(&length)) {
      return EGLYF_ERROR;
    }
    if (length != 262) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&ret->language)) {
      return EGLYF_ERROR;
    }
    if (!in.read(ret->glyphIdArray.data(), ret->glyphIdArray.size())) {
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(262)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(language)) {
      return EGLYF_ERROR;
    }
    if (!out.write(glyphIdArray.data(), glyphIdArray.size())) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

public:
  uint16_t language;
  std::array<uint8_t, 256> glyphIdArray;
};

} // namespace eglyf::cmap
