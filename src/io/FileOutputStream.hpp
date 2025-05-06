#pragma once

namespace eglyf {

class FileOutputStream : public OutputStream {
public:
  explicit FileOutputStream(std::filesystem::path file) {
    fp = File::Open(file, File::Mode::Write);
  }

  bool write(void const *buffer, size_t size) override {
    return File::Fwrite(buffer, size, 1, fp);
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
