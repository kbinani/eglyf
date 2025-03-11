#pragma once

namespace eglyf {

class Nullopt final {
public:
  std::vector<Status::Where> fTrace;
  std::string fWhat;

  Nullopt(char const *file, int line, char const *what) : fTrace({Status::Where(file, line)}), fWhat(what) {}
  Nullopt(std::vector<Status::Where> const &trace, std::string const &what) : fTrace(trace), fWhat(what) {}

  Nullopt pushed(char const *file, int line) {
    Nullopt r = *this;
    r.fTrace.emplace_back(file, line);
    return r;
  }
};

template <class T>
class Optional {
public:
  Optional(T value) : fStorage(value) {}

  Optional() : fStorage(Status::Where(__FILE__, __LINE__)) {}

  Optional(std::vector<Status::Where> trace, std::string what) : fStorage(Status(Status::ErrorData(trace, what))) {
  }

  Optional(Nullopt null) : fStorage(Status(Status::ErrorData(null.fTrace))) {}

  explicit operator bool() const {
    return fStorage.index() == 0;
  }

  T &operator*() {
    assert(fStorage.index() == 0);
    return std::get<T>(fStorage);
  }

  T *operator->() {
    assert(fStorage.index() == 0);
    return &std::get<T>(fStorage);
  }

  Status status() const {
    if (fStorage.index() == 0) {
      return Status::Ok();
    } else {
      return std::get<Status>(fStorage);
    }
  }

private:
  std::variant<T, Status> fStorage;
};

template <>
Optional<Status>::Optional(Status v) = delete;

} // namespace eglyf

#define _EGLYF_NULLOPT_HELPER(file, line, what) eglyf::Nullopt((file), (line), (what))
#define EGLYF_NULLOPT _EGLYF_NULLOPT_HELPER(__FILE__, __LINE__, "")
#define EGLYF_NULLOPT_WHAT(what) _EGLYF_NULLOPT_HELPER(__FILE__, __LINE__, (what))

#define EGLYF_NULLOPT_PUSH(st) ((st).error() ? eglyf::Nullopt((st).error()->pushed(__FILE__, __LINE__).fTrace, (st).error()->fWhat) : _EGLYF_NULLOPT_HELPER(__FILE__, __LINE__, ""))
