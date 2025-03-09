#pragma once

namespace eglyf {

class FontFile {
public:
  struct TrueTypeOutlines {
    std::shared_ptr<GlyphDataTable> glyf;
  };
  struct CFFOutlines {
  };

public:
  Status write(OutputStream &out) {
    using namespace std;

    map<array<uint8_t, 4>, std::shared_ptr<Table>> all;
    for (auto const &it : tables) {
      all[it.first] = it.second;
    }
    all[FCC("cmap")] = cmap;
    all[FCC("head")] = head;
    all[FCC("hhea")] = hhea;
    all[FCC("maxp")] = maxp;
    all[FCC("name")] = name;
    all[FCC("OS/2")] = os2;
    all[FCC("post")] = post;
    numTables = all.size();
    numTables += 1; // hmtx
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      numTables += 2; // glyf, loca
    }

    if (!out.u32(sfntVersion)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(numTables)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(searchRange)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(entrySelector)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(rangeShift)) {
      return EGLYF_ERROR;
    }

    int64_t const tableRecordLocation = 12;
    int64_t const tableContentLocation = tableRecordLocation + 16 * numTables;

    Offset32 offset = tableContentLocation;
    if (!out.seek(tableContentLocation)) {
      return EGLYF_ERROR;
    }

    map<array<uint8_t, 4>, TableRecord> tableRecords;
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      auto const &o = get<TrueTypeOutlines>(outlines);

      vector<Offset32> offsets;
      auto encodedGlyf = o.glyf->encode(offsets);
      if (!encodedGlyf) {
        return EGLYF_STATUS_PUSH(encodedGlyf.status());
      }
      if (offsets.empty()) {
        return EGLYF_ERROR;
      }

      auto glyfTag = FCC("glyf");
      TableRecord glyfRecord;
      glyfRecord.tag = glyfTag;
      glyfRecord.checksum = encodedGlyf->checksum;
      glyfRecord.offset = offset;
      glyfRecord.length = encodedGlyf->length;
      tableRecords[glyfTag] = glyfRecord;

      if (!out.write(encodedGlyf->data.data(), encodedGlyf->data.size())) {
        return EGLYF_ERROR;
      }
      offset += encodedGlyf->data.size();

      Offset32 maxOffset = offsets.back();
      uint32_t indexToLocationFormat = head->indexToLocFormat;
      if (indexToLocationFormat == 0 && maxOffset / 2 > (Offset32)numeric_limits<Offset16>::max()) {
        indexToLocationFormat = 1;
      }
      IndexToLocationTable loca(indexToLocationFormat);
      loca.offsets.swap(offsets);
      auto locaTag = FCC("loca");
      auto encodedLoca = loca.encode();
      if (!encodedLoca) {
        return EGLYF_STATUS_PUSH(encodedLoca.status());
      }
      TableRecord locaRecord;
      locaRecord.tag = locaTag;
      locaRecord.checksum = encodedLoca->checksum;
      locaRecord.offset = offset;
      locaRecord.length = encodedLoca->length;
      tableRecords[locaTag] = locaRecord;

      if (!out.write(encodedLoca->data.data(), encodedLoca->data.size())) {
        return EGLYF_ERROR;
      }
      offset += encodedLoca->data.size();
    }

    // hmtx
    uint16_t numberOfHMetrics = 0;
    if (auto encoded = hmtx->encode(numberOfHMetrics); encoded) {
      TableRecord hmtxRecord;
      hmtxRecord.tag = FCC("hmtx");
      hmtxRecord.checksum = encoded->checksum;
      hmtxRecord.offset = offset;
      hmtxRecord.length = encoded->length;
      tableRecords[FCC("hmtx")] = hmtxRecord;

      if (!out.write(encoded->data.data(), encoded->data.size())) {
        return EGLYF_ERROR;
      }
      offset += encoded->data.size();

      hhea->numberOfHMetrics = numberOfHMetrics;
    } else {
      return EGLYF_STATUS_PUSH(encoded.status());
    }

    for (auto &[tag, table] : all) {
      auto encoded = table->encode();
      if (!encoded) {
        return EGLYF_STATUS_PUSH(encoded.status());
      }
      TableRecord tr;
      tr.tag = tag;
      tr.checksum = encoded->checksum;
      tr.offset = offset;
      tr.length = encoded->length;
      tableRecords[tag] = tr;

      if (!out.write(encoded->data.data(), encoded->data.size())) {
        return EGLYF_ERROR;
      }
      offset += encoded->data.size();
    }

    if (!out.seek(tableRecordLocation)) {
      return EGLYF_ERROR;
    }
    for (auto [_, record] : tableRecords) {
      if (!out.write(record.tag.data(), record.tag.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u32(record.checksum)) {
        return EGLYF_ERROR;
      }
      if (!out.o32(record.offset)) {
        return EGLYF_ERROR;
      }
      if (!out.u32(record.length)) {
        return EGLYF_ERROR;
      }
    }
    return Status::Ok();
  }

  Optional<uint16_t> addEmptyGlyph(std::string const &name, uint16_t advanceWidth, uint16_t lsb) {
    return addTrueTypeGlyph(name, [&](GlyphDataTable &glyf, HorizontalMetricsTable &hmtx) -> Optional<uint16_t> {
      HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = advanceWidth;
      hm.lsb = lsb;
      hmtx.metrics.push_back(hm);
      return glyf.addEmptyGlyph();
    });
  }

