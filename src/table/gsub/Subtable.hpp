#pragma once

namespace eglyf::gsub {

class Subtable {
public:
  virtual ~Subtable() {}
  virtual bool write(OutputStream &out) = 0;

  std::shared_ptr<Coverage> coverage;
};

} // namespace eglyf::gsub
