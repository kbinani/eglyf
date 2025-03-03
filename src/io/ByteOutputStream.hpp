#pragma once

namespace eglyf {

class ByteOutputStream : public OutputStream {
public:
  bool write(void const *buf, size_t size) override {
    if (loc + size > buffer.size()) {
      buffer.resize(loc + size);
    }
    std::copy_n((char const *)buf, size, buffer.data() + loc);
    loc += size;
    return true;
  }

  bool seek(int64_t l) override {
    if (l >= 0) {
      loc = l;
      return true;
    } else {
      return false;
    }
  }

  int64_t position() override {
    return loc;
  }

  std::string data() {
    std::string s;
    s.assign((char const *)buffer.data(), buffer.size());
    return s;
  }

  size_t size() const {
    return buffer.size();
  }

private:
  std::vector<uint8_t> buffer;
  size_t loc = 0;
};

} // namespace eglyf
