#pragma once

namespace eglyf {

class GlyphSubstitutionTable : public Table {
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

    {
      if (!in.seek(scriptListOffset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto sl = ScriptList::Read(sub); sl) {
        r->scriptList = *sl;
      } else {
        return nullptr;
      }
    }
    {
      if (!in.seek(featureListOffset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto fl = FeatureList::Read(sub); fl) {
        r->featureList = *fl;
      } else {
        return nullptr;
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

  ScriptList scriptList;
  FeatureList featureList;
};

} // namespace eglyf
