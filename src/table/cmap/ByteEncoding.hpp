#pragma once

namespace eglyf::cmap {

// format 0
class ByteEncoding : public CmapSubtable {
public:
  static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
    return EGLYF_ERROR;
  }
};

} // namespace eglyf::cmap
