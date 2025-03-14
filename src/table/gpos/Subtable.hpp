#pragma once

namespace eglyf::gpos {

class Subtable {
public:
  virtual ~Subtable() {}
  virtual Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) = 0;
  virtual size_t size() const = 0;

  std::shared_ptr<Coverage> coverage;
};

} // namespace eglyf::gpos
