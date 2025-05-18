#pragma once

namespace eglyf {

class Font {
public:
  struct TrueTypeOutlines {
    std::shared_ptr<glyf::GlyphDataTable> glyf;
  };
  struct CFFOutlines {
    std::shared_ptr<cff::CompactFontFormatTable> cff;
  };

public:
  Status write(OutputStream &out) {
    using namespace std;

    if (holds_alternative<post::PostScriptTable::Version2Data>(post->data) || holds_alternative<post::PostScriptTable::ReadonlyVersion2Data>(post->data)) {
      post::PostScriptTable::Version2Data next;
      for (uint16_t gid = 0; gid < names.size(); gid++) {
        if (auto id = next.addName(names[gid]); !id) {
          return EGLYF_STATUS_PUSH(id.status());
        }
      }

      post->data = next;
    }

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
    if (vhea) {
      all[FCC("vhea")] = vhea;
    }
    if (vmtx) {
      all[FCC("vmtx")] = vmtx;
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
    uint16_t searchRange = pow(2, (int)floor(log2(numTables))) * 16;
    if (!out.u16(searchRange)) {
      return EGLYF_ERROR_WHAT("Failed to write searchRange");
    }
    uint16_t entrySelector = (uint16_t)floor(log2(numTables));
    if (!out.u16(entrySelector)) {
      return EGLYF_ERROR_WHAT("Failed to write entrySelector");
    }
    uint16_t rangeShift = (numTables * 16) - searchRange;
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
        indexToLocationFormat = loca::IndexToLocationTable::RecommendFormat(offsets);
      }
      head->indexToLocFormat = indexToLocationFormat;
      loca::IndexToLocationTable loca(indexToLocationFormat);
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
      auto vmtx = dynamic_pointer_cast<vmtx::VerticalMetricsTable>(vmtxEntry->second);
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
      auto vhea = dynamic_pointer_cast<vhea::VerticalHeaderTable>(vheaEntry->second);
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

    auto now = Now();
    head->modified = now;

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

  Optional<uint16_t> addEmptyGlyph(std::string const &name,
                                   gdef::GlyphDefinitionTable::Class classValue,
                                   uint16_t advanceWidth,
                                   int16_t lsb,
                                   uint16_t advanceHeight,
                                   int16_t tsb) {
    return addTrueTypeGlyph(name, classValue, [&](glyf::GlyphDataTable &glyf, hmtx::HorizontalMetricsTable &hmtx, vmtx::VerticalMetricsTable &vmtx) -> Optional<uint16_t> {
      auto gid = hmtx.metrics.size();

      hmtx::HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = advanceWidth;
      hm.lsb = lsb;
      hmtx.metrics.push_back(hm);

      vmtx.set(gid, advanceHeight, tsb);

      return glyf.addEmptyGlyph();
    });
  }

  Optional<uint16_t> addCompositeGlyph(std::string const &name,
                                       gdef::GlyphDefinitionTable::Class classValue,
                                       glyf::GlyphDataTable::CompositeGlyph::GlyphRecord const &child,
                                       uint16_t advanceWidth,
                                       int16_t lsb,
                                       uint16_t advanceHeight,
                                       int16_t tsb) {
    using namespace std;
    vector<glyf::GlyphDataTable::CompositeGlyph::GlyphRecord> children;
    children.push_back(child);
    auto gid = addCompositeGlyph(name, classValue, children, advanceWidth, lsb, advanceHeight, tsb);
    if (gid) {
      return *gid;
    } else {
      return EGLYF_NULLOPT_PUSH(gid.status());
    }
  }

  Optional<uint16_t> addCompositeGlyph(std::string const &name,
                                       gdef::GlyphDefinitionTable::Class classValue,
                                       std::vector<glyf::GlyphDataTable::CompositeGlyph::GlyphRecord> const &children,
                                       uint16_t advanceWidth,
                                       int16_t lsb,
                                       uint16_t advanceHeight,
                                       int16_t tsb) {
    using namespace std;
    return addTrueTypeGlyph(name, classValue, [&](glyf::GlyphDataTable &glyf, hmtx::HorizontalMetricsTable &hmtx, vmtx::VerticalMetricsTable &vmtx) -> Optional<uint16_t> {
      auto gid = glyf.addCompositeGlyph(children, *maxp);
      if (!gid) {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }

      hmtx::HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = advanceWidth;
      hm.lsb = lsb;
      hmtx.metrics.push_back(hm);

      vmtx.set(*gid, advanceHeight, tsb);

      return *gid;
    });
  }

  Optional<uint16_t> addSimpleGlyph(std::string const &name,
                                    gdef::GlyphDefinitionTable::Class classValue,
                                    std::vector<glyf::GlyphDataTable::Contour> const &contours,
                                    uint16_t advanceWidth,
                                    int16_t lsb,
                                    uint16_t advanceHeight,
                                    int16_t tsb) {
    using namespace std;
    return addTrueTypeGlyph(name, classValue, [&](glyf::GlyphDataTable &glyf, hmtx::HorizontalMetricsTable &hmtx, vmtx::VerticalMetricsTable &vmtx) -> Optional<uint16_t> {
      auto gid = glyf.addSimpleGlyph(contours, *maxp);
      if (!gid) {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }

      hmtx::HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = advanceWidth;
      hm.lsb = lsb;
      hmtx.metrics.push_back(hm);

      vmtx.set(*gid, advanceHeight, tsb);

      return *gid;
    });
  }

  Status replaceCompositeGlyph(uint16_t gid,
                               gdef::GlyphDefinitionTable::Class classValue,
                               std::vector<glyf::GlyphDataTable::CompositeGlyph::GlyphRecord> const &children,
                               uint16_t advanceWidth,
                               int16_t lsb,
                               uint16_t advanceHeight,
                               int16_t tsb) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<TrueTypeOutlines>(outlines).glyf;

    if (auto st = glyf->replaceOutline(gid, children, *maxp); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    hmtx::HorizontalMetricsTable::LongHorMetric hm;
    hm.advanceWidth = advanceWidth;
    hm.lsb = lsb;
    hmtx->metrics[gid] = hm;

    auto vmtx = ensureVmtx();
    vmtx->set(gid, advanceHeight, tsb);

    if (auto st = setGlyphClass(gid, classValue); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return Status::Ok();
  }

  Optional<uint16_t> replaceCompositeGlyphByName(std::string const &name,
                                                 gdef::GlyphDefinitionTable::Class classValue,
                                                 std::vector<glyf::GlyphDataTable::CompositeGlyph::GlyphRecord> const &children,
                                                 uint16_t advanceWidth,
                                                 int16_t lsb,
                                                 uint16_t advanceHeight,
                                                 int16_t tsb) {
    auto gid = postGetGlyphID(name);
    if (!gid) {
      return EGLYF_NULLOPT;
    }
    if (auto st = replaceCompositeGlyph(*gid, classValue, children, advanceWidth, lsb, advanceHeight, tsb); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    return *gid;
  }

  Optional<uint16_t> replaceCompositeGlyphByName(std::string const &name,
                                                 gdef::GlyphDefinitionTable::Class classValue,
                                                 glyf::GlyphDataTable::CompositeGlyph::GlyphRecord const &child,
                                                 uint16_t advanceWidth,
                                                 int16_t lsb,
                                                 uint16_t advanceHeight,
                                                 int16_t tsb) {
    using namespace std;
    vector<glyf::GlyphDataTable::CompositeGlyph::GlyphRecord> children;
    children.push_back(child);
    auto gid = replaceCompositeGlyphByName(name, classValue, children, advanceWidth, lsb, advanceHeight, tsb);
    if (gid) {
      return *gid;
    } else {
      return EGLYF_NULLOPT_PUSH(gid.status());
    }
  }

  Status replaceSimpleGlyph(uint16_t gid,
                            gdef::GlyphDefinitionTable::Class classValue,
                            std::vector<glyf::GlyphDataTable::Contour> const &contours,
                            uint16_t advanceWidth,
                            int16_t lsb,
                            uint16_t advanceHeight,
                            int16_t tsb) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<TrueTypeOutlines>(outlines).glyf;

    if (auto st = glyf->replaceOutline(gid, contours, *maxp); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    hmtx::HorizontalMetricsTable::LongHorMetric hm;
    hm.advanceWidth = advanceWidth;
    hm.lsb = lsb;
    hmtx->metrics[gid] = hm;

    auto vmtx = ensureVmtx();
    vmtx->set(gid, advanceHeight, tsb);

    if (auto st = setGlyphClass(gid, classValue); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return Status::Ok();
  }

  Optional<uint16_t> replaceSimpleGlyphByName(std::string const &name,
                                              gdef::GlyphDefinitionTable::Class classValue,
                                              std::vector<glyf::GlyphDataTable::Contour> const &contours,
                                              uint16_t advanceWidth,
                                              int16_t lsb,
                                              uint16_t advanceHeight,
                                              int16_t tsb) {
    using namespace std;
    auto gid = postGetGlyphID(name);
    if (!gid) {
      return EGLYF_NULLOPT;
    }
    auto st = replaceSimpleGlyph(*gid, classValue, contours, advanceWidth, lsb, advanceHeight, tsb);
    if (st.ok()) {
      return *gid;
    } else {
      return EGLYF_NULLOPT_PUSH(st);
    }
  }

  Status setGlyphClass(uint16_t gid, gdef::GlyphDefinitionTable::Class c) {
    using namespace std;
    if (!gdef) {
      gdef = make_shared<gdef::GlyphDefinitionTable>();
      gdef->majorVersion = 1;
      gdef->minorVersion = 2;
    }
    if (!gdef->glyphClassDef) {
      gdef->glyphClassDef = make_shared<ClassDef>();
    }
    if (auto st = gdef->glyphClassDef->add(gid, static_cast<uint16_t>(c)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  static Status Read(InputStream &in, std::shared_ptr<Font> &out) {
    using namespace std;
    auto ff = make_shared<Font>();
    auto td = TableDirectory::Read(in);
    if (!td) {
      return EGLYF_STATUS_PUSH(td.status());
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
      if (auto st = head::FontHeaderTable::Read(slice, ff->head); !st.ok()) {
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
      if (auto st = maxp::MaximumProfileTable::Read(slice, ff->maxp); !st.ok()) {
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
      if (auto st = hhea::HorizontalHeaderTable::Read(slice, ff->hhea); !st.ok()) {
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
      if (auto st = hmtx::HorizontalMetricsTable::Read(slice, ff->maxp->numGlyphs, ff->hhea->numberOfHMetrics, ff->hmtx); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read hmtx table");
    }

    if (auto tr = records.find(FCC("name")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("name table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = name::NamingTable::Read(slice, ff->name); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      records.erase(tr);
    } else {
      return EGLYF_ERROR_WHAT("Failed to read name table");
    }

    if (auto tr = records.find(FCC("OS/2")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("OS/2 table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = os2::OS2AndWindowsMetricsTable::Read(slice, ff->os2); !st.ok()) {
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
      if (auto st = post::PostScriptTable::Read(slice, ff->post); !st.ok()) {
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
      shared_ptr<loca::IndexToLocationTable> loca;
      {
        auto buffer = tr1->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR_WHAT("Failed to read loca table");
        }
        ByteInputStream slice(*buffer);
        if (auto st = loca::IndexToLocationTable::Read(slice, ff->head->indexToLocFormat, ff->maxp->numGlyphs, loca); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      shared_ptr<glyf::GlyphDataTable> glyf;
      {
        auto buffer = tr0->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR_WHAT("Failed to read glyf table");
        }
        ByteInputStream slice(*buffer);
        if (auto st = glyf::GlyphDataTable::Read(slice, *loca, glyf); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      TrueTypeOutlines o;
      o.glyf = glyf;
      ff->outlines = o;

      records.erase(tr0);
      records.erase(tr1);
    } else if (auto trCff = records.find(FCC("CFF ")); trCff != records.end()) {
      shared_ptr<cff::CompactFontFormatTable> cff;
      {
        auto buffer = trCff->second.read(in);
        if (!buffer) {
          return EGLYF_ERROR;
        }
        ByteInputStream slice(*buffer);
        if (auto st = cff::CompactFontFormatTable::Read(slice, cff); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      CFFOutlines o;
      o.cff = cff;
      ff->outlines = o;

      records.erase(trCff);
    } else {
      return EGLYF_ERROR;
    }

    if (auto tr = records.find(FCC("GSUB")); tr != records.end()) {
      if (auto buffer = tr->second.read(in); buffer) {
        ByteInputStream slice(*buffer);
        ff->gsub = make_shared<gsub::GlyphSubstitutionTable>();
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
        ff->gpos = make_shared<gpos::GlyphPositioningTable>();
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
        if (auto st = gdef::GlyphDefinitionTable::Read(slice, ff->gdef); !st.ok()) {
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

      shared_ptr<vhea::VerticalHeaderTable> vhea;
      {
        auto vheaBuffer = vheaRecord->second.read(in);
        if (!vheaBuffer) {
          return EGLYF_ERROR_WHAT("Failed to read vhea table");
        }
        ByteInputStream vheaSlice(*vheaBuffer);
        if (auto st = vhea::VerticalHeaderTable::Read(vheaSlice, vhea); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }

      shared_ptr<vmtx::VerticalMetricsTable> vmtx;
      {
        auto vmtxBuffer = vmtxRecord->second.read(in);
        if (!vmtxBuffer) {
          return EGLYF_ERROR_WHAT("Failed to read vmtx table");
        }
        ByteInputStream vmtxSlice(*vmtxBuffer);
        if (auto st = vmtx::VerticalMetricsTable::Read(vmtxSlice, ff->maxp->numGlyphs, vhea->numOfLongVerMetrics(), vmtx); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }

      ff->vhea = vhea;
      ff->vmtx = vmtx;
      records.erase(vheaRecord);
      records.erase(vmtxRecord);
    }

    if (auto tr = records.find(FCC("cmap")); tr == records.end()) {
      return EGLYF_ERROR_WHAT("cmap table not found");
    } else if (auto buffer = tr->second.read(in); buffer) {
      ByteInputStream slice(*buffer);
      if (auto st = cmap::CharacterToGlyphIndexMappingTable::Read(slice, ff->cmap); !st.ok()) {
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

    auto const &table = post::PostScriptTable::OSXPostScriptNames();
    if (holds_alternative<post::PostScriptTable::Version2Data>(ff->post->data)) {
      auto const &d = get<post::PostScriptTable::Version2Data>(ff->post->data);
      for (size_t i = 0; i < d.names.size(); i++) {
        auto entry = d.names[i];
        if (holds_alternative<string>(entry)) {
          string name = get<string>(entry);
          ff->nameLookupTable[name] = i;
          ff->names.push_back(name);
        } else {
          uint16_t index = get<uint16_t>(entry);
          if (index < table.size()) {
            string name = table[index];
            ff->nameLookupTable[name] = i;
            ff->names.push_back(name);
          } else {
            break;
          }
        }
      }
    } else if (holds_alternative<post::PostScriptTable::ReadonlyVersion2Data>(ff->post->data)) {
      auto const &d = get<post::PostScriptTable::ReadonlyVersion2Data>(ff->post->data);
      for (size_t i = 0; i < d.glyphNameIndex.size(); i++) {
        uint16_t index = d.glyphNameIndex[i];
        if (index < 258) {
          if (index >= table.size()) {
            break;
          }
          string name = table[index];
          ff->nameLookupTable[name] = i;
          ff->names.push_back(name);
        } else {
          auto idx = index - 258;
          if (idx >= d.nameStrings.size()) {
            break;
          }
          string name = d.nameStrings[idx];
          ff->nameLookupTable[name] = i;
          ff->names.push_back(name);
        }
      }
    } else {
      for (uint16_t gid = 0; gid < ff->maxp->numGlyphs; gid++) {
        string name = format("gid{}", gid);
        ff->names.push_back(name);
        ff->nameLookupTable[name] = gid;
      }
    }

    out.swap(ff);
    return Status::Ok();
  }

  std::optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    if (auto gid = cmap->getGlyphID(codepoint); gid) {
      return *gid;
    } else {
      return nullopt;
    }
  }

  std::optional<uint16_t> postGetGlyphID(std::string const &name) const {
    using namespace std;
    if (auto found = nameLookupTable.find(name); found != nameLookupTable.end()) {
      return found->second;
    } else {
      return nullopt;
    }
  }

  Optional<uint16_t> postAddName(std::string const &name) {
    using namespace std;
    uint16_t gid = names.size();
    names.push_back(name);
    nameLookupTable[name] = gid;
    return gid;
  }

  Optional<std::string> postGetName(uint16_t glyphID) const {
    if (glyphID >= names.size()) {
      return EGLYF_NULLOPT;
    }
    return names[glyphID];
  }

  Status postSetName(uint16_t glyphID, std::string const &name) {
    using namespace std;
    if (glyphID >= names.size()) {
      return EGLYF_ERROR;
    }
    string current = names[glyphID];
    if (current == name) {
      return Status::Ok();
    }
    names[glyphID] = name;
    nameLookupTable.erase(current);
    nameLookupTable[name] = glyphID;
    return Status::Ok();
  }

  std::shared_ptr<vmtx::VerticalMetricsTable> ensureVmtx() {
    using namespace std;
    if (!vhea) {
      vhea = make_shared<vhea::VerticalHeaderTable>();
      vhea::VerticalHeaderTable::Data11 data;
      data.caretSlopeRise = 0;
      data.caretSlopeRun = 1;
      data.caretOffset = 0;
      data.advanceHeightMax = 0;
      data.minTopSideBearing = 0;
      data.minBottomSideBearing = 0;
      data.yMaxExtent = 0;
      data.metricDataFormat = 0;
      data.vertTypoLineGap = 0;
      data.vertTypoAscender = 0;
      data.vertTypoDescender = 0;
      data.numOfLongVerMetrics = 0;
      vhea->data = data;
    }
    if (!vmtx) {
      vmtx = make_shared<vmtx::VerticalMetricsTable>();
    }
    return vmtx;
  }

private:
  Optional<uint16_t> addTrueTypeGlyph(std::string const &name,
                                      gdef::GlyphDefinitionTable::Class classValue,
                                      std::function<Optional<uint16_t>(glyf::GlyphDataTable &glyf, hmtx::HorizontalMetricsTable &hmtx, vmtx::VerticalMetricsTable &vmtx)> addOp) {
    using namespace std;
    if (!holds_alternative<TrueTypeOutlines>(outlines)) {
      return EGLYF_NULLOPT_WHAT("TrueType outlines not available");
    }
    TrueTypeOutlines &tto = get<TrueTypeOutlines>(outlines);
    auto vmtx = ensureVmtx();
    auto gid = addOp(*tto.glyf, *hmtx, *vmtx);
    if (!gid) {
      return EGLYF_NULLOPT_PUSH(gid.status());
    }
    if (auto postGid = postAddName(name); postGid) {
      if (*gid != *postGid) {
        return EGLYF_NULLOPT_WHAT("Glyph ID mismatch with post table");
      }
    } else {
      return EGLYF_NULLOPT_PUSH(postGid.status());
    }
    if (auto st = setGlyphClass(*gid, classValue); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    maxp->numGlyphs = tto.glyf->glyphs.size();
    return *gid;
  }

public:
  uint32_t sfntVersion;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;

  // Required tables
  std::shared_ptr<cmap::CharacterToGlyphIndexMappingTable> cmap;
  std::shared_ptr<head::FontHeaderTable> head;
  std::shared_ptr<hhea::HorizontalHeaderTable> hhea;
  std::shared_ptr<hmtx::HorizontalMetricsTable> hmtx;
  std::shared_ptr<maxp::MaximumProfileTable> maxp;
  std::shared_ptr<name::NamingTable> name;
  std::shared_ptr<os2::OS2AndWindowsMetricsTable> os2;
  std::shared_ptr<post::PostScriptTable> post;

  std::variant<TrueTypeOutlines, CFFOutlines> outlines;

  // Optional tables
  std::shared_ptr<gpos::GlyphPositioningTable> gpos;
  std::shared_ptr<gsub::GlyphSubstitutionTable> gsub;
  std::shared_ptr<gdef::GlyphDefinitionTable> gdef;
  std::shared_ptr<vhea::VerticalHeaderTable> vhea;
  std::shared_ptr<vmtx::VerticalMetricsTable> vmtx;

  std::map<std::array<uint8_t, 4>, std::shared_ptr<Table>> tables;

private:
  std::vector<std::string> names;
  std::unordered_map<std::string, uint16_t> nameLookupTable;
};

} // namespace eglyf
