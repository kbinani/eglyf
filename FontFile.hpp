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

    map<array<uint8_t, 4>, TableRecord> records;
    for (auto const &it : ff->tableDirectory.tableRecords) {
      records[it.tag.values] = it;
    }

    shared_ptr<FontHeaderTable> head;
    if (auto tr = records.find(Tag::FCC("head")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = FontHeaderTable::Read(slice); result) {
        head = result;
        ff->tables[tr->first] = head;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    shared_ptr<MaximumProfileTable> maxp;
    if (auto tr = records.find(Tag::FCC("maxp")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = MaximumProfileTable::Read(slice); result) {
        maxp = result;
        ff->tables[tr->first] = maxp;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr0 = records.find(Tag::FCC("glyf")); tr0 != records.end()) {
      auto tr1 = records.find(Tag::FCC("loca"));
      if (tr1 == records.end()) {
        return nullptr;
      }
      shared_ptr<IndexToLocationTable> loca;
      {
        auto buffer = tr1->second.read(in);
        if (!buffer) {
          return nullptr;
        }
        ByteInputStream slice(*buffer);
        loca = IndexToLocationTable::Read(slice, head->indexToLocFormat, maxp->numGlyphs);
        if (!loca) {
          return nullptr;
        }
      }
      shared_ptr<GlyphDataTable> glyf;
      {
        auto buffer = tr0->second.read(in);
        if (!buffer) {
          return nullptr;
        }
        ByteInputStream slice(*buffer);
        glyf = GlyphDataTable::Read(slice, *loca);
        if (!glyf) {
          return nullptr;
        }
      }
      ff->tables[tr0->first] = glyf;
      ff->tables[tr1->first] = loca;
      records.erase(tr0->first);
      records.erase(tr1->first);
    }

    for (auto const &it : records) {
      TableRecord tr = it.second;
      auto buffer = tr.read(in);
      if (!buffer) {
        return nullptr;
      }
      string s = *buffer;
      if (s.size() % 4 != 0) {
        s.resize(s.size() + 4 - (s.size() % 4));
      }
      ff->tables[tr.tag.values] = make_shared<ReadonlyTable>(s, tr.length);
    }
    return ff;
  }
};

} // namespace ksesh
