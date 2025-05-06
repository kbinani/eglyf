#pragma once

namespace eglyf {

class File {
  File() = delete;

public:
  enum class Mode {
    Read,
    Write,
    ReadWrite,
    Append,
  };

  static FILE *Open(std::string const &path, Mode mode) = delete;

  static FILE *Open(std::filesystem::path const &path, Mode mode) {
#if defined(_WIN32)
    std::wstring m = ModeWString(mode);
    FILE *ret = nullptr;
    if (_wfopen_s(&ret, path.c_str(), m.c_str()) == 0) {
      return ret;
    }
    if (path.native().starts_with(LR"(\\?\)")) {
      return nullptr;
    }
    auto p = path;
    p.make_preferred();
    std::wstring s = LR"(\\?\)" + p.native();
    if (_wfopen_s(&ret, s.c_str(), m.c_str()) == 0) {
      return ret;
    } else {
      return nullptr;
    }
#else
    auto m = ModeString(mode);
    return fopen(path.c_str(), m.c_str());
#endif
  }

  [[nodiscard]] static bool Fseek(FILE *file, int64_t pos, int origin) {
    if (!file) {
      return false;
    }
#if defined(_WIN32)
    return _fseeki64(file, pos, origin) == 0;
#else
    return fseeko(file, pos, origin) == 0;
#endif
  }

  static std::optional<int64_t> Ftell(FILE *file) {
    using namespace std;
    if (!file) {
      return nullopt;
    }
#if defined(_WIN32)
    auto ret = _ftelli64(file);
#else
    auto ret = ftello(file);
#endif
    if (ret < 0) {
      return nullopt;
    } else {
      return ret;
    }
  }

  [[nodiscard]] static bool Fwrite(void const *buffer, size_t elementSize, size_t elementCount, FILE *stream) {
    if (!stream) {
      return false;
    }
    if (elementSize == 0 || elementCount == 0) {
      return true;
    }
    return fwrite(buffer, elementSize, elementCount, stream) == elementCount;
  }

  [[nodiscard]] static bool Fread(void *buffer, size_t elementSize, size_t elementCount, FILE *stream) {
    if (!stream) {
      return false;
    }
    if (elementSize == 0 || elementCount == 0) {
      return true;
    }
    return fread(buffer, elementSize, elementCount, stream) == elementCount;
  }

  [[nodiscard]] static bool Ftruncate(FILE *stream, int64_t size) {
    if (!stream) {
      return false;
    }
#if defined(_MSC_VER)
    return _chsize_s(_fileno(stream), size) == 0;
#else
    return ftruncate(fileno(stream), size) == 0;
#endif
  }

private:
  static std::string ModeString(Mode mode) {
    switch (mode) {
    case Mode::Read:
      return "rb";
    case Mode::Write:
      return "wb";
    case Mode::Append:
      return "ab";
    case Mode::ReadWrite:
      return "rb+";
    }
    return "rb";
  }

  static std::wstring ModeWString(Mode mode) {
    switch (mode) {
    case Mode::Read:
      return L"rb";
    case Mode::Write:
      return L"wb";
    case Mode::Append:
      return L"ab";
    case Mode::ReadWrite:
      return L"rb+";
    }
    return L"rb";
  }
};

} // namespace eglyf
