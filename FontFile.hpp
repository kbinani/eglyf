#pragma once

namespace ksesh::otf {

class FontFile {
public:
  struct TrueTypeOutlines {
    std::shared_ptr<GlyphDataTable> glyf;
  };
  struct CFFOutlines {
  };

public:
  bool write(OutputStream &out) {
    using namespace std;

    map<array<uint8_t, 4>, std::shared_ptr<Table>> all;
    for (auto const &it : tables) {
      all[it.first] = it.second;
    }
    all[Tag::FCC("cmap")] = cmap;
    all[Tag::FCC("head")] = head;
    all[Tag::FCC("hhea")] = hhea;
    all[Tag::FCC("hmtx")] = hmtx;
    all[Tag::FCC("maxp")] = maxp;
    all[Tag::FCC("name")] = name;
    all[Tag::FCC("OS/2")] = os2;
    all[Tag::FCC("post")] = post;
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      auto const &o = get<TrueTypeOutlines>(outlines);
      numTables = all.size() + 2; // glyf, loca
    } else {
      numTables = all.size();
    }

    if (!out.u32(sfntVersion)) {
      return false;
    }
    if (!out.u16(numTables)) {
      return false;
    }
    if (!out.u16(searchRange)) {
      return false;
    }
    if (!out.u16(entrySelector)) {
      return false;
    }
    if (!out.u16(rangeShift)) {
      return false;
    }

    int64_t const tableRecordLocation = 12;
    int64_t const tableContentLocation = tableRecordLocation + 16 * numTables;

    Offset32 offset = tableContentLocation;
    if (!out.seek(tableContentLocation)) {
      return false;
    }

    map<array<uint8_t, 4>, TableRecord> tableRecords;
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      auto const &o = get<TrueTypeOutlines>(outlines);

      IndexToLocationTable loca(1);
      auto encodedGlyf = o.glyf->encode(loca);
      if (!encodedGlyf) {
        return false;
      }

      auto glyfTag = Tag::FCC("glyf");
      TableRecord glyfRecord;
      glyfRecord.tag.values = glyfTag;
      glyfRecord.checksum = encodedGlyf->checksum;
      glyfRecord.offset = offset;
      glyfRecord.length = encodedGlyf->length;
      tableRecords[glyfTag] = glyfRecord;

      if (!out.write(encodedGlyf->data.data(), encodedGlyf->data.size())) {
        return false;
      }
      offset += encodedGlyf->data.size();

      auto locaTag = Tag::FCC("loca");
      auto encodedLoca = loca.encode();
      if (!encodedLoca) {
        return false;
      }
      TableRecord locaRecord;
      locaRecord.tag.values = locaTag;
      locaRecord.checksum = encodedLoca->checksum;
      locaRecord.offset = offset;
      locaRecord.length = encodedLoca->length;
      tableRecords[locaTag] = locaRecord;

      if (!out.write(encodedLoca->data.data(), encodedLoca->data.size())) {
        return false;
      }
      offset += encodedLoca->data.size();
    }

    for (auto &[tag, table] : all) {
      auto encoded = table->encode();
      if (!encoded) {
        return false;
      }
      TableRecord tr;
      tr.tag.values = tag;
      tr.checksum = encoded->checksum;
      tr.offset = offset;
      tr.length = encoded->length;
      tableRecords[tag] = tr;

      if (!out.write(encoded->data.data(), encoded->data.size())) {
        return false;
      }
      offset += encoded->data.size();
    }

