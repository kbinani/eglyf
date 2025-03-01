#pragma once

namespace ksesh {

struct TableRecord {
  Tag tag;
  uint32_t checksum;
  Offset32 offset;
  uint32_t length;

  static std::optional<TableRecord> Read(InputStream &in) {
    using namespace std;
    TableRecord tr;
    if (auto tag = Tag::Read(in); tag) {
      tr.tag = *tag;
    } else {
      return nullopt;
    }
    tr.checksum = in.u32();
    tr.offset = in.o32();
    tr.length = in.u32();
    if (in.ok()) {
      return tr;
    } else {
      return nullopt;
    }
  }
};

} // namespace ksesh
