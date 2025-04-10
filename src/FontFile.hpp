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
    if (gdef) {
      all[FCC("GDEF")] = gdef;
    }
    if (gpos) {
      all[FCC("GPOS")] = gpos;
    }
    if (gsub) {
      all[FCC("GSUB")] = gsub;
    }
    numTables = all.size();
    numTables += 1; // hmtx
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      numTables += 2; // glyf, loca
    }

    if (!out.u32(sfntVersion)) {
      return EGLYF_ERROR_WHAT("Failed to write sfntVersion");
    }
    if (!out.u16(numTables)) {
      return EGLYF_ERROR_WHAT("Failed to write numTables");
    }
    if (!out.u16(searchRange)) {
      return EGLYF_ERROR_WHAT("Failed to write searchRange");
    }
    if (!out.u16(entrySelector)) {
      return EGLYF_ERROR_WHAT("Failed to write entrySelector");
    }
    if (!out.u16(rangeShift)) {
      return EGLYF_ERROR_WHAT("Failed to write rangeShift");
    }

    int64_t const tableRecordLocation = 12;
    int64_t const tableContentLocation = tableRecordLocation + 16 * numTables;

    Offset32 offset = tableContentLocation;
    if (!out.seek(tableContentLocation)) {
      return EGLYF_ERROR_WHAT("Failed to seek to table content location");
    }

    map<array<uint8_t, 4>, TableRecord> tableRecords;
    if (holds_alternative<TrueTypeOutlines>(outlines)) {
      auto const &o = get<TrueTypeOutlines>(outlines);

      vector<Offset32> offsets;
      auto encodedGlyf = o.glyf->encode(offsets, head->indexToLocFormat == 0 ? 2 : 1);
      if (!encodedGlyf) {
        return EGLYF_STATUS_PUSH(encodedGlyf.status());
      }
      if (offsets.empty()) {
        return EGLYF_ERROR_WHAT("Glyph offsets are empty");
      }

      auto glyfTag = FCC("glyf");
      TableRecord glyfRecord;
      glyfRecord.tag = glyfTag;
      glyfRecord.checksum = encodedGlyf->checksum;
      glyfRecord.offset = offset;
      glyfRecord.length = encodedGlyf->length;
      tableRecords[glyfTag] = glyfRecord;

      if (!out.write(encodedGlyf->data.data(), encodedGlyf->data.size())) {
        return EGLYF_ERROR_WHAT("Failed to write glyf table data");
      }
      offset += encodedGlyf->data.size();

      Offset32 maxOffset = offsets.back();
      uint32_t indexToLocationFormat = head->indexToLocFormat;
      if (indexToLocationFormat == 0) {
        indexToLocationFormat = IndexToLocationTable::RecommendFormat(offsets);
      }
      head->indexToLocFormat = indexToLocationFormat;
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
        return EGLYF_ERROR_WHAT("Failed to write loca table data");
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
        return EGLYF_ERROR_WHAT("Failed to write hmtx table data");
      }
      offset += encoded->data.size();

      hhea->numberOfHMetrics = numberOfHMetrics;
    } else {
      return EGLYF_STATUS_PUSH(encoded.status());
    }

    if (auto vmtxEntry = all.find(FCC("vmtx")); vmtxEntry != all.end()) {
      auto vmtx = dynamic_pointer_cast<VerticalMetricsTable>(vmtxEntry->second);
      if (!vmtx) {
        return EGLYF_ERROR_WHAT("Failed to cast vmtx to VerticalMetricsTable");
      }
      uint16_t numOfLongVerMetrics = 0;
      auto vmtxEncoded = vmtx->encode(numOfLongVerMetrics);
      if (!vmtxEncoded) {
        return EGLYF_STATUS_PUSH(vmtxEncoded.status());
      }
      TableRecord vmtxRecord;
      vmtxRecord.tag = FCC("vmtx");
      vmtxRecord.checksum = vmtxEncoded->checksum;
      vmtxRecord.offset = offset;
      vmtxRecord.length = vmtxEncoded->length;
      tableRecords[FCC("vmtx")] = vmtxRecord;
      if (!out.write(vmtxEncoded->data.data(), vmtxEncoded->data.size())) {
        return EGLYF_ERROR_WHAT("Failed to write vmtx table data");
      }
      offset += vmtxEncoded->data.size();

      auto vheaEntry = all.find(FCC("vhea"));
      if (vheaEntry == all.end()) {
        return EGLYF_ERROR_WHAT("vhea entry not found");
      }
      auto vhea = dynamic_pointer_cast<VerticalHeaderTable>(vheaEntry->second);
      if (!vhea) {
        return EGLYF_ERROR_WHAT("Failed to cast vhea to VerticalHeaderTable");
      }
      vhea->setNumOfLongVerMetrics(numOfLongVerMetrics);

      auto vheaEncoded = vhea->encode();
      if (!vheaEncoded) {
        return EGLYF_STATUS_PUSH(vheaEncoded.status());
      }
      TableRecord vheaRecord;
      vheaRecord.tag = FCC("vhea");
      vheaRecord.checksum = vheaEncoded->checksum;
      vheaRecord.offset = offset;
      vheaRecord.length = vheaEncoded->length;
      tableRecords[FCC("vhea")] = vheaRecord;
      if (!out.write(vheaEncoded->data.data(), vheaEncoded->data.size())) {
        return EGLYF_ERROR_WHAT("Failed to write vhea table data");
      }
      offset += vheaEncoded->data.size();

      all.erase(vheaEntry);
      all.erase(vmtxEntry);
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
        return EGLYF_ERROR_WHAT("Failed to write table data");
      }
      offset += encoded->data.size();
    }

    if (!out.seek(tableRecordLocation)) {
      return EGLYF_ERROR_WHAT("Failed to seek to table record location");
    }
    for (auto [_, record] : tableRecords) {
      if (!out.write(record.tag.data(), record.tag.size())) {
        return EGLYF_ERROR_WHAT("Failed to write table tag");
      }
      if (!out.u32(record.checksum)) {
        return EGLYF_ERROR_WHAT("Failed to write table checksum");
      }
      if (!out.o32(record.offset)) {
        return EGLYF_ERROR_WHAT("Failed to write table offset");
      }
      if (!out.u32(record.length)) {
        return EGLYF_ERROR_WHAT("Failed to write table length");
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
        return EGLYF_NULLOPT_WHAT("Failed to add composite glyph");
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
      return EGLYF_ERROR_WHAT("Failed to read table directory");
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
      return EGLYF_ERROR_WHAT("head table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = FontHeaderTable::Read(slice, ff->head); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read head table");
    }

    if (auto tr = records.find(FCC("maxp")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("maxp table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = MaximumProfileTable::Read(slice, ff->maxp); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read maxp table");
    }

    if (auto tr = records.find(FCC("hhea")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("hhea table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = HorizontalHeaderTable::Read(slice, ff->hhea); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read hhea table");
    }

    if (auto tr = records.find(FCC("hmtx")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("hmtx table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = HorizontalMetricsTable::Read(slice, ff->maxp->numGlyphs, ff->hhea->numberOfHMetrics, ff->hmtx); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read hmtx table");
    }

    if (auto tr = records.find(FCC("name")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("name table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ff->name = make_shared<ReadonlyTable>(*buffer);
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read name table");
    }

    if (auto tr = records.find(FCC("OS/2")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("OS/2 table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = OS2AndWindowsMetricsTable::Read(slice, ff->os2); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read OS/2 table");
    }

    if (auto tr = records.find(FCC("post")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("post table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = PostScriptTable::Read(slice, ff->post); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read post table");
    }

    if (auto tr0 = records.find(FCC("glyf")); tr0 != records.end()) {
      auto tr1 = records.find(FCC("loca"));
      if (tr1 == records.end()) {
        return EGLYF_ERROR_WHAT("loca table not found");
      }
      shared_ptr<IndexToLocationTable> loca;
      {
        auto buffer = tr1->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR_WHAT("Failed to read loca table");
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
          return EGLYF_ERROR_WHAT("Failed to read glyf table");
        }
        ByteInputStream slice(*buffer);
        if (auto st = GlyphDataTable::Read(slice, *loca, glyf); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      TrueTypeOutlines o;
      o.glyf = glyf;
      ff->outlines = o;

      records.erase(tr0);
      records.erase(tr1);
    } else {
      // TODO: CFF outlines not supported yet
      return EGLYF_ERROR_WHAT("CFF outlines not supported yet");
    }

    if (auto tr = records.find(FCC("GSUB")); tr != records.end()) {
      if (auto buffer = tr->second.read(in); buffer) {
        ByteInputStream slice(*buffer);
        ff->gsub = make_shared<GlyphSubstitutionTable>();
        if (auto st = ff->gsub->read(slice); !st.ok()) {
          if (st.error()->fWhat == Status::Error::UnexpectedFeatureParamsOffset()) {
            ff->gsub = nullptr;
            ff->tables[FCC("GSUB")] = make_shared<ReadonlyTable>(*buffer);
          } else {
            return EGLYF_STATUS_PUSH(st);
          }
        }
        records.erase(tr);
      } else {
        return EGLYF_ERROR_WHAT("Failed to read GSUB table");
      }
    }

    if (auto tr = records.find(FCC("GPOS")); tr != records.end()) {
      if (auto buffer = tr->second.read(in); buffer) {
        ByteInputStream slice(*buffer);
        ff->gpos = make_shared<GlyphPositioningTable>();
        if (auto st = ff->gpos->read(slice); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        records.erase(tr);
      } else {
        return EGLYF_ERROR_WHAT("Failed to read GPOS table");
      }
    }

    if (auto tr = records.find(FCC("GDEF")); tr != records.end()) {
      if (auto buffer = tr->second.read(in); buffer) {
        ByteInputStream slice(*buffer);
        if (auto st = GlyphDefinitionTable::Read(slice, ff->gdef); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        records.erase(tr);
      } else {
        return EGLYF_ERROR_WHAT("Failed to read GDEF table");
      }
    }

    if (auto vmtxRecord = records.find(FCC("vmtx")); vmtxRecord != records.end()) {
      auto vheaRecord = records.find(FCC("vhea"));
      if (vheaRecord == records.end()) {
        return EGLYF_ERROR_WHAT("vhea table not found");
      }

      shared_ptr<VerticalHeaderTable> vhea;
      {
        auto vheaBuffer = vheaRecord->second.read(in);
        if (!vheaBuffer) {
          return EGLYF_ERROR_WHAT("Failed to read vhea table");
        }
        ByteInputStream vheaSlice(*vheaBuffer);
        if (auto st = VerticalHeaderTable::Read(vheaSlice, vhea); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }

      shared_ptr<VerticalMetricsTable> vmtx;
      {
        auto vmtxBuffer = vmtxRecord->second.read(in);
        if (!vmtxBuffer) {
          return EGLYF_ERROR_WHAT("Failed to read vmtx table");
        }
        ByteInputStream vmtxSlice(*vmtxBuffer);
        if (auto st = VerticalMetricsTable::Read(vmtxSlice, ff->maxp->numGlyphs, vhea->numOfLongVerMetrics(), vmtx); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }

      ff->tables[vheaRecord->second.tag] = vhea;
      ff->tables[vmtxRecord->second.tag] = vmtx;
      records.erase(vheaRecord);
      records.erase(vmtxRecord);
    }

    if (auto tr = records.find(FCC("cmap")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("cmap table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = CharacterToGlyphIndexMappingTable::Read(slice, ff->cmap); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read cmap table");
    }

    for (auto const &it : records) {
      TableRecord tr = it.second;
      auto buffer = tr.read(in);
      if (!buffer) {
        return EGLYF_ERROR_WHAT("Failed to read table");
      }
      ff->tables[tr.tag] = make_shared<ReadonlyTable>(*buffer);
    }

    out.swap(ff);
    return Status::Ok();
  }

  Optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    if (auto gid = cmap->getGlyphId(codepoint); gid) {
      return *gid;
    } else {
      return EGLYF_NULLOPT_PUSH(gid.status());
    }
  }

private:
  Optional<uint16_t> addTrueTypeGlyph(std::string const &name, std::function<Optional<uint16_t>(GlyphDataTable &glyf, HorizontalMetricsTable &hmtx)> addOp) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return EGLYF_NULLOPT_WHAT("TrueType outlines not available");
    }
    TrueTypeOutlines &tto = get<TrueTypeOutlines>(outlines);

    auto gid = addOp(*tto.glyf, *hmtx);
    if (!gid) {
      return EGLYF_NULLOPT_WHAT("Failed to add glyph");
    }
    if (auto postGid = post->addName(name); postGid) {
      if (*gid != *postGid) {
        return EGLYF_NULLOPT_WHAT("Glyph ID mismatch with post table");
      }
    } else {
      return EGLYF_NULLOPT_PUSH(postGid.status());
    }
    if (auto st = tto.glyf->updateMaxp(*maxp); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    return *gid;
  }

public:
  uint32_t sfntVersion;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;

  // Required tables
  std::shared_ptr<CharacterToGlyphIndexMappingTable> cmap;
  std::shared_ptr<FontHeaderTable> head;
  std::shared_ptr<HorizontalHeaderTable> hhea;
  std::shared_ptr<HorizontalMetricsTable> hmtx;
  std::shared_ptr<MaximumProfileTable> maxp;
  std::shared_ptr<ReadonlyTable> name;
  std::shared_ptr<OS2AndWindowsMetricsTable> os2;
  std::shared_ptr<PostScriptTable> post;

  std::variant<TrueTypeOutlines, CFFOutlines> outlines;

  // Optional tables
  std::shared_ptr<GlyphPositioningTable> gpos;
  std::shared_ptr<GlyphSubstitutionTable> gsub;
  std::shared_ptr<GlyphDefinitionTable> gdef;

  std::map<std::array<uint8_t, 4>, std::shared_ptr<Table>> tables;
};

} // namespace eglyf
