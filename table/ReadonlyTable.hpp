#pragma once

namespace ksesh {

class ReadonlyTable : public Table {
public:
  explicit ReadonlyTable(std::string const &content, uint32_t length) : content(content), len(length) {}

  std::optional<EncodeResult> encode() override {
    EncodeResult er;
    er.data = content;
    er.length = len;
    return er;
  }

public:
  std::string const content;
  uint32_t const len;
};

} // namespace ksesh
