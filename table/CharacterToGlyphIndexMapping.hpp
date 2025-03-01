#pragma once

namespace ksesh {

// 'cmap'
class CharacterToGlyphIndexMapping : public Table {
public:
  struct EncodingRecord {
    uint16_t platformID;
    uint16_t encodingID;
    Offset32 subtableOffset;

    static std::optional<EncodingRecord> Read(InputStream &in) {
      using namespace std;
      EncodingRecord r;
      if (!in.u16(&r.platformID)) {
        return nullopt;
      }
      if (!in.u16(&r.encodingID)) {
        return nullopt;
      }
      if (!in.o32(&r.subtableOffset)) {
        return nullopt;
      }
      return r;
    }
  };
  struct Header {
    uint16_t version;
    uint16_t numTables;
    std::vector<EncodingRecord> encodingRecords;

    static std::optional<Header> Read(InputStream &in) {
      using namespace std;
      Header h;
      if (!in.u16(&h.version)) {
        return nullopt;
      }
      if (!in.u16(&h.numTables)) {
        return nullopt;
      }
      for (uint16_t i = 0; i < h.numTables; i++) {
        if (auto r = EncodingRecord::Read(in); r) {
          h.encodingRecords.push_back(*r);
        } else {
          return nullopt;
        }
      }
      return h;
    }
  };
  class Subtable {
  public:
    virtual ~Subtable() {}

    virtual std::optional<Offset32> write(OutputStream &out) = 0;
  };
  class ReadonlySubtable : public Subtable {
  public:
    std::optional<Offset32> write(OutputStream &out) override {
      using namespace std;
      switch (format) {
      case 0:
      case 2:
      case 4:
      case 6: {
        if (!out.u16(format)) {
          return nullopt;
        }
        uint16_t length = 4 + data.size();
        if (!out.u16(length)) {
          return nullopt;
        }
        if (!out.write(data.data(), data.size())) {
          return nullopt;
        }
        return 4 + data.size();
      }
      case 8:
      case 10:
      case 12:
      case 13:
      case 14: {
        if (!out.u16(format)) {
          return nullopt;
        }
        uint16_t reserved = 0;
        if (!out.u16(reserved)) {
          return nullopt;
        }
        Offset32 length = 8 + data.size();
        if (!out.o32(length)) {
          return nullopt;
        }
        if (!out.write(data.data(), data.size())) {
          return nullopt;
        }
        return 8 + data.size();
      }
      default:
        return nullopt;
      }
    }

    static std::shared_ptr<ReadonlySubtable> Read(InputStream &in) {
      using namespace std;
      auto r = make_shared<ReadonlySubtable>();
      if (!in.u16(&r->format)) {
        return nullptr;
      }
      switch (r->format) {
      case 0:
      case 2:
      case 4:
      case 6: {
        uint16_t length;
        if (!in.u16(&length)) {
          return nullptr;
        }
        if (length < 4) {
          return nullptr;
        }
        r->data.resize(length - 4);
        if (!in.read(r->data.data(), r->data.size())) {
          return nullptr;
        }
        break;
      }
      case 8:
      case 10:
      case 12:
      case 13:
      case 14: {
        uint16_t reserved;
        if (!in.u16(&reserved)) {
          return nullptr;
        }
        uint32_t length;
        if (!in.u32(&length)) {
          return nullptr;
        }
        if (length < 8) {
          return nullptr;
        }
        r->data.resize(length - 8);
        if (!in.read(r->data.data(), r->data.size())) {
          return nullptr;
        }
        break;
      }
      default:
        return nullptr;
      }
      return r;
    }

  public:
    uint16_t format;
    std::string data;
  };

  struct SubtableHolder {
    uint16_t platformID;
    uint16_t encodingID;
    std::shared_ptr<Subtable> subtable;
  };

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(header.version)) {
      return nullopt;
    }
    if (subtables.size() > numeric_limits<uint16_t>::max()) {
      return nullopt;
    }
    uint16_t const numTables = subtables.size();
    if (!out.u16(numTables)) {
      return nullopt;
    }
    array<uint8_t, 8> empty;
    for (uint16_t i = 0; i < numTables; i++) {
      if (!out.write(empty.data(), empty.size())) {
        return nullopt;
      }
    }
    Offset32 offset = 4 + numTables * 8;
    vector<EncodingRecord> records;
    map<shared_ptr<Subtable>, Offset32> written;
    for (size_t i = 0; i < subtables.size(); i++) {
      auto const &holder = subtables[i];
      EncodingRecord record;
      record.platformID = holder.platformID;
      record.encodingID = holder.encodingID;
      if (auto found = written.find(holder.subtable); found != written.end()) {
        record.subtableOffset = found->second;
        records.push_back(record);
        continue;
      }
      if (auto size = holder.subtable->write(out); size) {
        written[holder.subtable] = offset;
        record.subtableOffset = offset;
        records.push_back(record);
        offset += *size;
      } else {
        return nullopt;
      }
    }
    if (!out.seek(4)) {
      return nullopt;
    }
    for (auto const &record : records) {
      if (!out.u16(record.platformID)) {
        return nullopt;
      }
      if (!out.u16(record.encodingID)) {
        return nullopt;
      }
      if (!out.o32(record.subtableOffset)) {
        return nullopt;
      }
    }
    return EncodeResult(out.data());
  }

  static std::shared_ptr<CharacterToGlyphIndexMapping> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<CharacterToGlyphIndexMapping>();
    if (auto h = Header::Read(in); h) {
      r->header = *h;
    } else {
      return nullptr;
    }
    map<Offset32, shared_ptr<Subtable>> read;
    for (auto const &record : r->header.encodingRecords) {
      SubtableHolder holder;
      holder.platformID = record.platformID;
      holder.encodingID = record.encodingID;
      if (auto found = read.find(record.subtableOffset); found != read.end()) {
        holder.subtable = found->second;
        r->subtables.push_back(holder);
        continue;
      }
      if (!in.seek(record.subtableOffset)) {
        return nullptr;
      }
      if (auto s = ReadonlySubtable::Read(in); s) {
        read[record.subtableOffset] = s;
        holder.subtable = s;
        r->subtables.push_back(holder);
      } else {
        return nullptr;
      }
    }
    return r;
  }

public:
  Header header;
  std::vector<SubtableHolder> subtables;
};

} // namespace ksesh
