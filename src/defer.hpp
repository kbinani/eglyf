#pragma once

#include <functional>

#if defined(defer)
#error "'defer' macro already defined"
#endif

namespace ksesh {
class defer_t final {
public:
  template <class T>
  defer_t(T fn) : fn(fn) {}
  ~defer_t() {
    if (!fn) {
      return;
    }
    try {
      fn();
    } catch (...) {
    }
    fn = nullptr;
  }
  defer_t(defer_t const &) = delete;
  defer_t &operator=(defer_t const &) = delete;
  defer_t(defer_t &&) = delete;
  defer_t &operator=(defer_t &&) = delete;

private:
  std::function<void()> fn;
};
} // namespace ksesh

#define ksesh_defer_helper2(line) ksesh_defer_##line
#define ksesh_defer_helper(line) ksesh_defer_helper2(line)

#define defer ::ksesh::defer_t const ksesh_defer_helper(__LINE__) = [&]() -> void
