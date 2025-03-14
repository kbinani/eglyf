#pragma once

namespace eglyf {

template <class T>
concept Subtable = requires(T &t, OutputStream &out, std::map<std::shared_ptr<T>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) {
  { t.write(out, extensions) } -> std::convertible_to<Status>;
};

template <Subtable T>
class SubtableCollection : public Table {
public:
  struct LookupData {
    uint16_t lookupType;
    uint16_t lookupFlag;
    uint16_t markFilteringSet;
    std::vector<std::shared_ptr<T>> subtables;
  };

  struct Lookup {
    std::shared_ptr<LookupData> data;
  };

  struct FeatureData {
    std::optional<std::string> featureParams;
    std::vector<std::shared_ptr<Lookup>> lookups;
  };

  struct Feature {
    Tag tag;
    std::shared_ptr<FeatureData> data;
  };

  struct LangSys {
    std::shared_ptr<Feature> requiredFeature;
    std::vector<std::shared_ptr<Feature>> features;
  };

  struct Script {
    Tag tag;
    std::shared_ptr<LangSys> defaultLangSys;
    std::vector<std::pair<Tag, std::shared_ptr<LangSys>>> langSysTable;
  };

  struct FeatureTableSubstitution {
    std::shared_ptr<Feature> feature;
    std::shared_ptr<Feature> alternateFeature;
  };

  struct FeatureVariationRecord {
    eglyf::FeatureVariations::ConditionSet conditionSet;
    std::vector<FeatureTableSubstitution> substitutions;
  };

  struct FeatureVariations {
    std::vector<FeatureVariationRecord> featureVariationRecords;
  };

public:
  virtual ~SubtableCollection() {}

  virtual Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<T> &out) = 0;

