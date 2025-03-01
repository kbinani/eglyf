#pragma once

namespace eglyf::gsub {

class Subtable {
public:
  virtual ~Subtable() {}

  std::shared_ptr<Coverage> coverage;
};

} // namespace eglyf::gsub
