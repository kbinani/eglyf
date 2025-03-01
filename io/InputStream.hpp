#pragma once

namespace ksesh::otf {

class InputStream {
public:
  virtual ~InputStream() {}

  virtual size_t read(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;

  bool i64(int64_t *x) {
    int64_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    *x = juce::ByteOrder::bigEndianInt64(&v);
    return true;
  }

  bool u32(uint32_t *x) {
    uint32_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    int32_t w = juce::ByteOrder::bigEndianInt(&v);
    *x = *(uint32_t *)&w;
    return true;
  }

  bool i16(int16_t *x) {
    uint16_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    *x = juce::ByteOrder::bigEndianShort(&v);
    return true;
  }

  bool u16(uint16_t *x) {
    uint16_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    int16_t w = juce::ByteOrder::bigEndianShort(&v);
    *x = *(uint16_t *)&w;
    return true;
  }

  bool i8(int8_t *x) {
    return read(x, 1);
  }

  bool u8(uint8_t *x) {
    return read(x, 1);
  }

  bool o16(Offset16 *v) {
    return u16(v);
  }

  bool o32(Offset32 *v) {
    return u32(v);
  }

  std::string readUntilEos() {
    using namespace std;
    string ret;
    string chunk;
    chunk.resize(512);
    while (true) {
      size_t rd = read(chunk.data(), chunk.size());
      if (rd == 0) {
        break;
      }
      ret.append(chunk.c_str(), rd);
      if (rd < chunk.size()) {
        break;
      }
    }
    return ret;
  }
};

} // namespace ksesh::otf
