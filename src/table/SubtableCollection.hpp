#pragma once

namespace eglyf {

template <class T>
concept Subtable = requires(T &t, OutputStream &out, std::map<std::shared_ptr<T>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) {
  { t.write(out, extensions) } -> std::convertible_to<bool>;
};

template <Subtable T>
class SubtableCollection : public Table {
public:
  struct Lookup {
    uint16_t lookupType;
    uint16_t lookupFlag;
    uint16_t markFilteringSet;
    std::vector<std::shared_ptr<T>> subtables;
  };

  struct Feature {
    Tag tag;
    std::optional<Offset16> featureParamsOffset;
    std::vector<std::shared_ptr<Lookup>> lookups;
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

public:
  virtual ~SubtableCollection() {}

  virtual std::shared_ptr<T> readSubtable(InputStream &in, uint16_t lookupType) = 0;

  bool read(InputStream &in) {
    using namespace std;
    if (!in.u16(&majorVersion)) {
      return false;
    }
    if (!in.u16(&minorVersion)) {
      return false;
    }
    if (majorVersion != 1) {
      return false;
    }
    if (minorVersion > 1) {
      return false;
    }
    Offset16 scriptListOffset;
    if (!in.o16(&scriptListOffset)) {
      return false;
    }
    Offset16 featureListOffset;
    if (!in.o16(&featureListOffset)) {
      return false;
    }
    Offset16 lookupListOffset;
    if (!in.o16(&lookupListOffset)) {
      return false;
    }
    if (minorVersion == 1) {
      Offset32 v;
      if (!in.o32(&v)) {
        return false;
      }
      if (v > 0) {
        featureVariationsOffset = v;
      }
    }

    ScriptList scriptList;
    {
      if (!in.seek(scriptListOffset)) {
        return false;
      }
      OffsetInputStream sub(in);
      if (auto sl = ScriptList::Read(sub); sl) {
        scriptList = *sl;
      } else {
        return false;
      }
    }

    FeatureList featureList;
    {
      if (!in.seek(featureListOffset)) {
        return false;
      }
      OffsetInputStream sub(in);
      if (auto fl = FeatureList::Read(sub); fl) {
        featureList = *fl;
      } else {
        return false;
      }
    }

    LookupList lookupList;
    {
      if (!in.seek(lookupListOffset)) {
        return false;
      }
      OffsetInputStream sub(in);
      if (auto ll = LookupList::Read(sub); ll) {
        lookupList = *ll;
      } else {
        return false;
      }
    }

    for (auto const &it : lookupList.lookupTable) {
      auto l = make_shared<Lookup>();
      for (auto offset : it.subtableOffsets) {
        if (!in.seek(lookupListOffset + offset)) {
          return false;
        }
        OffsetInputStream sub(in);
        if (auto t = readSubtable(sub, it.lookupType); t) {
          l->subtables.push_back(t);
        } else {
          return false;
        }
      }
      lookups.push_back(l);
    }

    for (auto const &f : featureList.featureTable) {
      auto v = make_shared<Feature>();
      v->tag = f.tag;
      v->featureParamsOffset = f.featureParamsOffset;
      for (uint16_t index : f.lookupListIndices) {
        if (index >= lookups.size()) {
          return false;
        }
        v->lookups.push_back(lookups[index]);
      }
      features.push_back(v);
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
            return false;
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
            return false;
          }
          script.langSysTable.push_back(make_pair(langSysTag, converted));
          convertedLangSysList[langSys] = converted;
        } else {
          script.langSysTable.push_back(make_pair(langSysTag, found->second));
        }
      }
      scripts.push_back(script);
    }

    return true;
  }

  std::optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (majorVersion != 1) {
      return nullopt;
    }
    auto gsubHeader = make_shared<OffsetWriter>(out);
    if (!out.u16(majorVersion)) {
      return nullopt;
    }
    if (!out.u16(minorVersion)) {
      return nullopt;
    }
    auto scriptListOffsetHandle = gsubHeader->o16();
    if (!scriptListOffsetHandle) {
      return nullopt;
    }
    auto featureListOffsetHandle = gsubHeader->o16();
    if (!featureListOffsetHandle) {
      return nullopt;
    }
    auto lookupListOffsetHandle = gsubHeader->o16();
    if (!lookupListOffsetHandle) {
      return nullopt;
    }
    if (minorVersion == 0) {
      // nop
    } else if (minorVersion == 1) {
      if (!out.o32(featureVariationsOffset ? *featureVariationsOffset : 0)) {
        return nullopt;
      }
    } else {
      return nullopt;
    }

    LookupList lookupList;
    vector<vector<shared_ptr<gsub::Subtable>>> subtables;
    for (auto const &lookup : lookups) {
      LookupList::Lookup l;
      l.lookupType = lookup->lookupType;
      l.lookupFlag = lookup->lookupFlag;
      l.subtableOffsets.resize(lookup->subtables.size());
      l.markFilteringSet = lookup->markFilteringSet;
      lookupList.lookupTable.push_back(l);

      subtables.push_back(lookup->subtables);
    }

    FeatureList featureList;
    for (auto const &feature : features) {
      FeatureList::Feature f;
      f.tag = feature->tag;
      f.featureParamsOffset = feature->featureParamsOffset;
      for (auto const &lookup : feature->lookups) {
        auto found = ranges::find(lookups, lookup);
        if (found == lookups.end()) {
          return nullopt;
        }
        auto index = distance(lookups.begin(), found);
        if (index > numeric_limits<uint16_t>::max()) {
          return nullopt;
        }
        f.lookupListIndices.push_back(index);
      }
      featureList.featureTable.push_back(f);
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
          return nullopt;
        }
      }
      for (auto const &[tag, langSys] : script.langSysTable) {
        if (auto converted = convertLangSys(langSys); converted) {
          s.langSysTable.push_back(make_pair(tag, converted));
        } else {
          return nullopt;
        }
      }
      scriptList.scriptTable.push_back(s);
    }

    // Write ScriptList
    if (!scriptListOffsetHandle->mark()) {
      return nullopt;
    }
    if (!scriptList.write(out)) {
      return nullopt;
    }

    // Write FeatureList
    if (!featureListOffsetHandle->mark()) {
      return nullopt;
    }
    if (!featureList.write(out)) {
      return nullopt;
    }

    // Write LookupList
    if (!lookupListOffsetHandle->mark()) {
      return nullopt;
    }
    if (!lookupList.write<gsub::Subtable>(out, subtables)) {
      return nullopt;
    }

    if (!gsubHeader->commit()) {
      return nullopt;
    }
    return EncodeResult(out.data());
  }

public:
  uint16_t majorVersion;
  uint16_t minorVersion;
  std::optional<Offset32> featureVariationsOffset;

  std::vector<Script> scripts;
  std::vector<std::shared_ptr<Feature>> features;
  std::vector<std::shared_ptr<Lookup>> lookups;
};

} // namespace eglyf
