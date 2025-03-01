#pragma once

namespace ksesh::otf {

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

  std::optional<std::string> read(InputStream &in) const {
    using namespace std;
    string buffer;
    buffer.resize(length);
    if (!in.seek(offset)) {
      return nullopt;
    }
    if (length != in.read(buffer.data(), length)) {
      return nullopt;
    }
    return buffer;
  }
};

} // namespace ksesh::otf
