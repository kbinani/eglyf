#pragma once

namespace ksesh {

class ByteInputStream : public InputStream {
public:
  explicit ByteInputStream(std::string const &buffer) : buffer(buffer) {
  }

  int64_t i64() override {
    if (pos + 8 <= buffer.size()) {
      uint64_t v = juce::ByteOrder::bigEndianInt64(buffer.c_str() + pos);
      pos += 8;
      return *(int64_t *)&v;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  uint32_t u32() override {
    if (pos + 4 <= buffer.size()) {
      uint32_t v = juce::ByteOrder::bigEndianInt(buffer.c_str() + pos);
      pos += 4;
      return v;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  int16_t i16() override {
    if (pos + 2 <= buffer.size()) {
      uint16_t v = juce::ByteOrder::bigEndianShort(buffer.c_str() + pos);
      pos += 2;
      return *(int16_t *)&v;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  uint16_t u16() override {
    if (pos + 2 <= buffer.size()) {
      uint16_t v = juce::ByteOrder::bigEndianShort(buffer.c_str() + pos);
      pos += 2;
      return v;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  uint8_t u8() override {
    if (pos + 1 <= buffer.size()) {
      char v = buffer[pos];
      pos += 1;
      return *(uint8_t *)&v;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  bool read(void *buf, size_t size) override {
    if (pos + size <= buffer.size()) {
      pos += size;
      std::copy_n(buffer.c_str() + pos, size, (char *)buf);
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool seek(int64_t loc) override {
    if (0 <= pos + loc && pos + loc <= buffer.size()) {
      pos = loc;
      return true;
    } else {
      return false;
    }
  }

  bool ok() override {
    return pos < buffer.size();
  }

private:
  size_t pos = 0;
  std::string buffer;
};

} // namespace ksesh
