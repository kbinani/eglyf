#pragma once

namespace eglyf {

class Coverage {
public:
  virtual ~Coverage() {}
  virtual Status write(OutputStream &out) const = 0;
  virtual size_t size() const = 0;
};

} // namespace eglyf
