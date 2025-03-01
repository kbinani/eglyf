#pragma once

namespace eglyf {

class CoverageReader {
  CoverageReader() = delete;

public:
  static std::shared_ptr<Coverage> Read(InputStream &in) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format == 1) {
      return Coverage1::Read(in);
    } else if (format == 2) {
      return Coverage2::Read(in);
    } else {
      return nullptr;
    }
  }
};

} // namespace eglyf
