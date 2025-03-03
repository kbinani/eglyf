#pragma once

namespace eglyf {

class GlyphSubstitutionTable : public Table {
public:
  struct Lookup {
    uint16_t lookupType;
    uint16_t lookupFlag;
    uint16_t markFilteringSet;
    std::vector<std::shared_ptr<gsub::Subtable>> subtables;
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
  static std::shared_ptr<GlyphSubstitutionTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<GlyphSubstitutionTable>();
    if (!in.u16(&r->majorVersion)) {
      return nullptr;
    }
    if (!in.u16(&r->minorVersion)) {
      return nullptr;
    }
    if (r->majorVersion != 1) {
      return nullptr;
    }
    if (r->minorVersion > 1) {
      return nullptr;
    }
    Offset16 scriptListOffset;
    if (!in.o16(&scriptListOffset)) {
      return nullptr;
    }
    Offset16 featureListOffset;
    if (!in.o16(&featureListOffset)) {
      return nullptr;
    }
    Offset16 lookupListOffset;
    if (!in.o16(&lookupListOffset)) {
      return nullptr;
    }
    if (r->minorVersion == 1) {
      Offset32 v;
      if (!in.o32(&v)) {
        return nullptr;
      }
      if (v > 0) {
        r->featureVariationsOffset = v;
      }
    }

    ScriptList scriptList;
    {
      if (!in.seek(scriptListOffset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto sl = ScriptList::Read(sub); sl) {
        scriptList = *sl;
      } else {
        return nullptr;
      }
    }

    FeatureList featureList;
    {
      if (!in.seek(featureListOffset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto fl = FeatureList::Read(sub); fl) {
        featureList = *fl;
      } else {
        return nullptr;
      }
    }

    LookupList lookupList;
    {
      if (!in.seek(lookupListOffset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto ll = LookupList::Read(sub); ll) {
        lookupList = *ll;
      } else {
        return nullptr;
      }
    }

    for (auto const &it : lookupList.lookupTable) {
      auto l = make_shared<Lookup>();
      for (auto offset : it.subtableOffsets) {
        if (!in.seek(lookupListOffset + offset)) {
          return nullptr;
        }
        OffsetInputStream sub(in);
        if (it.lookupType == 1) {
          // Single
          if (auto t = gsub::Single::Read(sub); t) {
            l->subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 2) {
          // Multiple
          if (auto t = gsub::Multiple::Read(sub); t) {
            l->subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 3) {
          // Alternate
          return nullptr;
        } else if (it.lookupType == 4) {
          // Ligature
          if (auto t = gsub::Ligature::Read(sub); t) {
            l->subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 5) {
          // Contextual substitution
          return nullptr;
        } else if (it.lookupType == 6) {
          // Chained contexts substitution
          if (auto t = gsub::ChainedContextsSubstitution::Read(sub); t) {
            l->subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 7) {
          // Substitituion extension
          if (auto t = gsub::SubstitutionExtension::Read(sub); t) {
            l->subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 8) {
          // Reverse chaining context single
          return nullptr;
        } else {
          return nullptr;
        }
      }
      r->lookups.push_back(l);
    }

    for (auto const &f : featureList.featureTable) {
      auto v = make_shared<Feature>();
      v->tag = f.tag;
      v->featureParamsOffset = f.featureParamsOffset;
      for (uint16_t index : f.lookupListIndices) {
        if (index >= r->lookups.size()) {
          return nullptr;
        }
        v->lookups.push_back(r->lookups[index]);
      }
      r->features.push_back(v);
    }

    map<shared_ptr<ScriptList::LangSys>, shared_ptr<LangSys>> convertedLangSysList;
    auto convertLangSys = [&](ScriptList::LangSys const &from) -> shared_ptr<LangSys> {
      auto converted = make_shared<LangSys>();
      if (from.requiredFeatureIndex) {
        if (*from.requiredFeatureIndex >= r->features.size()) {
          return nullptr;
        }
        converted->requiredFeature = r->features[*from.requiredFeatureIndex];
      }
      for (auto const &index : from.featureIndices) {
        if (index >= r->features.size()) {
          return nullptr;
        }
        converted->features.push_back(r->features[index]);
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
            return nullptr;
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
            return nullptr;
          }
          script.langSysTable.push_back(make_pair(langSysTag, converted));
          convertedLangSysList[langSys] = converted;
        } else {
          script.langSysTable.push_back(make_pair(langSysTag, found->second));
        }
      }
      r->scripts.push_back(script);
    }

    return r;
  }

  std::optional<EncodeResult> encode() const override {
    using namespace std;
    return nullopt;
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
