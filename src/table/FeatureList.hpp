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
  static std::optional<FeatureList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    FeatureList featureList;
    uint16_t featureCount;
    if (!in.u16(&featureCount)) {
      return nullopt;
    }
    vector<pair<Tag, Offset16>> featureOffsets;
    for (uint16_t i = 0; i < featureCount; i++) {
      auto tag = ReadTag(in);
      if (!tag) {
        return nullopt;
      }
      Offset16 offset;
      if (!in.o16(&offset)) {
        return nullopt;
      }
      featureOffsets.push_back(make_pair(*tag, offset));
    }
    for (auto [tag, featureOffset] : featureOffsets) {
      Offset16 featureParamsOffset;
      if (!in.o16(&featureParamsOffset)) {
        return nullopt;
      }
      uint16_t lookupIndexCount;
      if (!in.u16(&lookupIndexCount)) {
        return nullopt;
      }
      Feature f;
      f.tag = tag;
      if (featureParamsOffset != 0) {
        f.featureParamsOffset = featureParamsOffset;
      }
      f.lookupListIndices.reserve(lookupIndexCount);
      for (uint16_t i = 0; i < lookupIndexCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        f.lookupListIndices.push_back(v);
      }
      featureList.featureTable.push_back(f);
    }
    return featureList;
  }

public:
  std::vector<Feature> featureTable;
};

} // namespace eglyf
