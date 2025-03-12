#pragma once

namespace eglyf {

class Status {
public:
  struct Where {
    std::string fFile;
    int fLine;

    Where(char const *file, int line) {
      namespace fs = std::filesystem;
      static fs::path const sProjectRoot(fs::path(__FILE__).parent_path().parent_path());
      std::error_code ec;
      fs::path path(file ? file : "(unknown)");
      fs::path p = fs::relative(path, sProjectRoot, ec);
      if (ec) {
        fFile = path.filename().string();
        fLine = line;
      } else {
        fFile = p.string();
        fLine = line;
      }
    }

    template <class T>
    void print(T &out) const {
      using namespace std;
      out << fFile << ":" << fLine << endl;
    }
  };

  struct ErrorData {
    std::vector<Where> fTrace;
    std::string fWhat;

    explicit ErrorData(Where where, std::string what = {}) : fTrace({where}), fWhat(what) {}
    explicit ErrorData(std::vector<Where> trace, std::string what = {}) : fTrace(trace), fWhat(what) {}

    ErrorData pushed(char const *file, int line) const {
      ErrorData d = *this;
      d.fTrace = fTrace;
      d.fTrace.emplace_back(file, line);
      d.fWhat = fWhat;
      return d;
    }
  };

  explicit Status(std::optional<ErrorData> error) : fError(error) {
  }

  Status() : fError(std::nullopt) {}

  bool ok() const {
    return !fError;
  }

  std::optional<ErrorData> error() const {
    return fError;
  }

  Status pushed(char const *file, int line) const {
    if (fError) {
      ErrorData err = fError->pushed(file, line);
      return Status(err);
    } else {
      return *this;
    }
  }

  static Status Ok() {
    return Status();
  }

  static Status Error(char const *file, int line, std::string const &what) {
    return Status(ErrorData(Where(file, line), what));
  }

  static void Merge(Status const &from, Status &to) {
    if (!to.ok()) {
      return;
    }
    if (!from.ok()) {
      to = from;
    } else {
      to = Status::Ok();
    }
  }

  template <class T>
  void print(T &out) const {
    using namespace std;
    if (!fError) {
      return;
    }
    if (!fError->fWhat.empty()) {
      out << "what: " << fError->fWhat << endl;
    }
    cout << "trace: " << endl;
    for (auto const &where : fError->fTrace) {
      where.print(out);
    }
  }

  class Error {
    Error() = delete;

  public:
    static std::string UnexpectedFeatureParamsOffset() {
      return "Unexpected feature params offset";
    }
  };

private:
  std::optional<ErrorData> fError;
};

} // namespace eglyf

#define _EGLYF_ERROR_HELPER(file, line, what) eglyf::Status::Error((file), (line), (what))
#define EGLYF_ERROR _EGLYF_ERROR_HELPER(__FILE__, __LINE__, std::string())
#define EGLYF_ERROR_WHAT(what) _EGLYF_ERROR_HELPER(__FILE__, __LINE__, (what))

#define EGLYF_ERROR_ERRNO _EGLYF_ERROR_HELPER(__FILE__, __LINE__, Errno::StringFromErrno(errno))

#define _EGLYF_STATUS_PUSH_HELPER(base, file, line) (base).pushed((file), (line))
#define EGLYF_STATUS_PUSH(base) _EGLYF_STATUS_PUSH_HELPER((base), __FILE__, __LINE__)
