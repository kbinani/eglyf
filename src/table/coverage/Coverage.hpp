#pragma once

namespace eglyf {

class Coverage {
public:
  virtual ~Coverage() {}
  virtual Status write(OutputStream &out) const = 0;
  virtual size_t size() const = 0;
  virtual size_t index(uint16_t gid) const = 0;
  virtual size_t count() const = 0;

  static constexpr size_t Npos = -1;
};

} // namespace eglyf
