#pragma once

namespace ksesh {

class ReadonlyTable : public Table {
public:
  explicit ReadonlyTable(std::string const &content, uint32_t length) : content(content), len(length) {}

  std::optional<EncodeResult> encode() override {
    return EncodeResult(content, len);
  }

public:
  std::string const content;
  uint32_t const len;
};

} // namespace ksesh