  Status read(InputStream &in) {
    using namespace std;
    if (!in.u16(&majorVersion)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&minorVersion)) {
      return EGLYF_ERROR;
    }
    if (majorVersion != 1) {
      return EGLYF_ERROR;
    }
    if (minorVersion > 1) {
      return EGLYF_ERROR;
    }
    Offset16 scriptListOffset;
    if (!in.o16(&scriptListOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 featureListOffset;
    if (!in.o16(&featureListOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 lookupListOffset;
    if (!in.o16(&lookupListOffset)) {
      return EGLYF_ERROR;
    }
    Offset32 featureVariationsOffset = 0;
    if (minorVersion == 1) {
      if (!in.o32(&featureVariationsOffset)) {
        return EGLYF_ERROR;
      }
    }

    ScriptList scriptList;
    {
      if (!in.seek(scriptListOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto sl = ScriptList::Read(sub); sl) {
        scriptList = *sl;
      } else {
        return EGLYF_STATUS_PUSH(sl.status());
      }
    }

    FeatureList featureList;
    {
      if (!in.seek(featureListOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto fl = FeatureList::Read(sub); fl) {
        featureList = *fl;
      } else {
        return EGLYF_STATUS_PUSH(fl.status());
      }
    }

    LookupList lookupList;
    {
      if (!in.seek(lookupListOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto ll = LookupList::Read(sub); ll) {
        lookupList = *ll;
      } else {
        return EGLYF_STATUS_PUSH(ll.status());
      }
    }

    map<int64_t, shared_ptr<T>> subtables;
    map<shared_ptr<LookupList::Lookup>, shared_ptr<LookupData>> convertedLookupDataList;
    for (size_t i = 0; i < lookupList.lookupTable.size(); i++) {
      shared_ptr<LookupList::Lookup> it = lookupList.lookupTable[i];
      auto l = make_shared<Lookup>();

      auto fnd = convertedLookupDataList.find(it);
      if (fnd != convertedLookupDataList.end()) {
        l->data = fnd->second;
        lookups.push_back(l);
        continue;
      }
      auto data = make_shared<LookupData>();
      data->lookupType = it->lookupType;
      data->lookupFlag = it->lookupFlag;
      data->markFilteringSet = it->markFilteringSet;
      for (auto offset : it->subtableOffsets) {
        int64_t pos = lookupListOffset;
        pos += it->lookupOffset;
        pos += offset;
        if (auto found = subtables.find(pos); found == subtables.end()) {
          if (!in.seek(pos)) {
            return EGLYF_ERROR;
          }
          OffsetInputStream sub(&in);
          shared_ptr<T> table;
          if (auto st = readSubtable(sub, it->lookupType, table); st.ok()) {
            data->subtables.push_back(table);
          } else {
            return EGLYF_STATUS_PUSH(st);
          }
          subtables[pos] = table;
        } else {
          data->subtables.push_back(found->second);
        }
      }
      l->data = data;
      convertedLookupDataList[it] = data;
      lookups.push_back(l);
    }

    map<shared_ptr<FeatureList::FeatureData>, shared_ptr<FeatureData>> convertedFeatureDataList;
    for (auto const &f : featureList.featureTable) {
      auto v = make_shared<Feature>();
      v->tag = f->tag;

      auto found = convertedFeatureDataList.find(f->data);
      if (found != convertedFeatureDataList.end()) {
        v->data = found->second;
        features.push_back(v);
        continue;
      }
      auto data = make_shared<FeatureData>();
      data->featureParams = f->data->featureParams;
      for (uint16_t index : f->data->lookupListIndices) {
        if (index >= lookups.size()) {
          return EGLYF_ERROR;
        }
        data->lookups.push_back(lookups[index]);
      }
      v->data = data;
      features.push_back(v);
      convertedFeatureDataList[f->data] = data;
    }

    if (featureVariationsOffset) {
      FeatureVariations featureVariations;
      if (!in.seek(featureVariationsOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      auto raw = ::eglyf::FeatureVariations::Read(sub);
      if (!raw) {
        return EGLYF_STATUS_PUSH(raw.status());
      }
      for (auto const &rawRecord : raw->featureVariationRecords) {
        FeatureVariationRecord record;
        record.conditionSet = rawRecord.conditionSet;
        for (auto const &rawSubstitution : rawRecord.featureTableSubstitution.substitutions) {
          FeatureTableSubstitution substitution;
          if (rawSubstitution.featureIndex >= features.size()) {
            return EGLYF_ERROR;
          }
          substitution.feature = features[rawSubstitution.featureIndex];
          if (!sub.seek(rawRecord.featureTableSubstitution.featureTableSubstitutionOffset + rawSubstitution.alternateFeatureOffset)) {
            return EGLYF_ERROR;
          }
          OffsetInputStream sub2(&sub);
          shared_ptr<FeatureList::FeatureData> fdata;
          if (auto st = FeatureList::FeatureData::Read(sub2, substitution.feature->tag, fdata); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          auto v = make_shared<Feature>();
          v->tag = substitution.feature->tag;
          auto data = make_shared<FeatureData>();
          data->featureParams = fdata->featureParams;
          for (uint16_t index : fdata->lookupListIndices) {
            if (index >= lookups.size()) {
              return EGLYF_ERROR;
            }
            data->lookups.push_back(lookups[index]);
          }
          v->data = data;
          substitution.alternateFeature = v;
          record.substitutions.push_back(substitution);
        }
        featureVariations.featureVariationRecords.push_back(record);
      }
      this->featureVariations = featureVariations;
    }

    map<shared_ptr<ScriptList::LangSys>, shared_ptr<LangSys>> convertedLangSysList;
    auto convertLangSys = [&](ScriptList::LangSys const &from) -> shared_ptr<LangSys> {
      auto converted = make_shared<LangSys>();
      if (from.requiredFeatureIndex) {
        if (*from.requiredFeatureIndex >= features.size()) {
          return nullptr;
        }
        converted->requiredFeature = features[*from.requiredFeatureIndex];
      }
      for (auto const &index : from.featureIndices) {
        if (index >= features.size()) {
          return nullptr;
        }
        converted->features.push_back(features[index]);
      }
      return converted;
    };
    for (auto const &s : scriptList.scriptTable) {
      Script script;
      script.tag = s.tag;
      if (s.defaultLangSys) {
        if (auto found = convertedLangSysList.find(s.defaultLangSys); found == convertedLangSysList.end()) {
          auto converted = convertLangSys(*s.defaultLangSys);
          if (!converted) {
            return EGLYF_ERROR;
          }
          script.defaultLangSys = converted;
          convertedLangSysList[s.defaultLangSys] = converted;
        } else {
          script.defaultLangSys = found->second;
        }
      }
      for (auto const &[langSysTag, langSys] : s.langSysTable) {
        if (auto found = convertedLangSysList.find(langSys); found == convertedLangSysList.end()) {
          auto converted = convertLangSys(*langSys);
          if (!converted) {
            return EGLYF_ERROR;
          }
          script.langSysTable.push_back(make_pair(langSysTag, converted));
          convertedLangSysList[langSys] = converted;
        } else {
          script.langSysTable.push_back(make_pair(langSysTag, found->second));
        }
      }
      scripts.push_back(script);
    }

    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (majorVersion != 1) {
      return EGLYF_NULLOPT;
    }
    auto gsubHeader = make_shared<OffsetWriter>(out);
    if (!out.u16(majorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(minorVersion)) {
      return EGLYF_NULLOPT;
    }
    auto scriptListOffsetHandle = gsubHeader->o16();
    if (!scriptListOffsetHandle) {
      return EGLYF_NULLOPT;
    }
    auto featureListOffsetHandle = gsubHeader->o16();
    if (!featureListOffsetHandle) {
      return EGLYF_NULLOPT;
    }
    auto lookupListOffsetHandle = gsubHeader->o16();
    if (!lookupListOffsetHandle) {
      return EGLYF_NULLOPT;
    }
    OffsetWriter::Handle32 featureVariationOffset;
    if (minorVersion == 0) {
      // nop
    } else if (minorVersion == 1) {
      if (featureVariations) {
        featureVariationOffset = gsubHeader->o32();
        if (!featureVariationOffset) {
          return EGLYF_NULLOPT;
        }
      } else {
        if (!out.o32(0)) {
          return EGLYF_NULLOPT;
        }
      }
    } else {
      return EGLYF_NULLOPT;
    }

    LookupList lookupList;
    vector<vector<shared_ptr<T>>> subtables;
    map<shared_ptr<LookupData>, shared_ptr<LookupList::Lookup>> convertedLookups;
    for (auto const &lookup : lookups) {
      subtables.push_back(lookup->data->subtables);

      auto found = convertedLookups.find(lookup->data);
      if (found != convertedLookups.end()) {
        lookupList.lookupTable.push_back(found->second);
        continue;
      }
      auto l = make_shared<LookupList::Lookup>();
      l->lookupType = lookup->data->lookupType;
      l->lookupFlag = lookup->data->lookupFlag;
      l->subtableOffsets.resize(lookup->data->subtables.size());
      l->markFilteringSet = lookup->data->markFilteringSet;
      lookupList.lookupTable.push_back(l);
      convertedLookups[lookup->data] = l;
    }

    FeatureList featureList;
    map<shared_ptr<FeatureData>, shared_ptr<FeatureList::FeatureData>> convertedFeatures;
    for (size_t i = 0; i < features.size(); i++) {
      auto const &feature = features[i];
      auto f = make_shared<FeatureList::Feature>();
      f->tag = feature->tag;

      auto found = convertedFeatures.find(feature->data);
      if (found != convertedFeatures.end()) {
        f->data = found->second;
        featureList.featureTable.push_back(f);
        continue;
      }

      auto data = make_shared<FeatureList::FeatureData>();
      data->featureParams = feature->data->featureParams;
      for (auto const &lookup : feature->data->lookups) {
        auto found = ranges::find(lookups, lookup);
        if (found == lookups.end()) {
          return EGLYF_NULLOPT;
        }
        size_t index = distance(lookups.begin(), found);
        if (index > numeric_limits<uint16_t>::max()) {
          return EGLYF_NULLOPT;
        }
        data->lookupListIndices.push_back(index);
      }
      f->data = data;
      featureList.featureTable.push_back(f);
      convertedFeatures[feature->data] = data;
    }

    ScriptList scriptList;
    map<shared_ptr<LangSys>, shared_ptr<ScriptList::LangSys>> langSysMap;
    auto convertLangSys = [&](shared_ptr<LangSys> from) -> shared_ptr<ScriptList::LangSys> {
      auto found = langSysMap.find(from);
      if (found == langSysMap.end()) {
        auto to = make_shared<ScriptList::LangSys>();
        if (from->requiredFeature) {
          auto f = ranges::find(features, from->requiredFeature);
          if (f == features.end()) {
            return nullptr;
          }
          auto index = distance(features.begin(), f);
          if (index > numeric_limits<uint16_t>::max()) {
            return nullptr;
          }
          to->requiredFeatureIndex = index;
        }
        for (auto const &f : from->features) {
          auto ff = ranges::find(features, f);
          if (ff == features.end()) {
            return nullptr;
          }
          auto index = distance(features.begin(), ff);
          if (index > numeric_limits<uint16_t>::max()) {
            return nullptr;
          }
          to->featureIndices.push_back(index);
        }
        langSysMap[from] = to;
        return to;
      } else {
        return found->second;
      }
    };
    for (auto const &script : scripts) {
      ScriptList::Script s;
      s.tag = script.tag;
      if (script.defaultLangSys) {
        if (auto converted = convertLangSys(script.defaultLangSys); converted) {
          s.defaultLangSys = converted;
        } else {
          return EGLYF_NULLOPT;
        }
      }
      for (auto const &[tag, langSys] : script.langSysTable) {
        if (auto converted = convertLangSys(langSys); converted) {
          s.langSysTable.push_back(make_pair(tag, converted));
        } else {
          return EGLYF_NULLOPT;
        }
      }
      scriptList.scriptTable.push_back(s);
    }

    // Write ScriptList
    if (auto st = scriptListOffsetHandle->mark(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    if (auto st = scriptList.write(out); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    // Write FeatureList
    if (auto st = featureListOffsetHandle->mark(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    if (auto st = featureList.write(out); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    // Write LookupList
    if (auto st = lookupListOffsetHandle->mark(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    if (auto st = lookupList.write<T>(out, subtables); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    // Write feature variations
    if (featureVariationOffset) {
      if (auto st = featureVariationOffset->mark(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      auto writer = make_shared<OffsetWriter>(out);
      // majorVersion
      if (!out.u16(1)) {
        return EGLYF_NULLOPT;
      }
      // minorVersion
      if (!out.u16(0)) {
        return EGLYF_NULLOPT;
      }
      if (!out.sizeU32(featureVariations->featureVariationRecords.size())) {
        return EGLYF_NULLOPT;
      }
      vector<tuple<FeatureVariationRecord, OffsetWriter::Handle32, OffsetWriter::Handle32>> handles;
      for (auto const &rawRecord : featureVariations->featureVariationRecords) {
        auto conditionSetOffset = writer->o32();
        if (!conditionSetOffset) {
          return EGLYF_NULLOPT;
        }
        auto featureTableSubstitutionOffset = writer->o32();
        if (!featureTableSubstitutionOffset) {
          return EGLYF_NULLOPT;
        }
        handles.push_back(make_tuple(rawRecord, conditionSetOffset, featureTableSubstitutionOffset));
      }
      for (auto const &[rawRecord, conditionSetOffset, featureTableSubstitutionOffset] : handles) {
        if (auto st = conditionSetOffset->mark(); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        if (auto st = rawRecord.conditionSet.write(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        if (auto st = featureTableSubstitutionOffset->mark(); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        auto w = make_shared<OffsetWriter>(out);
        if (!out.u16(1)) {
          return EGLYF_NULLOPT;
        }
        if (!out.u16(0)) {
          return EGLYF_NULLOPT;
        }
        if (!out.sizeU16(rawRecord.substitutions.size())) {
          return EGLYF_NULLOPT;
        }
        vector<OffsetWriter::Handle32> alternateFeatureOffsets;
        for (auto const &substitution : rawRecord.substitutions) {
          auto found = ranges::find_if(features, [&](auto const &it) { return it == substitution.feature; });
          if (found == features.end()) {
            return EGLYF_NULLOPT;
          }
          auto index = distance(features.begin(), found);
          if (!out.sizeU16(index)) {
            return EGLYF_NULLOPT;
          }
          auto alternateFeatureOffset = w->o32();
          if (!alternateFeatureOffset) {
            return EGLYF_NULLOPT;
          }
          alternateFeatureOffsets.push_back(alternateFeatureOffset);
        }
        vector<tuple<string, OffsetWriter::Handle16, shared_ptr<OffsetWriter>>> featureParamsWriters;
        for (size_t i = 0; i < rawRecord.substitutions.size(); i++) {
          auto const &substitution = rawRecord.substitutions[i];
          auto offset = alternateFeatureOffsets[i];
          if (auto st = offset->mark(); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
          }
          auto w2 = make_shared<OffsetWriter>(out);
          if (substitution.alternateFeature->data->featureParams) {
            auto featureParamsOffset = w2->o16();
            if (!featureParamsOffset) {
              return EGLYF_NULLOPT;
            }
            featureParamsWriters.push_back(make_tuple(*substitution.alternateFeature->data->featureParams, featureParamsOffset, w2));
          } else {
            if (!out.o16(0)) {
              return EGLYF_NULLOPT;
            }
          }
          if (!out.sizeU16(substitution.alternateFeature->data->lookups.size())) {
            return EGLYF_NULLOPT;
          }
          for (auto const &lookup : substitution.alternateFeature->data->lookups) {
            auto found = ranges::find_if(lookups, [&](auto const &it) { return it == lookup; });
            if (found == lookups.end()) {
              return EGLYF_NULLOPT;
            }
            auto index = distance(lookups.begin(), found);
            if (!out.sizeU16(index)) {
              return EGLYF_NULLOPT;
            }
          }
        }
        if (auto st = w->commit(); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        for (auto const &[featureParams, featureParamsOffset, w2] : featureParamsWriters) {
          if (auto st = featureParamsOffset->mark(); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
          }
          if (!out.write(featureParams.data(), featureParams.size())) {
            return EGLYF_NULLOPT;
          }
          if (auto st = w2->commit(); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
          }
        }
      }
      if (auto st = writer->commit(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }

    if (auto st = gsubHeader->commit(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    return EncodeResult(out.data());
  }

public:
  uint16_t majorVersion;
  uint16_t minorVersion;
  std::optional<FeatureVariations> featureVariations;

  std::vector<Script> scripts;
  std::vector<std::shared_ptr<Feature>> features;
  std::vector<std::shared_ptr<Lookup>> lookups;
};

} // namespace eglyf
