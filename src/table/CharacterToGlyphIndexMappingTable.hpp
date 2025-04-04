#pragma once

namespace eglyf {

// 'cmap'
class CharacterToGlyphIndexMappingTable : public Table {
public:
  struct EncodingRecord {
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
    vector<Record> encodingRecords;
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
      encodingRecords.push_back(r);
    }
    std::map<Offset32, shared_ptr<cmap::CmapSubtable>> tables;
    auto ret = make_unique<CharacterToGlyphIndexMappingTable>();
    for (auto const &record : encodingRecords) {
      EncodingRecord r;
      r.platformID = record.platformID;
      r.encodingID = record.encodingID;
      if (auto found = tables.find(record.subtableOffset); found != tables.end()) {
        r.subtable = found->second;
        ret->encodingRecords.push_back(r);
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
      ret->encodingRecords.push_back(r);
      tables[record.subtableOffset] = r.subtable;
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

  Optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    auto p0e4 = find_if(encodingRecords.rbegin(), encodingRecords.rend(), [](EncodingRecord const &r) {
      return r.platformID == 0 && (r.encodingID == 4 || r.encodingID == 6);
    });
    if (p0e4 == encodingRecords.rend()) {
      return 0;
    }
    auto const &subtable = p0e4->subtable;
    if (auto format12 = dynamic_pointer_cast<cmap::SegmentedCoverage>(subtable); format12) {
      if (auto gid = format12->getGlyphID(codepoint); gid) {
        return *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    } else if (auto format4 = dynamic_pointer_cast<cmap::SegmentMappingToDeltaValues>(subtable); format4) {
      if (auto gid = format4->getGlyphID(codepoint); gid) {
        return *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    } else {
      return 0;
    }
    return 0;
  }

  Status map(uint32_t codepoint, uint16_t glyphId) {
    using namespace std;
    set<shared_ptr<cmap::CmapSubtable>> done;
    for (size_t i = 0; i < encodingRecords.size(); i++) {
      auto &r = encodingRecords[i];
      if (done.find(r.subtable) != done.end()) {
        continue;
      }
      done.insert(r.subtable);
      if (auto format4 = dynamic_pointer_cast<cmap::SegmentMappingToDeltaValues>(r.subtable); format4 && codepoint <= 0xffff) {
        if (auto st = format4->map((uint16_t)codepoint, glyphId); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      } else if (auto format6 = dynamic_pointer_cast<cmap::TrimmedTableMapping>(r.subtable); format6 && codepoint <= 0xffff) {
        if (auto st = format6->map((uint16_t)codepoint, glyphId); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (format6->writeMayFail()) {
          shared_ptr<cmap::SegmentMappingToDeltaValues> format4;
          if (auto st = format6->convertToFormat4(format4); !st.ok()) {
            return EGLYF_ERROR_WHAT("Failed to convert format 6 to format 4");
          }
          r.subtable = format4;
          done.insert(format4);
          for (size_t j = 0; j < encodingRecords.size(); j++) {
            if (encodingRecords[j].subtable == format6) {
              encodingRecords[j].subtable = format4;
            }
          }
        }
      } else if (auto format12 = dynamic_pointer_cast<cmap::SegmentedCoverage>(r.subtable); format12) {
        if (auto st = format12->map(codepoint, glyphId); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }
    return Status::Ok();
  }

public:
  std::vector<EncodingRecord> encodingRecords;
};

} // namespace eglyf
