#pragma once

namespace eglyf {

class InputStream {
public:
  virtual ~InputStream() {}

  virtual size_t read(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;
  virtual int64_t position() = 0;

  bool i64(int64_t *x) {
    uint64_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    uint64_t w = BigEndian(v);
    *x = *(int64_t *)&w;
    return true;
  }

  bool u32(uint32_t *x) {
    uint32_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    *x = BigEndian(v);
    return true;
  }

  bool i16(int16_t *x) {
    uint16_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    uint16_t w = BigEndian(v);
    *x = *(int16_t *)&w;
    return true;
  }

  bool u16(uint16_t *x) {
    uint16_t v;
    auto size = read(&v, sizeof(v));
    if (size < sizeof(v)) {
      return false;
    }
    *x = BigEndian(v);
    return true;
  }

  bool i8(int8_t *x) {
    return read(x, 1) == 1;
  }

  bool u8(uint8_t *x) {
    return read(x, 1) == 1;
  }

  bool o16(Offset16 *v) {
    return u16(v);
  }

  bool o32(Offset32 *v) {
    return u32(v);
  }

  bool f2dot14(F2DOT14 *v) {
    return i16(&v->data);
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

  bool u16a(std::vector<uint16_t> &out, size_t count) {
    out.clear();
    out.reserve(count);
    for (size_t i = 0; i < count; i++) {
      uint16_t v;
      if (!u16(&v)) {
        return false;
      }
      out.push_back(v);
    }
    return true;
  }

  bool i16a(std::vector<int16_t> &out, size_t count) {
    out.clear();
    out.reserve(count);
    for (size_t i = 0; i < count; i++) {
      int16_t v;
      if (!i16(&v)) {
        return false;
      }
      out.push_back(v);
    }
    return true;
  }

  bool o16a(std::vector<Offset16> &out, size_t count) {
    return u16a(out, count);
  }

  bool u32a(std::vector<uint32_t> &out, size_t count) {
    out.clear();
    out.reserve(count);
    for (size_t i = 0; i < count; i++) {
      uint32_t v;
      if (!u32(&v)) {
        return false;
      }
      out.push_back(v);
    }
    return true;
  }

  bool o32a(std::vector<Offset32> &out, size_t count) {
    return u32a(out, count);
  }
};

} // namespace eglyf
