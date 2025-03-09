#pragma once

namespace eglyf {

class FeatureList {
public:
  struct Feature {
    Tag tag;
    std::optional<Offset16> featureParamsOffset;
    std::vector<uint16_t> lookupListIndices;
  };

public:
  static Optional<FeatureList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    FeatureList featureList;
    uint16_t featureCount;
    if (!in.u16(&featureCount)) {
      return EGLYF_NULLOPT;
    }
    vector<pair<Tag, Offset16>> featureOffsets;
    for (uint16_t i = 0; i < featureCount; i++) {
      auto tag = ReadTag(in);
      if (!tag) {
        return EGLYF_NULLOPT;
      }
      Offset16 offset;
      if (!in.o16(&offset)) {
        return EGLYF_NULLOPT;
      }
      featureOffsets.push_back(make_pair(*tag, offset));
    }
    for (auto [tag, featureOffset] : featureOffsets) {
      if (!in.seek(featureOffset)) {
        return EGLYF_NULLOPT;
      }
      Offset16 featureParamsOffset;
      if (!in.o16(&featureParamsOffset)) {
        return EGLYF_NULLOPT;
      }
      uint16_t lookupIndexCount;
      if (!in.u16(&lookupIndexCount)) {
        return EGLYF_NULLOPT;
      }
      Feature f;
      f.tag = tag;
      if (featureParamsOffset != 0) {
        f.featureParamsOffset = featureParamsOffset;
      }
      if (!in.u16a(f.lookupListIndices, lookupIndexCount)) {
        return EGLYF_NULLOPT;
      }
      featureList.featureTable.push_back(f);
    }
    return featureList;
  }

  Status write(OutputStream &out) {
    using namespace std;
    auto featureTableBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(featureTable.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> featureOffsets;
    for (auto const &feature : featureTable) {
      if (!out.write(feature.tag.data(), feature.tag.size())) {
        return EGLYF_ERROR;
      }
      auto handle = featureTableBeginning->o16();
      if (!handle) {
        return EGLYF_ERROR;
      }
      featureOffsets.push_back(handle);
    }
    for (size_t i = 0; i < featureTable.size(); i++) {
      auto const &feature = featureTable[i];
      auto handle = featureOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (feature.featureParamsOffset) {
        if (!out.o16(*feature.featureParamsOffset)) {
          return EGLYF_ERROR;
        }
      } else {
        if (!out.o16(0)) {
          return EGLYF_ERROR;
        }
      }
      if (!out.sizeU16(feature.lookupListIndices.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(feature.lookupListIndices)) {
        return EGLYF_ERROR;
      }
    }
    return EGLYF_STATUS_PUSH(featureTableBeginning->commit());
  }

public:
  std::vector<Feature> featureTable;
};

} // namespace eglyf
