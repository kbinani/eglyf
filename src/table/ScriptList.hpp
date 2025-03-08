#pragma once

namespace eglyf {

class ScriptList {
public:
  struct LangSys {
    std::optional<uint16_t> requiredFeatureIndex;
    std::vector<uint16_t> featureIndices;

    static std::shared_ptr<LangSys> Read(InputStream &in) {
      using namespace std;
      Offset16 lookupOrderOffset;
      if (!in.o16(&lookupOrderOffset)) {
        return nullptr;
      }
      auto langSys = make_shared<LangSys>();
      uint16_t requiredFeatureIndex;
      if (!in.u16(&requiredFeatureIndex)) {
        return nullptr;
      }
      if (requiredFeatureIndex != 0xffff) {
        langSys->requiredFeatureIndex = requiredFeatureIndex;
      }
      uint16_t featureIndexCount;
      if (!in.u16(&featureIndexCount)) {
        return nullptr;
      }
      langSys->featureIndices.reserve(featureIndexCount);
      for (uint16_t i = 0; i < featureIndexCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullptr;
        }
        langSys->featureIndices.push_back(v);
      }
      return langSys;
    }
  };

  struct Script {
    Tag tag;
    std::shared_ptr<LangSys> defaultLangSys;
    std::vector<std::pair<Tag, std::shared_ptr<LangSys>>> langSysTable;
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
      map<Offset16, shared_ptr<LangSys>> langSysList;
      if (!in.seek(scriptOffset)) {
        return nullopt;
      }
      OffsetInputStream sub(in);
      Script script;
      script.tag = scriptTag;
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
        if (auto found = langSysList.find(defaultLangSysOffset); found == langSysList.end()) {
          if (!sub.seek(defaultLangSysOffset)) {
            return nullopt;
          }
          auto defaultLangSys = LangSys::Read(sub);
          if (!defaultLangSys) {
            return nullopt;
          }
          script.defaultLangSys = defaultLangSys;
          langSysList[defaultLangSysOffset] = defaultLangSys;
        } else {
          script.defaultLangSys = found->second;
        }
      }
      for (auto [langSysTag, langSysOffset] : langSysOffsetList) {
        if (auto found = langSysList.find(langSysOffset); found == langSysList.end()) {
          if (!sub.seek(langSysOffset)) {
            return nullopt;
          }
          auto langSys = LangSys::Read(sub);
          if (!langSys) {
            return nullopt;
          }
          script.langSysTable.push_back(make_pair(langSysTag, langSys));
          langSysList[langSysOffset] = langSys;
        } else {
          script.langSysTable.push_back(make_pair(langSysTag, found->second));
        }
      }
      scriptList.scriptTable.push_back(script);
    }
    return scriptList;
  }

  bool write(OutputStream &out) {
    using namespace std;
    auto scriptListBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(scriptTable.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> scriptTableHandles;
    for (auto const &script : scriptTable) {
      if (!out.write(script.tag.data(), script.tag.size())) {
        return false;
      }
      auto handle = scriptListBeginning->o16();
      if (!handle) {
        return false;
      }
      scriptTableHandles.push_back(handle);
    }
    map<shared_ptr<LangSys>, vector<OffsetWriter::Handle16>> langSysHandles;
    vector<shared_ptr<OffsetWriter>> langSysWriters;
    for (size_t i = 0; i < scriptTable.size(); i++) {
      auto const &script = scriptTable[i];
      auto scriptTableHandle = scriptTableHandles[i];
      if (!scriptTableHandle->mark()) {
        return false;
      }
      auto scriptTableBeginning = make_shared<OffsetWriter>(out);
      langSysWriters.push_back(scriptTableBeginning);
      if (script.defaultLangSys) {
        auto handle = scriptTableBeginning->o16();
        if (!handle) {
          return false;
        }
        langSysHandles[script.defaultLangSys].push_back(handle);
      } else {
        if (!out.o16(0)) {
          return false;
        }
      }
      if (!out.sizeU16(script.langSysTable.size())) {
        return false;
      }
      for (auto const &[tag, langSys] : script.langSysTable) {
        if (!out.write(tag.data(), tag.size())) {
          return false;
        }
        auto handle = scriptTableBeginning->o16();
        if (!handle) {
          return false;
        }
        langSysHandles[langSys].push_back(handle);
      }
    }
    for (auto const &[langSys, handles] : langSysHandles) {
      for (auto &handle : handles) {
        if (!handle->mark()) {
          return false;
        }
      }
      // lookupOrderOffset (Reserved, set to NULL)
      if (!out.o16(0)) {
        return false;
      }
      if (langSys->requiredFeatureIndex) {
        if (!out.u16(*langSys->requiredFeatureIndex)) {
          return false;
        }
      } else {
        if (!out.u16(0xffff)) {
          return false;
        }
      }
      if (!out.sizeU16(langSys->featureIndices.size())) {
        return false;
      }
      for (uint16_t index : langSys->featureIndices) {
        if (!out.u16(index)) {
          return false;
        }
      }
    }
    for (auto &writer : langSysWriters) {
      if (!writer->commit()) {
        return false;
      }
    }
    return scriptListBeginning->commit();
  }

public:
  std::vector<Script> scriptTable;
};

} // namespace eglyf
