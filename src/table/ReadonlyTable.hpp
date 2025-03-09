#pragma once

namespace eglyf {

class ReadonlyTable : public Table {
public:
  explicit ReadonlyTable(std::string const &content) : content(content), len(content.size()) {
    if (len % 4 != 0) {
      this->content.resize(len + 4 - (len % 4));
    }
  }

  Optional<EncodeResult> encode() const override {
    auto checksum = Table::Checksum(content);
    if (!checksum) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(content, len, *checksum);
  }

public:
  std::string content;
  uint32_t len;
};

} // namespace eglyf
