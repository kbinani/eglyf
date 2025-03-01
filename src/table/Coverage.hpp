#pragma once

namespace eglyf {

class Coverage {
  Coverage() {}

public:
  static std::optional<std::variant<Coverage1, Coverage2>> Read(InputStream &in) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return nullopt;
    }
    if (format == 1) {
      return Coverage1::Read(in);
    } else if (format == 2) {
      return Coverage2::Read(in);
    } else {
      return nullopt;
    }
  }
};

} // namespace eglyf
