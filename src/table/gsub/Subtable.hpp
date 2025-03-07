#pragma once

namespace eglyf::gsub {

class Subtable {
public:
  virtual ~Subtable() {}
  virtual bool write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) = 0;

  std::shared_ptr<Coverage> coverage;
};

} // namespace eglyf::gsub
