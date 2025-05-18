#pragma once

namespace eglyf {

class FileInputStream : public InputStream {
public:
  explicit FileInputStream(std::filesystem::path file) {
    fp = File::Open(file, File::Mode::Read);
  }

  size_t read(void *buffer, size_t size) override {
    if (!fp) {
      return 0;
    }
    if (size == 0) {
      return 0;
    }
    int count = fread(buffer, 1, size, fp);
    if (count < 0) {
      return 0;
    } else {
      return count;
    }
  }

  bool seek(int64_t loc) override {
    return File::Fseek(fp, loc, SEEK_SET);
  }

  int64_t position() override {
    auto p = File::Ftell(fp);
    if (p) {
      return *p;
    } else {
      return 0;
    }
  }

private:
  FILE *fp = nullptr;
};

} // namespace eglyf
