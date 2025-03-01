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
      Lookup l;
      for (auto offset : it.subtableOffsets) {
        if (!in.seek(lookupListOffset + offset)) {
          return nullptr;
        }
        OffsetInputStream sub(in);
        if (it.lookupType == 1) {
          // Single
          if (auto t = gsub::Single::Read(sub); t) {
            l.subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 2) {
          // Multiple
          return nullptr;
        } else if (it.lookupType == 3) {
          // Alternate
          return nullptr;
        } else if (it.lookupType == 4) {
          // Ligature
          if (auto t = gsub::Ligature::Read(sub); t) {
            l.subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 5) {
          // Contextual substitution
          return nullptr;
        } else if (it.lookupType == 6) {
          // Chained contexts substitution
          if (auto t = gsub::ChainedContextsSubstitution::Read(sub); t) {
            l.subtables.push_back(t);
          } else {
            return nullptr;
          }
        } else if (it.lookupType == 7) {
          // Substitituion extension
          if (auto t = gsub::SubstitutionExtension::Read(sub); t) {
            l.subtables.push_back(t);
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

  std::vector<Lookup> lookups;
};

} // namespace eglyf
