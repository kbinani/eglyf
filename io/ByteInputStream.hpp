#pragma once

namespace ksesh {

class ByteInputStream : public InputStream {
public:
  explicit ByteInputStream(std::string_view buffer) : buffer(buffer) {
  }

  bool i64(int64_t *x) override {
    if (pos + 8 <= buffer.size()) {
      uint64_t v = juce::ByteOrder::bigEndianInt64(buffer.data() + pos);
      pos += 8;
      *x = *(int64_t *)&v;
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool u32(uint32_t *x) override {
    if (pos + 4 <= buffer.size()) {
      *x = juce::ByteOrder::bigEndianInt(buffer.data() + pos);
      pos += 4;
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool i16(int16_t *x) override {
    if (pos + 2 <= buffer.size()) {
      uint16_t v = juce::ByteOrder::bigEndianShort(buffer.data() + pos);
      pos += 2;
      *x = *(int16_t *)&v;
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool u16(uint16_t *x) override {
    if (pos + 2 <= buffer.size()) {
      *x = juce::ByteOrder::bigEndianShort(buffer.data() + pos);
      pos += 2;
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool u8(uint8_t *x) override {
    if (pos + 1 <= buffer.size()) {
      char v = buffer[pos];
      pos += 1;
      *x = *(uint8_t *)&v;
      return true;
    } else {
      pos = buffer.size();
      return false;
    }
  }

  bool read(void *buf, size_t size) override {
    if (pos + size <= buffer.size()) {
      pos += size;
      std::copy_n(buffer.data() + pos, size, (char *)buf);
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
  std::string_view buffer;
};

} // namespace ksesh
