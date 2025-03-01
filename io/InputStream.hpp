#pragma once

namespace ksesh {

  class InputStream {
public:
  virtual ~InputStream() {}
  virtual bool ok() = 0;
  virtual int64_t i64() = 0;
  virtual uint32_t u32() = 0;
  virtual int16_t i16() = 0;
  virtual uint16_t u16() = 0;
  virtual uint8_t u8() = 0;
  virtual bool read(void *buffer, size_t size) = 0;
  virtual bool seek(int64_t loc) = 0;

  Offset32 o32() {
    return u32();
  }
};

}
