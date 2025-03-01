#pragma once

namespace ksesh {

class FileOutputStream : public OutputStream {
public:
  explicit FileOutputStream(juce::File file) : s(file) {
    if (!s.setPosition(0)) {
      return;
    }
    s.truncate();
  }

  bool i64(int64_t v) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.writeInt64BigEndian(v);
  }

  bool u32(uint32_t v) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.writeIntBigEndian(*(int32_t *)&v);
  }

  bool i16(int16_t v) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.writeShortBigEndian(v);
  }

  bool u16(uint16_t v) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.writeShortBigEndian(*(int16_t *)&v);
  }

  bool u8(uint8_t v) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.writeByte(*(char *)&v);
  }

  bool write(void *buffer, size_t size) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.write(buffer, size);
  }

  bool seek(int64_t loc) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.setPosition(loc);
  }

private:
  juce::FileOutputStream s;
};

} // namespace ksesh
