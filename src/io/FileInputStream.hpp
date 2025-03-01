#pragma once

namespace ksesh::otf {

class FileInputStream : public InputStream {
public:
  explicit FileInputStream(juce::File file) : s(file) {}

  size_t read(void *buffer, size_t size) override {
    if (size == 0) {
      return 0;
    }
    int ret = s.read(buffer, size);
    if (ret < 0) {
      return 0;
    } else {
      return (size_t)ret;
    }
  }

  bool seek(int64_t loc) override {
    return s.setPosition(loc);
  }

private:
  juce::FileInputStream s;
};

} // namespace ksesh::otf
