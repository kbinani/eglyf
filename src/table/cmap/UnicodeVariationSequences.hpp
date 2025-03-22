#pragma once

namespace eglyf::cmap {

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

    Status write(OutputStream &out) const {
      if (!out.sizeU32(ranges.size())) {
        return EGLYF_ERROR;
      }
      for (auto const &range : ranges) {
        if (!out.u24(range.startUnicodeValue)) {
          return EGLYF_ERROR;
        }
        if (!out.u8(range.additionalCount)) {
          return EGLYF_ERROR;
        }
      }
      return Status::Ok();
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

    Status write(OutputStream &out) const {
      if (!out.sizeU32(uvsMappings.size())) {
        return EGLYF_ERROR;
      }
      for (auto const &m : uvsMappings) {
        if (!out.u24(m.unicodeValue)) {
          return EGLYF_ERROR;
        }
        if (!out.u16(m.glyphID)) {
          return EGLYF_ERROR;
        }
      }
      return Status::Ok();
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

  Status write(OutputStream &out) const override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    auto const beginPos = out.position();
    if (!out.u16(14)) {
      return EGLYF_ERROR;
    }
    auto const lengthPos = out.position();
    if (!out.u32(0)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU32(varSelectors.size())) {
      return EGLYF_ERROR;
    }
    vector<pair<OffsetWriter::Handle32, OffsetWriter::Handle32>> offsets;
    for (auto const &selector : varSelectors) {
      if (!out.u24(selector.varSelector)) {
        return EGLYF_ERROR;
      }
      auto defaultUVSOffset = writer->o32();
      if (!defaultUVSOffset) {
        return EGLYF_ERROR;
      }
      auto nonDefaultUVSOffset = writer->o32();
      if (!nonDefaultUVSOffset) {
        return EGLYF_ERROR;
      }
      offsets.push_back(make_pair(defaultUVSOffset, nonDefaultUVSOffset));
    }
    for (size_t i = 0; i < varSelectors.size(); i++) {
      auto const &selector = varSelectors[i];
      auto [defaultUVSOffset, nonDefaultUVSOffset] = offsets[i];
      if (selector.defaultUVS) {
        if (auto st = defaultUVSOffset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = selector.defaultUVS->write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      } else {
        if (auto st = defaultUVSOffset->null(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (selector.nonDefaultUVS) {
        if (auto st = nonDefaultUVSOffset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = selector.nonDefaultUVS->write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      } else {
        if (auto st = nonDefaultUVSOffset->null(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }
    if (auto st = writer->commit(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    auto const endPos = out.position();
    if (!out.seek(lengthPos)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU32(endPos - beginPos)) {
      return EGLYF_ERROR;
    }
    if (!out.seek(endPos)) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

public:
  std::vector<VariationSelector> varSelectors;
};

} // namespace eglyf::cmap
