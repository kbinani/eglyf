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

  // format 0
  class ByteEncodingTable : public CmapSubtable {
  public:
    static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
      return EGLYF_ERROR;
    }
  };

  // format 4
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

  // format 12
  class SegmentedCoverage : public CmapSubtable {
  public:
    struct SequentialMapGroup {
      uint32_t startCharCode;
      uint32_t endCharCode;
      uint32_t startGlyphID;
    };

  public:
    static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
      using namespace std;
      uint16_t reserved;
      if (!stream.u16(&reserved)) {
        return EGLYF_ERROR;
      }
      uint32_t length;
      if (!stream.u32(&length)) {
        return EGLYF_ERROR;
      }
      if (length < 8) {
        return EGLYF_ERROR;
      }
      string data;
      data.resize(length - 8);
      if (!stream.read(data.data(), data.size())) {
        return EGLYF_ERROR;
      }
      auto ret = make_unique<SegmentedCoverage>();
      ByteInputStream in(data);
      if (!in.u32(&ret->language)) {
        return EGLYF_ERROR;
      }
      uint32_t numGroups;
      if (!in.u32(&numGroups)) {
        return EGLYF_ERROR;
      }
      for (uint32_t i = 0; i < numGroups; i++) {
        SequentialMapGroup g;
        if (!in.u32(&g.startCharCode)) {
          return EGLYF_ERROR;
        }
        if (!in.u32(&g.endCharCode)) {
          return EGLYF_ERROR;
        }
        if (!in.u32(&g.startGlyphID)) {
          return EGLYF_ERROR;
        }
        ret->groups.push_back(g);
      }
      out.reset(ret.release());
      return Status::Ok();
    }

  public:
    uint32_t language;
    std::vector<SequentialMapGroup> groups;
  };

  // format 14
  class UnicodeVariationSequences : public CmapSubtable {
  public:
    struct UnicodeRange {
      uint24 startUnicodeValue;
      uint8_t additionalCount;

      static Optional<UnicodeRange> Read(InputStream &in) {
        UnicodeRange r;
        if (!in.read(r.startUnicodeValue.data(), r.startUnicodeValue.size())) {
          return EGLYF_NULLOPT;
        }
        if (!in.u8(&r.additionalCount)) {
          return EGLYF_NULLOPT;
        }
        return r;
      }
    };

    struct DefaultUVS {
      std::vector<UnicodeRange> ranges;

      static Optional<DefaultUVS> Read(InputStream &in) {
        uint32_t numUnicodeValueRanges;
        if (!in.u32(&numUnicodeValueRanges)) {
          return EGLYF_NULLOPT;
        }
        DefaultUVS ret;
        for (uint32_t i = 0; i < numUnicodeValueRanges; i++) {
          if (auto r = UnicodeRange::Read(in); r) {
            ret.ranges.push_back(*r);
          } else {
            return EGLYF_NULLOPT_PUSH(r.status());
          }
        }
        return ret;
      }
    };

    struct UVSMapping {
      uint24 unicodeValue;
      uint16_t glyphID;

      static Optional<UVSMapping> Read(InputStream &in) {
        UVSMapping m;
        if (!in.read(m.unicodeValue.data(), m.unicodeValue.size())) {
          return EGLYF_NULLOPT;
        }
        if (!in.u16(&m.glyphID)) {
          return EGLYF_NULLOPT;
        }
        return m;
      }
    };

    struct NonDefaultUVS {
      std::vector<UVSMapping> uvsMappings;

      static Optional<NonDefaultUVS> Read(InputStream &in) {
        uint32_t numUVSMappings;
        if (!in.u32(&numUVSMappings)) {
          return EGLYF_NULLOPT;
        }
        NonDefaultUVS ret;
        for (uint32_t i = 0; i < numUVSMappings; i++) {
          if (auto m = UVSMapping::Read(in); m) {
            ret.uvsMappings.push_back(*m);
          } else {
            return EGLYF_NULLOPT_PUSH(m.status());
          }
        }
        return ret;
      }
    };

    struct VariationSelector {
      uint24 varSelector;
      std::optional<DefaultUVS> defaultUVS;
      std::optional<NonDefaultUVS> nonDefaultUVS;
    };

  public:
    static Status Read(InputStream &in, std::shared_ptr<CmapSubtable> &out) {
      using namespace std;
      auto o = in.position();
      jassert(in.position() == 2);
      uint32_t length;
      if (!in.u32(&length)) {
        return EGLYF_ERROR;
      }
      if (length < 4) {
        return EGLYF_ERROR;
      }
      uint32_t numVarSelectorRecords;
      if (!in.u32(&numVarSelectorRecords)) {
        return EGLYF_ERROR;
      }
      struct Selector {
        uint24 varSelector;
        Offset32 defaultUVSOffset;
        Offset32 nonDefaultUVSOffset;
      };
      vector<Selector> varSelectors;
      for (uint32_t i = 0; i < numVarSelectorRecords; i++) {
        Selector s;
        if (!in.read(s.varSelector.data(), s.varSelector.size())) {
          return EGLYF_ERROR;
        }
        if (!in.o32(&s.defaultUVSOffset)) {
          return EGLYF_ERROR;
        }
        if (!in.o32(&s.nonDefaultUVSOffset)) {
          return EGLYF_ERROR;
        }
        varSelectors.push_back(s);
      }
      auto ret = make_unique<UnicodeVariationSequences>();
      for (auto const &s : varSelectors) {
        VariationSelector vs;
        vs.varSelector = s.varSelector;
        if (s.defaultUVSOffset > 0) {
          if (!in.seek(s.defaultUVSOffset)) {
            return EGLYF_ERROR;
          }
          if (auto duvs = DefaultUVS::Read(in); duvs) {
            vs.defaultUVS = *duvs;
          } else {
            return EGLYF_STATUS_PUSH(duvs.status());
          }
        }
        if (s.nonDefaultUVSOffset > 0) {
          if (!in.seek(s.nonDefaultUVSOffset)) {
            return EGLYF_ERROR;
          }
          if (auto nduvs = NonDefaultUVS::Read(in); nduvs) {
            vs.nonDefaultUVS = *nduvs;
          } else {
            return EGLYF_STATUS_PUSH(nduvs.status());
          }
        }
        ret->varSelectors.push_back(vs);
      }
      out.reset(ret.release());
      return Status::Ok();
    }

  public:
    std::vector<VariationSelector> varSelectors;
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
      OffsetInputStream sub(&in);
      uint16_t fmt;
      if (!sub.u16(&fmt)) {
        return EGLYF_ERROR;
      }
      switch (fmt) {
      case 0:
        if (auto st = ByteEncodingTable::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 4:
        if (auto st = SegmentMappingToDeltaValues::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 12:
        if (auto st = SegmentedCoverage::Read(sub, r.subtable); st.ok()) {
          break;
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      case 14:
        if (auto st = UnicodeVariationSequences::Read(sub, r.subtable); st.ok()) {
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