    if (!out.seek(tableRecordLocation)) {
      return false;
    }
    for (auto [_, record] : tableRecords) {
      if (!out.write(record.tag.values.data(), record.tag.values.size())) {
        return false;
      }
      if (!out.u32(record.checksum)) {
        return false;
      }
      if (!out.o32(record.offset)) {
        return false;
      }
      if (!out.u32(record.length)) {
        return false;
      }
    }
    return true;
  }

  std::optional<uint16_t> addCompositeGlyph(std::string const &name, GlyphDataTable::CompositeGlyph::GlyphRecord child) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return nullopt;
    }
    TrueTypeOutlines &tto = get<TrueTypeOutlines>(outlines);

    auto nGlyph = tto.glyf->clone();
    if (!nGlyph) {
      return nullopt;
    }
    auto nPost = post->clone();
    if (!nPost) {
      return nullopt;
    }
    auto nMaxp = maxp->clone();
    if (!nMaxp) {
      return nullopt;
    }

    auto gid = nGlyph->addCompositeGlyph(child);
    if (!gid) {
      return nullopt;
    }
    if (!nPost->addName(name)) {
      return nullopt;
    }
    nMaxp->numGlyphs = nGlyph->glyphs.size();

    tto.glyf = nGlyph;
    post = nPost;
    maxp = nMaxp;
    return *gid;
  }

  static std::shared_ptr<FontFile> Read(InputStream &in) {
    using namespace std;
    auto ff = make_shared<FontFile>();
    auto td = TableDirectory::Read(in);
    if (!td) {
      return nullptr;
    }
    ff->sfntVersion = td->sfntVersion;
    ff->numTables = td->numTables;
    ff->searchRange = td->searchRange;
    ff->entrySelector = td->entrySelector;
    ff->rangeShift = td->rangeShift;

    map<array<uint8_t, 4>, TableRecord> records;
    for (auto const &it : td->tableRecords) {
      records[it.tag.values] = it;
    }

    if (auto tr = records.find(Tag::FCC("head")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = FontHeaderTable::Read(slice); result) {
        ff->head = result;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("maxp")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = MaximumProfileTable::Read(slice); result) {
        ff->maxp = result;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("cmap")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ff->cmap = make_shared<ReadonlyTable>(*buffer);
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("hhea")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = HorizontalHeaderTable::Read(slice); result) {
        ff->hhea = result;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("hmtx")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = HorizontalMetricsTable::Read(slice, ff->maxp->numGlyphs, ff->hhea->numberOfHMetrics); result) {
        ff->hmtx = result;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("name")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ff->name = make_shared<ReadonlyTable>(*buffer);
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("OS/2")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = OS2AndWindowsMetricsTable::Read(slice); result) {
        ff->os2 = result;
      } else {
        return nullptr;
      }
      records.erase(tr->first);
    } else {
      return nullptr;
    }

    if (auto tr = records.find(Tag::FCC("post")); tr == records.end()) {
      return nullptr;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto result = PostScriptTable::Read(slice); result) {
        ff->post = result;
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
        loca = IndexToLocationTable::Read(slice, ff->head->indexToLocFormat, ff->maxp->numGlyphs);
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
      TrueTypeOutlines o;
      o.glyf = glyf;
      ff->outlines = o;

      records.erase(tr0->first);
      records.erase(tr1->first);
    } else {
      // TODO:
      return nullptr;
    }

    for (auto const &it : records) {
      TableRecord tr = it.second;
      auto buffer = tr.read(in);
      if (!buffer) {
        return nullptr;
      }
      ff->tables[tr.tag.values] = make_shared<ReadonlyTable>(*buffer);
    }
    return ff;
  }

public:
  uint32_t sfntVersion;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;

  std::shared_ptr<ReadonlyTable> cmap;
  std::shared_ptr<FontHeaderTable> head;
  std::shared_ptr<HorizontalHeaderTable> hhea;
  std::shared_ptr<HorizontalMetricsTable> hmtx;
  std::shared_ptr<MaximumProfileTable> maxp;
  std::shared_ptr<ReadonlyTable> name;
  std::shared_ptr<OS2AndWindowsMetricsTable> os2;
  std::shared_ptr<PostScriptTable> post;

  std::variant<TrueTypeOutlines, CFFOutlines> outlines;

  std::map<std::array<uint8_t, 4>, std::shared_ptr<Table>> tables;
};

} // namespace ksesh::otf
