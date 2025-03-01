#pragma once

namespace ksesh {

class InputStream {
public:
  virtual ~InputStream() {}
  virtual bool ok() = 0;
  virtual bool i64(int64_t *) = 0;
  virtual bool u32(uint32_t *) = 0;
  virtual bool i16(int16_t *) = 0;
  virtual bool u16(uint16_t *) = 0;
  virtual bool u8(uint8_t *) = 0;
  virtual bool read(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;

  bool o32(Offset32 *v) {
    return u32(v);
  }
};

} // namespace ksesh
