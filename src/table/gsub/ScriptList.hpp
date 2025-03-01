#pragma once

namespace eglyf {

class ScriptList {
public:
  struct LangSys {
    std::optional<uint16_t> requiredFeatureIndex;
    std::vector<uint16_t> featureIndices;

    static std::optional<LangSys> Read(InputStream &in) {
      using namespace std;
      Offset16 lookupOrderOffset;
      if (!in.o16(&lookupOrderOffset)) {
        return nullopt;
      }
      LangSys langSys;
      uint16_t requiredFeatureIndex;
      if (!in.u16(&requiredFeatureIndex)) {
        return nullopt;
      }
      if (requiredFeatureIndex != 0xffff) {
        langSys.requiredFeatureIndex = requiredFeatureIndex;
      }
      uint16_t featureIndexCount;
      if (!in.u16(&featureIndexCount)) {
        return nullopt;
      }
      langSys.featureIndices.reserve(featureIndexCount);
      for (uint16_t i = 0; i < featureIndexCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        langSys.featureIndices.push_back(v);
      }
      return langSys;
    }
  };

  struct Script {
    std::optional<LangSys> defaultLangSys;
    std::map<std::array<uint8_t, 4>, LangSys> langSysTable;
  };

public:
  static std::optional<ScriptList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);

    ScriptList scriptList;
    uint16_t scriptCount;
    if (!in.u16(&scriptCount)) {
      return nullopt;
    }
    vector<pair<Tag, Offset16>> scriptOffsetList;
    for (uint16_t i = 0; i < scriptCount; i++) {
      auto scriptTag = ReadTag(in);
      if (!scriptTag) {
        return nullopt;
      }
      Offset16 scriptOffset;
      if (!in.o16(&scriptOffset)) {
        return nullopt;
      }
      scriptOffsetList.push_back(make_pair(*scriptTag, scriptOffset));
    }
    for (auto [scriptTag, scriptOffset] : scriptOffsetList) {
      if (!in.seek(scriptOffset)) {
        return nullopt;
      }
      OffsetInputStream sub(in);
      Script script;
      Offset16 defaultLangSysOffset;
      if (!sub.o16(&defaultLangSysOffset)) {
        return nullopt;
      }
      uint16_t langSysCount;
      if (!in.u16(&langSysCount)) {
        return nullopt;
      }
      vector<pair<Tag, Offset16>> langSysOffsetList;
      for (uint16_t j = 0; j < langSysCount; j++) {
        auto langSysTag = ReadTag(in);
        if (!langSysTag) {
          return nullopt;
        }
        Offset16 langSysOffset;
        if (!in.o16(&langSysOffset)) {
          return nullopt;
        }
        langSysOffsetList.push_back(make_pair(*langSysTag, langSysOffset));
      }
      if (defaultLangSysOffset > 0) {
        if (!sub.seek(defaultLangSysOffset)) {
          return nullopt;
        }
        auto defaultLangSys = LangSys::Read(sub);
        if (!defaultLangSys) {
          return nullopt;
        }
        script.defaultLangSys = *defaultLangSys;
      }
      for (auto [langSysTag, langSysOffset] : langSysOffsetList) {
        if (!sub.seek(langSysOffset)) {
          return nullopt;
        }
        auto langSys = LangSys::Read(sub);
        if (!langSys) {
          return nullopt;
        }
        script.langSysTable[langSysTag] = *langSys;
      }
      scriptList.scriptTable[scriptTag] = script;
    }
    return scriptList;
  }

public:
  std::map<std::array<uint8_t, 4>, Script> scriptTable;
};

} // namespace eglyf
