#pragma once

namespace ksesh {

class FontFile {
public:
  TableDirectory tableDirectory;
  std::map<std::array<uint8_t, 4>, std::shared_ptr<Table>> tables;

  bool write(OutputStream &out) {
    using namespace std;
    if (!out.u32(tableDirectory.sfntVersion)) {
      return false;
    }
    if (!out.u16(tableDirectory.numTables)) {
      return false;
    }
    if (!out.u16(tableDirectory.searchRange)) {
      return false;
    }
    if (!out.u16(tableDirectory.entrySelector)) {
      return false;
    }
    if (!out.u16(tableDirectory.rangeShift)) {
      return false;
    }
    Offset32 const start = 12 + 16 * tableDirectory.numTables;
    Offset32 offset = start;
    vector<string> tableContents;
    for (TableRecord const &tr : tableDirectory.tableRecords) {
      if (!out.write((void *)tr.tag.values.data(), tr.tag.values.size())) {
        return false;
      }
      auto item = tables.find(tr.tag.values);
      if (item == tables.end()) {
        return false;
      }
      auto table = item->second;
      auto encoded = table->encode();
      if (!encoded) {
        return false;
      }
      uint32_t length = encoded->length;
      tableContents.push_back(encoded->data);
      if (encoded->data.size() < length) {
        return false;
      }
      if (encoded->data.size() % 4 != 0) {
        return false;
      }
      auto checksum = Table::Checksum(encoded->data);
      if (!checksum) {
        return false;
      }
      if (!out.u32(*checksum)) {
        return false;
      }
      if (!out.o32(offset)) {
        return false;
      }
      offset += encoded->data.size();
      if (!out.u32(length)) {
        return false;
      }
    }
    if (!out.seek(start)) {
      return false;
    }
    for (auto const &data : tableContents) {
      if (!out.write((void *)data.c_str(), data.size())) {
        return false;
      }
    }
    return true;
  }

  static std::shared_ptr<FontFile> Read(InputStream &in) {
    using namespace std;
    auto ff = make_shared<FontFile>();
    if (auto td = TableDirectory::Read(in); td) {
      ff->tableDirectory = *td;
    } else {
      return nullptr;
    }
    for (uint32_t i = 0; i < ff->tableDirectory.numTables; i++) {
      TableRecord tr = ff->tableDirectory.tableRecords[i];
      uint32_t size = tr.length;
      if (size % 4 != 0) {
        size += 4 - (tr.length % 4);
      }
      string buffer;
      buffer.resize(size);
      if (!in.seek(tr.offset)) {
        return nullptr;
      }
      if (!in.read(buffer.data(), tr.length)) {
        return nullptr;
      }
      if (tr.tag.values == Tag::FCC("head")) {
        ByteInputStream slice(buffer);
        auto head = FontHeaderTable::Read(slice);
        if (!head) {
          return nullptr;
        }
        ff->tables[tr.tag.values] = head;
      } else {
        ff->tables[tr.tag.values] = make_shared<ReadonlyTable>(buffer, tr.length);
      }
    }
    return ff;
  }
};

} // namespace ksesh
