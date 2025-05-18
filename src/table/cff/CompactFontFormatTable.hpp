#pragma once

namespace eglyf::cff {

class CompactFontFormatTable : public Table {
public:
  static Status Read(InputStream &stream, std::shared_ptr<CompactFontFormatTable> &out) {
    return EGLYF_ERROR;
  }
};

} // namespace eglyf::cff
