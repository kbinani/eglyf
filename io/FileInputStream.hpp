#pragma once

namespace ksesh {

class FileInputStream : public InputStream {
public:
  explicit FileInputStream(juce::File file) : s(file) {}

  bool i64(int64_t *v) override {
    if (s.openedOk() && s.getPosition() + 8 <= s.getTotalLength()) {
      *v = s.readInt64BigEndian();
      return s.openedOk();
    } else {
      return false;
    }
  }

  bool u32(uint32_t *x) override {
    if (s.openedOk() && s.getPosition() + 4 <= s.getTotalLength()) {
      int32_t v = s.readIntBigEndian();
      *x = *(uint32_t *)&v;
      return s.openedOk();
    } else {
      return false;
    }
  }

  bool i16(int16_t *x) override {
    if (s.openedOk() && s.getPosition() + 2 <= s.getTotalLength()) {
      *x = s.readShortBigEndian();
      return s.openedOk();
    } else {
      return false;
    }
  }

  bool u16(uint16_t *x) override {
    if (s.openedOk() && s.getPosition() + 2 <= s.getTotalLength()) {
      int16_t v = s.readShortBigEndian();
      *x = *(uint16_t *)&v;
      return s.openedOk();
    } else {
      return false;
    }
  }

  bool u8(uint8_t *x) override {
    if (s.openedOk() && s.getPosition() + 1 <= s.getTotalLength()) {
      char v = s.readByte();
      *x = *(uint8_t *)&v;
      return s.openedOk();
    } else {
      return false;
    }
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
