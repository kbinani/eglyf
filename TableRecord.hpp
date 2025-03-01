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
    if (!in.u32(&tr.checksum)) {
      return nullopt;
    }
    if (!in.o32(&tr.offset)) {
      return nullopt;
    }
    if (!in.u32(&tr.length)) {
      return nullopt;
    }
    return tr;
  }
};

} // namespace ksesh
