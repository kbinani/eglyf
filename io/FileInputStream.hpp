#pragma once

namespace ksesh {

class FileInputStream : public InputStream {
public:
  explicit FileInputStream(juce::File file) : s(file) {}

  int64_t i64() override {
    if (!ok()) {
      return 0;
    }
    return s.readInt64BigEndian();
  }

  uint32_t u32() override {
    if (!ok()) {
      return 0;
    }
    int32_t v = s.readIntBigEndian();
    return *(uint32_t *)&v;
  }

  int16_t i16() override {
    if (!ok()) {
      return 0;
    }
    return s.readShortBigEndian();
  }

  uint16_t u16() override {
    if (!ok()) {
      return 0;
    }
    int16_t v = s.readShortBigEndian();
    return *(uint16_t *)&v;
  }

  uint8_t u8() override {
    if (!ok()) {
      return 0;
    }
    char v = s.readByte();
    return *(uint8_t *)&v;
  }

  bool ok() override {
    return s.openedOk() && !s.isExhausted();
  }

  bool read(void *buffer, size_t size) override {
    if (size == 0) {
      return true;
    }
    return s.read(buffer, size) == size;
  }

  bool seek(int64_t loc) override {
    return s.setPosition(loc);
  }

private:
  juce::FileInputStream s;
};

} // namespace ksesh
