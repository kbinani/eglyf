#pragma once

namespace eglyf {

class Coverage1 : public Coverage {
public:
  static Status Read(InputStream &in, std::shared_ptr<Coverage> &out) {
    using namespace std;
    auto r = make_unique<Coverage1>();
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR_WHAT("Failed to read glyphCount");
    }
    vector<uint16_t> glyphIdList;
    if (!in.u16a(glyphIdList, glyphCount)) {
      return EGLYF_ERROR_WHAT("Failed to read glyphIdList");
    }
    for (uint16_t gid : glyphIdList) {
      r->glyphArray.insert(gid);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    if (!out.u16(1)) {
      return EGLYF_ERROR_WHAT("Failed to write format");
    }
    if (!out.sizeU16(glyphArray.size())) {
      return EGLYF_ERROR_WHAT("Failed to write glyphArray size");
    }
    for (uint16_t gid : glyphArray) {
      if (!out.u16(gid)) {
        return EGLYF_ERROR_WHAT("Failed to write glyph ID");
      }
    }
    return Status::Ok();
  }

  size_t size() const override {
    return (2 + glyphArray.size()) * sizeof(uint16_t);
  }

public:
  std::set<uint16_t> glyphArray;
};

} // namespace eglyf
