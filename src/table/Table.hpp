#pragma once

namespace eglyf {

class Table {
public:
  virtual ~Table() {}
  struct EncodeResult {
    std::string data;
    uint32_t length;
    uint32_t checksum;

    explicit EncodeResult(std::string const &buffer) : data(buffer), length(data.size()) {
      if (length % 4 != 0) {
        data.resize(length + 4 - (length % 4));
      }
      checksum = *Checksum(data);
    }

    EncodeResult(std::string const &data, uint32_t length, uint32_t checksum) : data(data), length(length), checksum(checksum) {}
  };
  virtual Optional<EncodeResult> encode() const = 0;

  static std::optional<uint32_t> Checksum(std::string const &table) {
    if (table.size() % 4 != 0) {
      return std::nullopt;
    }
    uint32_t const *ptr = (uint32_t const *)table.data();
    uint32_t sum = 0;
    uint32_t const *const end = ptr + table.size() / 4;
    while (ptr < end) {
      sum += *ptr++;
    }
    return sum;
  }
};

} // namespace eglyf
