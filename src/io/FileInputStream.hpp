#pragma once

namespace eglyf {

class FileInputStream : public InputStream {
public:
  explicit FileInputStream(std::filesystem::path file) {
    fp = File::Open(file, File::Mode::Read);
  }

  size_t read(void *buffer, size_t size) override {
    if (File::Fread(buffer, size, 1, fp)) {
      return size;
    } else {
      return 0;
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