  Optional<uint16_t> addCompositeGlyph(std::string const &name, GlyphDataTable::CompositeGlyph::GlyphRecord child, uint16_t advanceWidth, uint16_t lsb) {
    return addTrueTypeGlyph(name, [&](GlyphDataTable &glyf, HorizontalMetricsTable &hmtx) -> Optional<uint16_t> {
      auto gid = glyf.addCompositeGlyph(child);
      if (!gid) {
        return EGLYF_NULLOPT;
      }
      HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = advanceWidth;
      hm.lsb = lsb;
      hmtx.metrics.push_back(hm);
      return *gid;
    });
  }

  static Status Read(InputStream &in, std::shared_ptr<FontFile> &out) {
    using namespace std;
    auto ff = make_shared<FontFile>();
    auto td = TableDirectory::Read(in);
    if (!td) {
      return EGLYF_ERROR;
    }
    ff->sfntVersion = td->sfntVersion;
    ff->numTables = td->numTables;
    ff->searchRange = td->searchRange;
    ff->entrySelector = td->entrySelector;
    ff->rangeShift = td->rangeShift;

    map<array<uint8_t, 4>, TableRecord> records;
    for (auto const &it : td->tableRecords) {
      records[it.tag] = it;
    }

    if (auto tr = records.find(FCC("head")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = FontHeaderTable::Read(slice, ff->head); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("maxp")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = MaximumProfileTable::Read(slice, ff->maxp); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("cmap")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ff->cmap = make_shared<ReadonlyTable>(*buffer);
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("hhea")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = HorizontalHeaderTable::Read(slice, ff->hhea); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("hmtx")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = HorizontalMetricsTable::Read(slice, ff->maxp->numGlyphs, ff->hhea->numberOfHMetrics, ff->hmtx); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("name")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ff->name = make_shared<ReadonlyTable>(*buffer);
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("OS/2")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = OS2AndWindowsMetricsTable::Read(slice, ff->os2); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("post")); tr == records.end()) {
      return EGLYF_ERROR;
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = PostScriptTable::Read(slice, ff->post); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr->first);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr0 = records.find(FCC("glyf")); tr0 != records.end()) {
      auto tr1 = records.find(FCC("loca"));
      if (tr1 == records.end()) {
        return EGLYF_ERROR;
      }
      shared_ptr<IndexToLocationTable> loca;
      {
        auto buffer = tr1->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR;
        }
        ByteInputStream slice(*buffer);
        if (auto st = IndexToLocationTable::Read(slice, ff->head->indexToLocFormat, ff->maxp->numGlyphs, loca); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      shared_ptr<GlyphDataTable> glyf;
      {
        auto buffer = tr0->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR;
        }
        ByteInputStream slice(*buffer);
        if (auto st = GlyphDataTable::Read(slice, *loca, glyf); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      TrueTypeOutlines o;
      o.glyf = glyf;
      ff->outlines = o;

      records.erase(tr0->first);
      records.erase(tr1->first);
    } else {
      // TODO:
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("GSUB")); tr != records.end()) {
      if (auto buffer = tr->second.read(in); buffer) {
        ByteInputStream slice(*buffer);
        auto result = make_shared<GlyphSubstitutionTable>();
        if (auto st = result->read(slice); st.ok()) {
          ff->tables[tr->second.tag] = result;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
        records.erase(tr->first);
      } else {
        return EGLYF_ERROR;
      }
    }

    for (auto const &it : records) {
      TableRecord tr = it.second;
      auto buffer = tr.read(in);
      if (!buffer) {
        return EGLYF_ERROR;
      }
      ff->tables[tr.tag] = make_shared<ReadonlyTable>(*buffer);
    }

    out.swap(ff);
    return Status::Ok();
  }

private:
  Optional<uint16_t> addTrueTypeGlyph(std::string const &name, std::function<Optional<uint16_t>(GlyphDataTable &glyf, HorizontalMetricsTable &hmtx)> addOp) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return EGLYF_NULLOPT;
    }
    TrueTypeOutlines &tto = get<TrueTypeOutlines>(outlines);

    shared_ptr<GlyphDataTable> nGlyf;
    if (auto st = tto.glyf->clone(nGlyf); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    shared_ptr<PostScriptTable> nPost;
    if (auto st = post->clone(nPost); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    shared_ptr<MaximumProfileTable> nMaxp;
    if (auto st = maxp->clone(nMaxp); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    shared_ptr<HorizontalMetricsTable> nHmtx;
    if (auto st = hmtx->clone(nHmtx); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    shared_ptr<HorizontalHeaderTable> nHhea;
    if (auto st = hhea->clone(nHhea); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    auto gid = addOp(*nGlyf, *nHmtx);
    if (!gid) {
      return EGLYF_NULLOPT;
    }
    if (auto st = nPost->addName(name); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    if (auto st = nGlyf->updateMaxp(*nMaxp); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    tto.glyf = nGlyf;
    post = nPost;
    maxp = nMaxp;
    hmtx = nHmtx;
    hhea = nHhea;
    return *gid;
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

} // namespace eglyf
