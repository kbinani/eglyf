#pragma once

namespace eglyf::cmap {

class CmapSubtable {
public:
  virtual ~CmapSubtable() {}
  virtual Status write(OutputStream &out) const = 0;
};

} // namespace eglyf::cmap
