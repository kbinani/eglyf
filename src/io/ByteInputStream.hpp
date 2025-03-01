#pragma once

namespace eglyf {

class ByteInputStream : public InputStream {
public:
  explicit ByteInputStream(std::string_view buffer) : buffer(buffer) {
  }

  size_t read(void *buf, size_t size) override {
    if (pos < buffer.size()) {
      size_t ret = std::min(size, buffer.size() - pos);
      std::copy_n(buffer.data() + pos, ret, (char *)buf);
      pos += ret;
      return ret;
    } else {
      pos = buffer.size();
      return 0;
    }
  }

  bool seek(int64_t loc) override {
    if (0 <= loc && loc <= buffer.size()) {
      pos = loc;
      return true;
    } else {
      return false;
    }
  }

  int64_t position() override {
    return pos;
  }

private:
  size_t pos = 0;
  std::string_view buffer;
};

} // namespace eglyf
