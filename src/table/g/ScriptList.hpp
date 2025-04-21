#pragma once

namespace eglyf {

class ScriptList {
public:
  struct LangSys {
    std::optional<uint16_t> requiredFeatureIndex;
    std::vector<uint16_t> featureIndices;

    static Status Read(InputStream &in, std::shared_ptr<LangSys> &out) {
      using namespace std;
      Offset16 lookupOrderOffset;
      if (!in.o16(&lookupOrderOffset)) {
        return EGLYF_ERROR_WHAT("Failed to read lookupOrderOffset");
      }
      auto langSys = make_unique<LangSys>();
      uint16_t requiredFeatureIndex;
      if (!in.u16(&requiredFeatureIndex)) {
        return EGLYF_ERROR_WHAT("Failed to read requiredFeatureIndex");
      }
      if (requiredFeatureIndex != 0xffff) {
        langSys->requiredFeatureIndex = requiredFeatureIndex;
      }
      uint16_t featureIndexCount;
      if (!in.u16(&featureIndexCount)) {
        return EGLYF_ERROR_WHAT("Failed to read featureIndexCount");
      }
      if (!in.u16a(langSys->featureIndices, featureIndexCount)) {
        return EGLYF_ERROR_WHAT("Failed to read featureIndices");
      }
      out.reset(langSys.release());
      return Status::Ok();
    }
  };

  struct Script {
    Tag tag;
    std::shared_ptr<LangSys> defaultLangSys;
    std::vector<std::pair<Tag, std::shared_ptr<LangSys>>> langSysTable;
  };

public:
  static Optional<ScriptList> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);

    ScriptList scriptList;
    uint16_t scriptCount;
    if (!in.u16(&scriptCount)) {
      return EGLYF_NULLOPT_WHAT("Failed to read scriptCount");
    }
    vector<pair<Tag, Offset16>> scriptOffsetList;
    for (uint16_t i = 0; i < scriptCount; i++) {
      auto scriptTag = ReadTag(in);
      if (!scriptTag) {
        return EGLYF_NULLOPT_WHAT("Failed to read scriptTag");
      }
      Offset16 scriptOffset;
      if (!in.o16(&scriptOffset)) {
        return EGLYF_NULLOPT_WHAT("Failed to read scriptOffset");
      }
      scriptOffsetList.push_back(make_pair(*scriptTag, scriptOffset));
    }
    for (auto [scriptTag, scriptOffset] : scriptOffsetList) {
      map<Offset16, shared_ptr<LangSys>> langSysList;
      if (!in.seek(scriptOffset)) {
        return EGLYF_NULLOPT_WHAT("Failed to seek to scriptOffset");
      }
      OffsetInputStream sub(&in);
      Script script;
      script.tag = scriptTag;
      Offset16 defaultLangSysOffset;
      if (!sub.o16(&defaultLangSysOffset)) {
        return EGLYF_NULLOPT_WHAT("Failed to read defaultLangSysOffset");
      }
      uint16_t langSysCount;
      if (!in.u16(&langSysCount)) {
        return EGLYF_NULLOPT_WHAT("Failed to read langSysCount");
      }
      vector<pair<Tag, Offset16>> langSysOffsetList;
      for (uint16_t j = 0; j < langSysCount; j++) {
        auto langSysTag = ReadTag(in);
        if (!langSysTag) {
          return EGLYF_NULLOPT_WHAT("Failed to read langSysTag");
        }
        Offset16 langSysOffset;
        if (!in.o16(&langSysOffset)) {
          return EGLYF_NULLOPT_WHAT("Failed to read langSysOffset");
        }
        langSysOffsetList.push_back(make_pair(*langSysTag, langSysOffset));
      }
      if (defaultLangSysOffset > 0) {
        if (auto found = langSysList.find(defaultLangSysOffset); found == langSysList.end()) {
          if (!sub.seek(defaultLangSysOffset)) {
            return EGLYF_NULLOPT_WHAT("Failed to seek to defaultLangSysOffset");
          }
          shared_ptr<LangSys> defaultLangSys;
          if (auto st = LangSys::Read(sub, defaultLangSys); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
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
            return EGLYF_NULLOPT_WHAT("Failed to seek to langSysOffset");
          }
          shared_ptr<LangSys> langSys;
          if (auto st = LangSys::Read(sub, langSys); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
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

  Status write(OutputStream &out) {
    using namespace std;
    auto scriptListBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(scriptTable.size())) {
      return EGLYF_ERROR_WHAT("Failed to write script table size");
    }
    vector<OffsetWriter::Handle16> scriptTableHandles;
    for (auto const &script : scriptTable) {
      if (!out.write(script.tag.data(), script.tag.size())) {
        return EGLYF_ERROR_WHAT("Failed to write script tag");
      }
      auto handle = scriptListBeginning->o16();
      if (!handle) {
        return EGLYF_ERROR_WHAT("Failed to create script offset handle");
      }
      scriptTableHandles.push_back(handle);
    }
    map<shared_ptr<LangSys>, vector<OffsetWriter::Handle16>> langSysHandles;
    vector<shared_ptr<OffsetWriter>> langSysWriters;
    for (size_t i = 0; i < scriptTable.size(); i++) {
      auto const &script = scriptTable[i];
      auto scriptTableHandle = scriptTableHandles[i];
      if (auto st = scriptTableHandle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      auto scriptTableBeginning = make_shared<OffsetWriter>(out);
      langSysWriters.push_back(scriptTableBeginning);
      if (script.defaultLangSys) {
        auto handle = scriptTableBeginning->o16();
        if (!handle) {
          return EGLYF_ERROR_WHAT("Failed to create defaultLangSys offset handle");
        }
        langSysHandles[script.defaultLangSys].push_back(handle);
      } else {
        if (!out.o16(0)) {
          return EGLYF_ERROR_WHAT("Failed to write zero defaultLangSys offset");
        }
      }
      if (!out.sizeU16(script.langSysTable.size())) {
        return EGLYF_ERROR_WHAT("Failed to write langSys table size");
      }
      for (auto const &[tag, langSys] : script.langSysTable) {
        if (!out.write(tag.data(), tag.size())) {
          return EGLYF_ERROR_WHAT("Failed to write langSys tag");
        }
        auto handle = scriptTableBeginning->o16();
        if (!handle) {
          return EGLYF_ERROR_WHAT("Failed to create langSys offset handle");
        }
        langSysHandles[langSys].push_back(handle);
      }
    }
    for (auto const &[langSys, handles] : langSysHandles) {
      for (auto &handle : handles) {
        if (auto st = handle->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      // lookupOrderOffset (Reserved, set to NULL)
      if (!out.o16(0)) {
        return EGLYF_ERROR_WHAT("Failed to write lookupOrderOffset");
      }
      if (langSys->requiredFeatureIndex) {
        if (!out.u16(*langSys->requiredFeatureIndex)) {
          return EGLYF_ERROR_WHAT("Failed to write requiredFeatureIndex");
        }
      } else {
        if (!out.u16(0xffff)) {
          return EGLYF_ERROR_WHAT("Failed to write 0xffff for no required feature");
        }
      }
      if (!out.sizeU16(langSys->featureIndices.size())) {
        return EGLYF_ERROR_WHAT("Failed to write feature indices size");
      }
      if (!out.u16a(langSys->featureIndices)) {
        return EGLYF_ERROR_WHAT("Failed to write feature indices");
      }
    }
    for (auto &writer : langSysWriters) {
      if (auto st = writer->commit(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(scriptListBeginning->commit());
  }

public:
  std::vector<Script> scriptTable;
};

} // namespace eglyf
