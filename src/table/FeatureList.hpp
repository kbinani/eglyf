#pragma once

namespace eglyf {

class FeatureList {
public:
  struct Feature {
    Tag tag;
    std::optional<std::string> featureParams;
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
      if (!in.u16a(f.lookupListIndices, lookupIndexCount)) {
        return EGLYF_NULLOPT;
      }
      if (featureParamsOffset != 0) {
        auto pos = in.position();
        if (!in.seek(featureOffset + featureParamsOffset)) {
          return EGLYF_NULLOPT;
        }
        string data;
        if (FCC("cv01") <= tag && tag <= FCC("cv99")) {
          data.resize(17);
        } else if (FCC("size") == tag) {
          data.resize(10);
        } else if (FCC("ss01") <= tag && tag <= FCC("ss20")) {
          data.resize(4);
        } else {
          return EGLYF_NULLOPT_WHAT("Unexpected feature params offset");
        }
        if (!in.read(data.data(), data.size())) {
          return EGLYF_NULLOPT;
        }
        if (!in.seek(pos)) {
          return EGLYF_NULLOPT;
        }
        f.featureParams = data;
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
    vector<tuple<string, shared_ptr<OffsetWriter>, OffsetWriter::Handle16>> featureParamsOffsets;
    for (size_t i = 0; i < featureTable.size(); i++) {
      auto const &feature = featureTable[i];
      auto handle = featureOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      auto writer = make_shared<OffsetWriter>(out);
      if (feature.featureParams) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        featureParamsOffsets.push_back(make_tuple(*feature.featureParams, writer, offset));
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
    for (auto &[data, writer, offset] : featureParamsOffsets) {
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (!out.write(data.data(), data.size())) {
        return EGLYF_ERROR;
      }
      if (auto st = writer->commit(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(featureTableBeginning->commit());
  }

public:
  std::vector<Feature> featureTable;
};

} // namespace eglyf
