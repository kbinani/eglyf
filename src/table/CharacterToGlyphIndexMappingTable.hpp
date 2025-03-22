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
      return EGLYF_ERROR;
    }
    if (format != 0) {
      return EGLYF_ERROR;
    }
    uint16_t numTables;
    if (!in.u16(&numTables)) {
      return EGLYF_ERROR;
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
        return EGLYF_ERROR;
      }
      if (!in.u16(&r.encodingID)) {
        return EGLYF_ERROR;
      }
      if (!in.o32(&r.subtableOffset)) {
        return EGLYF_ERROR;
      }
      encodingRecords.push_back(r);
    }
    map<Offset32, shared_ptr<cmap::CmapSubtable>> tables;
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
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      uint16_t fmt;
      if (!sub.u16(&fmt)) {
        return EGLYF_ERROR;
      }
      switch (fmt) {
      case 0:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 0");
      case 2:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 2");
      case 4:
        if (auto st = cmap::SegmentMappingToDeltaValues::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 6:
        if (auto st = cmap::TrimmedTableMapping::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 8:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 8");
      case 10:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 10");
      case 12:
        if (auto st = cmap::SegmentedCoverage::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 13:
        return EGLYF_ERROR_WHAT("Unsupported cmap subtable format: 13");
      case 14:
        if (auto st = cmap::UnicodeVariationSequences::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      default:
        return EGLYF_ERROR;
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
      return EGLYF_NULLOPT;
    }
    if (!writer->sizeU16(encodingRecords.size())) {
      return EGLYF_NULLOPT;
    }
    vector<DataFragmentWriter::Marker32> subtableOffsets;
    for (auto const &record : encodingRecords) {
      if (!writer->u16(record.platformID)) {
        return EGLYF_NULLOPT;
      }
      if (!writer->u16(record.encodingID)) {
        return EGLYF_NULLOPT;
      }
      auto offset = writer->o32();
      if (!offset) {
        return EGLYF_NULLOPT;
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

public:
  std::vector<EncodingRecord> encodingRecords;
};

} // namespace eglyf
