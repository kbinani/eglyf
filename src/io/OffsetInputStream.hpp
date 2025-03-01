#pragma once

namespace eglyf {

class OffsetInputStream : public InputStream {
public:
  explicit OffsetInputStream(InputStream &upstream) : upstream(upstream), offset(upstream.position()) {}

  size_t read(void *buffer, size_t size) override {
    return upstream.read(buffer, size);
  }

  bool seek(int64_t loc) override {
    return upstream.seek(offset + loc);
  }

  int64_t position() override {
    return upstream.position() - offset;
  }

private:
  int64_t const offset;
  InputStream &upstream;
};

} // namespace eglyf
