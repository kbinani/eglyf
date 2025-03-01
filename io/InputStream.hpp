#pragma once

namespace ksesh {

class InputStream {
public:
  virtual ~InputStream() {}

  virtual bool i64(int64_t *) = 0;
  virtual bool u32(uint32_t *) = 0;
  virtual bool i16(int16_t *) = 0;
  virtual bool u16(uint16_t *) = 0;
  virtual bool u8(uint8_t *) = 0;
  virtual size_t read(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;

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

} // namespace ksesh
