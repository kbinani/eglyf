#pragma once

namespace eglyf {

class FeatureList {
public:
  struct FeatureData {
    std::optional<std::string> featureParams;
    std::vector<uint16_t> lookupListIndices;

    static Status Read(InputStream &stream, Tag tag, std::shared_ptr<FeatureData> &out) {
      using namespace std;
      OffsetInputStream in(&stream);
      Offset16 featureParamsOffset;
      if (!in.o16(&featureParamsOffset)) {
        return EGLYF_ERROR;
      }
      uint16_t lookupIndexCount;
      if (!in.u16(&lookupIndexCount)) {
        return EGLYF_ERROR;
      }
      auto f = make_unique<FeatureData>();
      if (!in.u16a(f->lookupListIndices, lookupIndexCount)) {
        return EGLYF_ERROR;
      }
      if (featureParamsOffset != 0) {
        auto pos = in.position();
        if (!in.seek(featureParamsOffset)) {
          return EGLYF_ERROR;
        }
        string data;
        if (FCC("cv01") <= tag && tag <= FCC("cv99")) {
          data.resize(17);
        } else if (FCC("size") == tag) {
          data.resize(10);
        } else if (FCC("ss01") <= tag && tag <= FCC("ss20")) {
          data.resize(4);
        } else {
          return EGLYF_ERROR_WHAT(Status::Error::UnexpectedFeatureParamsOffset());
        }
        if (!in.read(data.data(), data.size())) {
          return EGLYF_ERROR;
        }
        if (!in.seek(pos)) {
          return EGLYF_ERROR;
        }
        f->featureParams = data;
      }
      out.reset(f.release());
      return Status::Ok();
    }
  };

  struct Feature {
    Tag tag;
    std::shared_ptr<FeatureData> data;
  };

public:
  static Optional<FeatureList> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);
    FeatureList featureList;
    uint16_t featureCount;
    if (!in.u16(&featureCount)) {
      return EGLYF_NULLOPT;
    }
    vector<pair<Tag, Offset16>> featureOffsets;
    set<Offset16> o;
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
      o.insert(offset);
    }
    map<Offset16, shared_ptr<FeatureData>> convertedFeatureDataList;
    for (auto [tag, featureOffset] : featureOffsets) {
      auto feature = make_shared<Feature>();
      feature->tag = tag;

      auto found = convertedFeatureDataList.find(featureOffset);
      if (found != convertedFeatureDataList.end()) {
        feature->data = found->second;
        featureList.featureTable.push_back(feature);
        continue;
      }
      if (!in.seek(featureOffset)) {
        return EGLYF_NULLOPT;
      }
      shared_ptr<FeatureData> data;
      if (auto st = FeatureData::Read(in, tag, data); st.ok()) {
        feature->data = data;
        featureList.featureTable.push_back(feature);
        convertedFeatureDataList[featureOffset] = data;
      } else {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    return featureList;
  }

  Status write(OutputStream &out) {
    using namespace std;
    auto featureTableBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(featureTable.size())) {
      return EGLYF_ERROR;
    }
    map<shared_ptr<FeatureData>, vector<OffsetWriter::Handle16>> featureOffsets;
    for (auto const &feature : featureTable) {
      if (!out.write(feature->tag.data(), feature->tag.size())) {
        return EGLYF_ERROR;
      }
      auto handle = featureTableBeginning->o16();
      if (!handle) {
        return EGLYF_ERROR;
      }
      featureOffsets[feature->data].push_back(handle);
    }
    vector<tuple<string, shared_ptr<OffsetWriter>, OffsetWriter::Handle16>> featureParamsOffsets;
    for (auto const &[data, offsets] : featureOffsets) {
      for (auto offset : offsets) {
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      auto writer = make_shared<OffsetWriter>(out);
      if (data->featureParams) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        featureParamsOffsets.push_back(make_tuple(*data->featureParams, writer, offset));
      } else {
        if (!out.o16(0)) {
          return EGLYF_ERROR;
        }
      }
      if (!out.sizeU16(data->lookupListIndices.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(data->lookupListIndices)) {
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
  std::vector<std::shared_ptr<Feature>> featureTable;
};

} // namespace eglyf
