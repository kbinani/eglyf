#pragma once

namespace eglyf {

class LookupList {
public:
  struct Lookup {
    uint16_t lookupType;
    uint16_t lookupFlag;
    std::vector<Offset16> subtableOffsets;
    uint16_t markFilteringSet;
  };

public:
  static std::optional<LookupList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    LookupList ret;
    uint16_t lookupCount;
    if (!in.u16(&lookupCount)) {
      return nullopt;
    }
    vector<Offset16> lookupOffsets;
    lookupOffsets.reserve(lookupCount);
    for (uint16_t i = 0; i < lookupCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullopt;
      }
      lookupOffsets.push_back(v);
    }
    for (uint16_t lookupOffset : lookupOffsets) {
      Lookup l;
      if (!in.seek(lookupOffset)) {
        return nullopt;
      }
      if (!in.u16(&l.lookupType)) {
        return nullopt;
      }
      if (!in.u16(&l.lookupFlag)) {
        return nullopt;
      }
      uint16_t subTableCount;
      if (!in.u16(&subTableCount)) {
        return nullopt;
      }
      l.subtableOffsets.reserve(subTableCount);
      for (uint16_t i = 0; i < subTableCount; i++) {
        Offset16 v;
        if (!in.o16(&v)) {
          return nullopt;
        }
        l.subtableOffsets.push_back(lookupOffset + v);
      }
      if (!in.u16(&l.markFilteringSet)) {
        return nullopt;
      }
      ret.lookupTable.push_back(l);
    }
    return ret;
  }

  template <class Subtable>
    requires requires(Subtable &t, OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) {
      { t.write(out, extensions) } -> std::convertible_to<bool>;
    }
  bool write(OutputStream &out, std::vector<std::vector<std::shared_ptr<Subtable>>> const &subtables) {
    using namespace std;

    if (lookupTable.size() != subtables.size()) {
      return false;
    }
    for (size_t i = 0; i < lookupTable.size(); i++) {
      if (lookupTable[i].subtableOffsets.size() != subtables[i].size()) {
        return false;
      }
    }

    map<shared_ptr<Subtable>, vector<OffsetWriter::Handle16>> handles;
    vector<shared_ptr<OffsetWriter>> writers;

    auto lookupListBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(lookupTable.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> lookupOffsets;
    for (auto const &lookup : lookupTable) {
      auto handle = lookupListBeginning->o16();
      if (!handle) {
        return false;
      }
      lookupOffsets.push_back(handle);
    }

    for (size_t i = 0; i < lookupTable.size(); i++) {
      auto lookupTableBeginning = make_shared<OffsetWriter>(out);
      writers.push_back(lookupTableBeginning);

      auto const &lookup = lookupTable[i];
      auto tables = subtables[i];

      auto handle = lookupOffsets[i];
      if (!handle->mark()) {
        return false;
      }

      if (!out.u16(lookup.lookupType)) {
        return false;
      }
      if (!out.u16(lookup.lookupFlag)) {
        return false;
      }
      if (!out.sizeU16(tables.size())) {
        return false;
      }
      for (auto const &table : tables) {
        auto h = lookupTableBeginning->o16();
        if (!h) {
          return false;
        }
        handles[table].push_back(h);
      }
      if (!out.u16(lookup.markFilteringSet)) {
        return false;
      }
    }

    map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> extensions;
    for (auto const &[table, handleList] : handles) {
      for (auto handle : handleList) {
        if (!handle->mark()) {
          return false;
        }
      }
      if (!table->write(out, extensions)) {
        return false;
      }
    }

    for (auto [table, p] : extensions) {
      auto [writer, offset] = p;
      if (!offset->mark()) {
        return false;
      }
      map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> ex;
      if (!table->write(out, ex)) {
        return false;
      }
      if (!ex.empty()) {
        return false;
      }
    }
    for (auto [table, p] : extensions) {
      auto [writer, offset] = p;
      if (!writer->commit()) {
        return false;
      }
    }

    if (!lookupListBeginning->commit()) {
      return false;
    }
    for (auto &writer : writers) {
      if (!writer->commit()) {
        return false;
      }
    }

    return true;
  }

public:
  std::vector<Lookup> lookupTable;
};

} // namespace eglyf
