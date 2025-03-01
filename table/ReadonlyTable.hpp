#pragma once

namespace ksesh {

class ReadonlyTable : public Table {
public:
  explicit ReadonlyTable(std::string const &content) : content(content), len(content.size()) {
    if (len % 4 != 0) {
      this->content.resize(len + 4 - (len % 4));
    }
  }

  std::optional<EncodeResult> encode() override {
    auto checksum = Table::Checksum(content);
    if (!checksum) {
      return std::nullopt;
    }
    return EncodeResult(content, len, *checksum);
  }

public:
  std::string content;
  uint32_t len;
};

} // namespace ksesh
