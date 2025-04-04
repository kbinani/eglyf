#pragma once

namespace eglyf {

class CoverageReader {
  CoverageReader() = delete;

public:
  static Status Read(InputStream &in, std::shared_ptr<Coverage> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR_WHAT("Failed to read coverage format");
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(Coverage1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(Coverage2::Read(in, out));
    } else {
      return EGLYF_ERROR_WHAT("Unsupported coverage format: " + std::to_string(format));
    }
  }
};

} // namespace eglyf
