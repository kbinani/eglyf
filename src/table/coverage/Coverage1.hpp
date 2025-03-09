#pragma once

namespace eglyf {

class Coverage1 : public Coverage {
public:
  static Status Read(InputStream &in, std::shared_ptr<Coverage> &out) {
    using namespace std;
    auto r = make_unique<Coverage1>();
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR;
    }
    if (!in.u16a(r->glyphArray, glyphCount)) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) override {
    using namespace std;
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(glyphArray.size())) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(glyphArray)) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

public:
  std::vector<uint16_t> glyphArray;
};

} // namespace eglyf
