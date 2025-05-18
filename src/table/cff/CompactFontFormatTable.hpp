#pragma once

namespace eglyf::cff {

class CompactFontFormatTable : public Table {
public:
  struct Header {
    Card8 major;
    Card8 minor;
    Card8 hdrSize;
    OffSize offSize;
  };

public:
  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT;
  }

  static Status Read(InputStream &stream, std::shared_ptr<CompactFontFormatTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    auto ret = make_unique<CompactFontFormatTable>();
    if (!in.u8(&ret->header.major)) {
      return EGLYF_ERROR;
    }
    if (!in.u8(&ret->header.minor)) {
      return EGLYF_ERROR;
    }
    if (ret->header.major != 1 || ret->header.minor != 0) {
      return EGLYF_ERROR;
    }
    if (!in.u8(&ret->header.hdrSize)) {
      return EGLYF_ERROR;
    }
    if (!in.u8(&ret->header.offSize)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(ret->header.hdrSize)) {
      return EGLYF_ERROR;
    }
    return EGLYF_ERROR;
  }

  Header header;
};

} // namespace eglyf::cff
