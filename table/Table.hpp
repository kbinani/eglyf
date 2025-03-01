#pragma once

namespace ksesh {

class Table {
public:
  virtual ~Table() {}
  struct EncodeResult {
    std::string data;
    uint32_t length;

    explicit EncodeResult(std::string const &buffer) : data(buffer), length(data.size()) {
      if (length % 4 != 0) {
        data.resize(length + 4 - length % 4);
      }
    }

    EncodeResult(std::string const &data, uint32_t length) : data(data), length(length) {}
  };
  virtual std::optional<EncodeResult> encode() = 0;

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

} // namespace ksesh
