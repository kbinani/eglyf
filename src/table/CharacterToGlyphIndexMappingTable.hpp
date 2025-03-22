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
        if (auto st = cmap::ByteEncoding::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 4:
        if (auto st = cmap::SegmentMappingToDeltaValues::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 12:
        if (auto st = cmap::SegmentedCoverage::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
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
    return EGLYF_NULLOPT;
  }

public:
  std::vector<EncodingRecord> encodingRecords;
};

} // namespace eglyf
