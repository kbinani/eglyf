#pragma once

namespace eglyf::cmap {

// 'cmap'
class CharacterToGlyphIndexMappingTable : public Table {
public:
  struct EncodingRecord {
    EncodingRecord() = default;
    EncodingRecord(uint16_t platformID, uint16_t encodingID, std::shared_ptr<cmap::CmapSubtable> const &subtable) : platformID(platformID), encodingID(encodingID), subtable(subtable) {}

    uint16_t platformID;
    uint16_t encodingID;
    std::shared_ptr<cmap::CmapSubtable> subtable;
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<CharacterToGlyphIndexMappingTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR_WHAT("Failed to read format");
    }
    if (format != 0) {
      return EGLYF_ERROR_WHAT("Invalid format, expected 0");
    }
    uint16_t numTables;
    if (!in.u16(&numTables)) {
      return EGLYF_ERROR_WHAT("Failed to read numTables");
    }
    struct Record {
      uint16_t platformID;
      uint16_t encodingID;
      Offset32 subtableOffset;
    };
    vector<Record> records;
    for (uint16_t i = 0; i < numTables; i++) {
      Record r;
      if (!in.u16(&r.platformID)) {
        return EGLYF_ERROR_WHAT("Failed to read platformID");
      }
      if (!in.u16(&r.encodingID)) {
        return EGLYF_ERROR_WHAT("Failed to read encodingID");
      }
      if (!in.o32(&r.subtableOffset)) {
        return EGLYF_ERROR_WHAT("Failed to read subtableOffset");
      }
      records.push_back(r);
    }
    std::map<Offset32, shared_ptr<cmap::CmapSubtable>> tables;
    auto ret = make_unique<CharacterToGlyphIndexMappingTable>();
    vector<EncodingRecord> encodingRecords;
    for (auto const &record : records) {
      EncodingRecord r;
      r.platformID = record.platformID;
      r.encodingID = record.encodingID;
      if (auto found = tables.find(record.subtableOffset); found != tables.end()) {
        r.subtable = found->second;
        encodingRecords.push_back(r);
        continue;
      }
      if (!in.seek(record.subtableOffset)) {
        return EGLYF_ERROR_WHAT("Failed to seek to subtable offset");
      }
      OffsetInputStream sub(&in);
      uint16_t fmt;
      if (!sub.u16(&fmt)) {
        return EGLYF_ERROR_WHAT("Failed to read subtable format");
      }
      switch (fmt) {
      case 0: {
        shared_ptr<cmap::ByteEncoding> subtable;
        if (auto st = cmap::ByteEncoding::Read(sub, subtable); st.ok()) {
          r.subtable = subtable;
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      case 2:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 2");
      case 4: {
        shared_ptr<cmap::SegmentMappingToDeltaValues> subtable;
        if (auto st = cmap::SegmentMappingToDeltaValues::Read(sub, subtable); st.ok()) {
          r.subtable = subtable;
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      case 6: {
        shared_ptr<cmap::TrimmedTableMapping> subtable;
        if (auto st = cmap::TrimmedTableMapping::Read(sub, subtable); st.ok()) {
          r.subtable = subtable;
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      case 8:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 8");
      case 10:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 10");
      case 12: {
        shared_ptr<cmap::SegmentedCoverage> subtable;
        if (auto st = cmap::SegmentedCoverage::Read(sub, subtable); st.ok()) {
          r.subtable = subtable;
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      case 13:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 13");
      case 14: {
        shared_ptr<cmap::UnicodeVariationSequences> subtable;
        if (auto st = cmap::UnicodeVariationSequences::Read(sub, subtable); st.ok()) {
          r.subtable = subtable;
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      default:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: " + std::to_string(fmt));
      }
      encodingRecords.push_back(r);
      tables[record.subtableOffset] = r.subtable;
    }

    static auto const NumBits = [](shared_ptr<cmap::CmapSubtable> const &subtable) {
      if (dynamic_pointer_cast<SegmentMappingToDeltaValues>(subtable)) {
        return 16;
      } else if (dynamic_pointer_cast<SegmentedCoverage>(subtable)) {
        return 32;
      } else if (dynamic_pointer_cast<TrimmedTableMapping>(subtable)) {
        return 16;
      } else {
        return -1;
      }
    };

    vector<EncodingRecord> sorted;
    set<shared_ptr<cmap::CmapSubtable>> added;
    for (auto const &r : encodingRecords) {
      switch (r.platformID) {
      case 0:
        break;
      case 3:
        switch (r.encodingID) {
        case 1:
        case 10:
          break;
        default:
          continue;
        }
        break;
      default:
        continue;
      }
      if (added.find(r.subtable) != added.end()) {
        continue;
      }
      sorted.push_back(r);
      added.insert(r.subtable);
    }

    ranges::sort(sorted, [](auto const &a, auto const &b) {
      int bitsA = NumBits(a.subtable);
      int bitsB = NumBits(b.subtable);
      return bitsA < bitsB;
    });
    for (auto const &s : sorted) {
      if (auto format4 = dynamic_pointer_cast<SegmentMappingToDeltaValues>(s.subtable); format4) {
        format4->forEach([&](uint32_t cp, uint32_t gid) {
          if (gid != 0) {
            ret->mapping[cp] = gid;
          }
        });
      } else if (auto format6 = dynamic_pointer_cast<TrimmedTableMapping>(s.subtable); format6) {
        format6->forEach([&](uint32_t cp, uint32_t gid) {
          if (gid != 0) {
            ret->mapping[cp] = gid;
          }
        });
      } else if (auto format12 = dynamic_pointer_cast<SegmentedCoverage>(s.subtable); format12) {
        format12->forEach([&](uint32_t cp, uint32_t gid) {
          if (gid != 0) {
            ret->mapping[cp] = gid;
          }
        });
      }
    }

    out.reset(ret.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    auto writer = make_unique<DataFragmentWriter>(&out);
    if (!writer->u16(0)) {
      return EGLYF_NULLOPT_WHAT("Failed to write format");
    }

    auto format4 = make_shared<SegmentMappingToDeltaValues>();
    auto format12 = make_shared<SegmentedCoverage>();
    for (auto const [codepoint, gid] : mapping) {
      if (gid == 0) {
        continue;
      }
      if (codepoint <= 0xffff) {
        format4->map(codepoint, gid);
      }
      format12->map(codepoint, gid);
    }

    vector<EncodingRecord> encodingRecords;
    EncodingRecord unicode3(0, 3, format4);
    encodingRecords.push_back(unicode3);
    EncodingRecord unicode4(0, 4, format12);
    encodingRecords.push_back(unicode4);
    EncodingRecord win1(3, 1, format4);
    encodingRecords.push_back(win1);
    EncodingRecord win10(3, 10, format12);
    encodingRecords.push_back(win10);

    if (!writer->sizeU16(encodingRecords.size())) {
      return EGLYF_NULLOPT_WHAT("Failed to write encoding records size");
    }
    vector<DataFragmentWriter::Marker32> subtableOffsets;
    for (auto const &record : encodingRecords) {
      if (!writer->u16(record.platformID)) {
        return EGLYF_NULLOPT_WHAT("Failed to write platform ID");
      }
      if (!writer->u16(record.encodingID)) {
        return EGLYF_NULLOPT_WHAT("Failed to write encoding ID");
      }
      auto offset = writer->o32();
      if (!offset) {
        return EGLYF_NULLOPT_WHAT("Failed to create offset marker");
      }
      subtableOffsets.push_back(offset);
    }
    for (size_t i = 0; i < encodingRecords.size(); i++) {
      auto const &record = encodingRecords[i];
      auto offset = subtableOffsets[i];
      if (auto st = writer->writeDataFragment(offset, *record.subtable); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (auto st = writer->commit(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    return EncodeResult(out.data());
  }

  std::optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    auto found = mapping.find(codepoint);
    if (found == mapping.end()) {
      return nullopt;
    } else {
      return found->second;
    }
  }

  void map(uint32_t codepoint, uint16_t glyphID) {
    using namespace std;
    if (glyphID == 0) {
      mapping.erase(codepoint);
    } else {
      mapping[codepoint] = glyphID;
    }
  }

private:
  std::map<uint32_t, uint32_t> mapping;
};

} // namespace eglyf::cmap
