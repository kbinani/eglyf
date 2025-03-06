#pragma once

namespace eglyf {

class Coverage {
public:
  virtual ~Coverage() {}
  virtual bool write(OutputStream &out) = 0;
};

} // namespace eglyf
