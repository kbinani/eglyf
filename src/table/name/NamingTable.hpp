#pragma once

namespace eglyf::name {

class NamingTable : public Table {
public:
  struct NameRecord {
    uint16_t platformID;
    uint16_t encodingID;
    uint16_t languageID;
    uint16_t nameID;

    static Optional<std::pair<NameRecord, std::string>> Read(InputStream &in, Offset16 storageOffset) {
      using namespace std;
      NameRecord r;
      if (!in.u16(&r.platformID)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.encodingID)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.languageID)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.nameID)) {
        return EGLYF_NULLOPT;
      }
      uint16_t length;
      if (!in.u16(&length)) {
        return EGLYF_NULLOPT;
      }
      Offset16 stringOffset;
      if (!in.o16(&stringOffset)) {
        return EGLYF_NULLOPT;
      }
      auto pos = in.position();
      if (!in.seek(storageOffset + stringOffset)) {
        return EGLYF_NULLOPT;
      }
      string data;
      data.resize(length);
      if (!in.read(data.data(), length)) {
        return EGLYF_NULLOPT;
      }
      if (!in.seek(pos)) {
        return EGLYF_NULLOPT;
      }
      return make_pair(r, data);
    }

    auto operator<=>(NameRecord const &other) const = default;
  };

  static Status Read(InputStream &stream, std::shared_ptr<NamingTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 0 && format != 1) {
      return EGLYF_ERROR;
    }
    uint16_t count;
    if (!in.u16(&count)) {
      return EGLYF_ERROR;
    }
    Offset16 storageOffset;
    if (!in.o16(&storageOffset)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<NamingTable>();
    for (uint16_t i = 0; i < count; i++) {
      auto record = NameRecord::Read(in, storageOffset);
      if (!record) {
        return EGLYF_STATUS_PUSH(record.status());
      }
      ret->records.insert(*record);
    }
    if (format == 1) {
      uint16_t langTagCount;
      if (!in.u16(&langTagCount)) {
        return EGLYF_ERROR;
      }
      for (uint16_t i = 0; i < langTagCount; i++) {
        uint16_t length;
        if (!in.u16(&length)) {
          return EGLYF_ERROR;
        }
        Offset16 langTagOffset;
        if (!in.u16(&langTagOffset)) {
          return EGLYF_ERROR;
        }
        auto pos = in.position();
        if (!in.seek(storageOffset + langTagOffset)) {
          return EGLYF_ERROR;
        }
        u16string tag;
        for (int i = 0; i < length; i += 2) {
          uint16_t u;
          if (!in.u16(&u)) {
            return EGLYF_ERROR;
          }
          char16_t ch = *(char16_t *)&u;
          tag.push_back(ch);
        }
        if (!in.seek(pos)) {
          return EGLYF_ERROR;
        }
        ret->langTags.push_back(tag);
      }
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (langTags.empty()) {
      if (!out.u16(0)) {
        return EGLYF_NULLOPT;
      }
    } else {
      if (!out.u16(1)) {
        return EGLYF_NULLOPT;
      }
    }
    ByteOutputStream storage;
    map<string, Offset16> mapping;
    if (!out.sizeU16(records.size())) {
      return EGLYF_NULLOPT;
    }
    auto storageOffsetPos = out.position();
    if (!out.o16(0)) {
      return EGLYF_NULLOPT;
    }
    for (auto const &[record, data] : records) {
      if (!out.u16(record.platformID)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(record.encodingID)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(record.languageID)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(record.nameID)) {
        return EGLYF_NULLOPT;
      }
      if (!out.sizeU16(data.size())) {
        return EGLYF_NULLOPT;
      }
      auto found = mapping.find(data);
      if (found == mapping.end()) {
        auto offset = storage.position();
        if (offset > numeric_limits<Offset16>::max()) {
          return EGLYF_NULLOPT;
        }
        Offset16 offset16 = (Offset16)offset;
        if (!storage.write(data.data(), data.size())) {
          return EGLYF_NULLOPT;
        }
        if (!out.o16(offset16)) {
          return EGLYF_NULLOPT;
        }
        mapping[data] = offset16;
      } else {
        if (!out.o16(found->second)) {
          return EGLYF_NULLOPT;
        }
      }
    }
    if (!langTags.empty()) {
      if (!out.sizeU16(langTags.size())) {
        return EGLYF_NULLOPT;
      }
      for (auto const &tag : langTags) {
        if (!out.sizeU16(tag.size() * 2)) {
          return EGLYF_NULLOPT;
        }
        Offset16 offset = storage.position();
        if (!out.sizeU16(offset)) {
          return EGLYF_NULLOPT;
        }
        for (char16_t ch : tag) {
          uint16_t v = *(uint16_t *)&ch;
          if (!storage.u16(v)) {
            return EGLYF_NULLOPT;
          }
        }
      }
    }
    auto storagePos = out.position();
    if (storagePos > numeric_limits<Offset16>::max()) {
      return EGLYF_NULLOPT;
    }
    Offset16 storagePos16 = (Offset16)storagePos;

    string data = storage.data();
    if (!out.write(data.data(), data.size())) {
      return EGLYF_NULLOPT;
    }
    if (!out.seek(storageOffsetPos)) {
      return EGLYF_NULLOPT;
    }
    if (!out.o16(storagePos16)) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

public:
  std::map<NameRecord, std::string> records;
  std::deque<std::u16string> langTags;
};

} // namespace eglyf::name
