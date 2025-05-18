#pragma once

namespace eglyf::cff {

class CompactFontFormatTable : public Table {
public:
  struct Header {
    Card8 major;
    Card8 minor;
    Card8 hdrSize;
    OffSize offSize;

    Status read(InputStream &in) {
      if (!in.u8(&major)) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&minor)) {
        return EGLYF_ERROR;
      }
      if (major != 1 || minor != 0) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&hdrSize)) {
        return EGLYF_ERROR;
      }
      if (!in.u8(&offSize)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

public:
  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT;
  }

  static Status Read(InputStream &stream, std::shared_ptr<CompactFontFormatTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    auto ret = make_unique<CompactFontFormatTable>();
    if (auto st = ret->header.read(in); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (!in.seek(ret->header.hdrSize)) {
      return EGLYF_ERROR;
    }
    return EGLYF_ERROR;
  }

  Header header;
};

} // namespace eglyf::cff
