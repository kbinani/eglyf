#pragma once

namespace ksesh::otf {

class OutputStream {
public:
  virtual ~OutputStream() {}

  virtual bool write(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;

  bool i64(int64_t v) {
    int64_t x = juce::ByteOrder::bigEndianInt64(&v);
    return write(&x, sizeof(x));
  }

  bool u32(uint32_t v) {
    int32_t x = juce::ByteOrder::bigEndianInt(&v);
    return write(&x, sizeof(x));
  }

  bool i16(int16_t v) {
    int16_t x = juce::ByteOrder::bigEndianShort(&v);
    return write(&x, sizeof(x));
  }

  bool u16(uint16_t v) {
    int16_t x = juce::ByteOrder::bigEndianShort(&v);
    return write(&x, sizeof(v));
  }

  bool i8(uint8_t v) {
    return write(&v, sizeof(v));
  }

  bool u8(uint8_t v) {
    return write(&v, sizeof(v));
  }

  bool o16(Offset16 v) {
    return u16(v);
  }

  bool o32(Offset32 v) {
    return u32(v);
  }

  bool f2dot14(F2DOT14 v) {
    return i16(v.data);
  }
};

} // namespace ksesh::otf
