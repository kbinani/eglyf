#pragma once

namespace eglyf {

// 'cmap'
class CharacterToGlyphIndexMappingTable : public Table {
public:
  class CmapSubtable {
  public:
    virtual ~CmapSubtable() {}
  };

  struct EncodingRecord {
    uint16_t platformID;
    uint16_t encodingID;
    std::shared_ptr<CmapSubtable> subtable;
  };

  class ByteEncodingTable : public CmapSubtable {
  public:
    static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
      return EGLYF_ERROR;
    }
  };

  class SegmentMappingToDeltaValues : public CmapSubtable {
  public:
    static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
      using namespace std;
      uint16_t length;
      if (!stream.u16(&length)) {
        return EGLYF_ERROR;
      }
      if (length < 4) {
        return EGLYF_ERROR;
      }
      string buffer;
      buffer.resize(length - 4);
      if (!stream.read(buffer.data(), buffer.size())) {
        return EGLYF_ERROR;
      }
      ByteInputStream in(buffer);
      auto ret = make_unique<SegmentMappingToDeltaValues>();
      if (!in.u16(&ret->language)) {
        return EGLYF_ERROR;
      }
      uint16_t segCountX2;
      if (!in.u16(&segCountX2)) {
        return EGLYF_ERROR;
      }
      if (segCountX2 % 2 == 1) {
        return EGLYF_ERROR;
      }
      uint16_t segCount = segCountX2 / 2;
      ret->segCount = segCount;
      uint16_t entrySector;
      if (!in.u16(&entrySector)) {
        return EGLYF_ERROR;
      }
      uint16_t expectedEntrySector = (uint16_t)2 << (int)(floor(log2(segCount)) + 0.01f);
      if (entrySector != expectedEntrySector) {
        return EGLYF_ERROR;
      }
      uint16_t rangeShift;
      if (!in.u16(&rangeShift)) {
        return EGLYF_ERROR;
      }
      uint16_t expectedRangeShift = (uint16_t)(floor(log2(segCount)) + 0.01f);
      if (rangeShift != expectedRangeShift) {
        return EGLYF_ERROR;
      }
      if (!in.u16a(ret->endCode, segCount)) {
        return EGLYF_ERROR;
      }
      uint16_t reservedPad;
      if (!in.u16(&reservedPad)) {
        return EGLYF_ERROR;
      }
      if (!in.u16a(ret->startCode, segCount)) {
        return EGLYF_ERROR;
      }
      if (!in.i16a(ret->idDelta, segCount)) {
        return EGLYF_ERROR;
      }
      if (!in.u16a(ret->idRangeOffset, segCount)) {
        return EGLYF_ERROR;
      }
      while (true) {
        uint16_t glyphId;
        if (!in.u16(&glyphId)) {
          break;
        }
        ret->glyphIdArray.push_back(glyphId);
      }
      out.reset(ret.release());
      return Status::Ok();
    }

  public:
    uint16_t language;
    uint16_t segCount;
    std::vector<uint16_t> endCode;
    std::vector<uint16_t> startCode;
    std::vector<int16_t> idDelta;
    std::vector<uint16_t> idRangeOffset;
    std::vector<uint16_t> glyphIdArray;
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<CharacterToGlyphIndexMappingTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
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
    map<Offset32, shared_ptr<CmapSubtable>> tables;
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
      uint16_t fmt;
      if (!in.u16(&fmt)) {
        return EGLYF_ERROR;
      }
      shared_ptr<CmapSubtable> subtable;
      switch (fmt) {
      case 0:
        if (auto st = ByteEncodingTable::Read(in, subtable); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        } else {
          break;
        }
      case 4:
        if (auto st = SegmentMappingToDeltaValues::Read(in, subtable); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        } else {
          break;
        }
      default:
        return EGLYF_ERROR;
      }
      r.subtable = subtable;
      ret->encodingRecords.push_back(r);
      tables[record.subtableOffset] = subtable;
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
