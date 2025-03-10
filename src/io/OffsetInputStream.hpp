#pragma once

namespace eglyf {

class OffsetInputStream : public InputStream {
public:
  explicit OffsetInputStream(InputStream *up) : upstream(up), offset(up->position()) {
  }

  size_t read(void *buffer, size_t size) override {
    return upstream->read(buffer, size);
  }

  bool seek(int64_t loc) override {
    auto ok = upstream->seek(offset + loc);
    jassert(position() == loc);
    return ok;
  }

  int64_t position() override {
    return upstream->position() - offset;
  }

private:
  int64_t const offset;
  InputStream *const upstream;
};

} // namespace eglyf
