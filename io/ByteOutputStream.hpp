#pragma once

namespace ksesh {

class ByteOutputStream : public OutputStream {
public:
  bool i64(int64_t v) override {
    uint64_t t = juce::ByteOrder::bigEndianInt64(&v);
    if (loc + 8 > buffer.size()) {
      buffer.resize(loc + 8);
    }
    std::copy_n((char const *)&t, 8, buffer.data() + loc);
    loc += 8;
    return true;
  }

  bool u32(uint32_t v) override {
    uint32_t t = juce::ByteOrder::bigEndianInt(&v);
    if (loc + 4 > buffer.size()) {
      buffer.resize(loc + 4);
    }
    std::copy_n((char const *)&t, 4, buffer.data() + loc);
    loc += 4;
    return true;
  }

  bool i16(int16_t v) override {
    uint16_t t = juce::ByteOrder::bigEndianShort(&v);
    if (loc + 2 > buffer.size()) {
      buffer.resize(loc + 2);
    }
    std::copy_n((char const *)&t, 2, buffer.data() + loc);
    loc += 2;
    return true;
  }

  bool u16(uint16_t v) override {
    uint16_t t = juce::ByteOrder::bigEndianShort(&v);
    if (loc + 2 > buffer.size()) {
      buffer.resize(loc + 2);
    }
    std::copy_n((char const *)&t, 2, buffer.data() + loc);
    loc += 2;
    return true;
  }

  bool u8(uint8_t v) override {
    if (loc + 1 > buffer.size()) {
      buffer.resize(loc + 1);
    }
    buffer[loc] = *(char *)&v;
    loc += 1;
    return true;
  }

  bool write(void *buf, size_t size) override {
    if (loc + size > buffer.size()) {
      buffer.resize(loc + size);
    }
    std::copy_n((char const *)buf, size, buffer.data() + loc);
    loc += size;
    return true;
  }

  bool seek(int64_t l) override {
    if (l >= 0) {
      loc = l;
      return true;
    } else {
      return false;
    }
  }

  std::string data() {
    std::string s;
    s.assign((char const *)buffer.data(), buffer.size());
    return s;
  }

private:
  std::vector<uint8_t> buffer;
  size_t loc = 0;
};

} // namespace ksesh
