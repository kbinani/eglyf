#pragma once

namespace eglyf {

class FileOutputStream : public OutputStream {
public:
  explicit FileOutputStream(juce::File file) : s(file) {
    if (!s.setPosition(0)) {
      return;
    }
    s.truncate();
  }

  bool write(void const *buffer, size_t size) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.write(buffer, size);
  }

  bool seek(int64_t loc) override {
    if (s.failedToOpen()) {
      return false;
    }
    return s.setPosition(loc);
  }

private:
  juce::FileOutputStream s;
};

} // namespace eglyf
