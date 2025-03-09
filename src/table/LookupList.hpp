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
  static Optional<LookupList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    LookupList ret;
    uint16_t lookupCount;
    if (!in.u16(&lookupCount)) {
      return EGLYF_NULLOPT;
    }
    vector<Offset16> lookupOffsets;
    if (!in.o16a(lookupOffsets, lookupCount)) {
      return EGLYF_NULLOPT;
    }
    for (uint16_t lookupOffset : lookupOffsets) {
      Lookup l;
      if (!in.seek(lookupOffset)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&l.lookupType)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&l.lookupFlag)) {
        return EGLYF_NULLOPT;
      }
      uint16_t subTableCount;
      if (!in.u16(&subTableCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.o16a(l.subtableOffsets, subTableCount)) {
        return EGLYF_NULLOPT;
      }
      for (size_t i = 0; i < l.subtableOffsets.size(); i++) {
        l.subtableOffsets[i] += lookupOffset;
      }
      if (!in.u16(&l.markFilteringSet)) {
        return EGLYF_NULLOPT;
      }
      ret.lookupTable.push_back(l);
    }
    return ret;
  }

  template <class Subtable>
    requires requires(Subtable &t, OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) {
      { t.write(out, extensions) } -> std::convertible_to<Status>;
    }
  Status write(OutputStream &out, std::vector<std::vector<std::shared_ptr<Subtable>>> const &subtables) {
    using namespace std;

    if (lookupTable.size() != subtables.size()) {
      return EGLYF_ERROR;
    }
    for (size_t i = 0; i < lookupTable.size(); i++) {
      if (lookupTable[i].subtableOffsets.size() != subtables[i].size()) {
        return EGLYF_ERROR;
      }
    }

    map<shared_ptr<Subtable>, vector<OffsetWriter::Handle16>> handles;
    vector<shared_ptr<OffsetWriter>> writers;

    auto lookupListBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(lookupTable.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> lookupOffsets;
    for (auto const &lookup : lookupTable) {
      auto handle = lookupListBeginning->o16();
      if (!handle) {
        return EGLYF_ERROR;
      }
      lookupOffsets.push_back(handle);
    }

    for (size_t i = 0; i < lookupTable.size(); i++) {
      auto lookupTableBeginning = make_shared<OffsetWriter>(out);
      writers.push_back(lookupTableBeginning);

      auto const &lookup = lookupTable[i];
      auto tables = subtables[i];

      auto handle = lookupOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (!out.u16(lookup.lookupType)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(lookup.lookupFlag)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(tables.size())) {
        return EGLYF_ERROR;
      }
      for (auto const &table : tables) {
        auto h = lookupTableBeginning->o16();
        if (!h) {
          return EGLYF_ERROR;
        }
        handles[table].push_back(h);
      }
      if (!out.u16(lookup.markFilteringSet)) {
        return EGLYF_ERROR;
      }
    }

    map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> extensions;
    for (auto const &[table, handleList] : handles) {
      for (auto handle : handleList) {
        if (auto st = handle->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (auto st = table->write(out, extensions); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (auto [table, p] : extensions) {
      auto [writer, offset] = p;
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> ex;
      if (auto st = table->write(out, ex); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (!ex.empty()) {
        return EGLYF_ERROR;
      }
    }
    for (auto [table, p] : extensions) {
      auto [writer, offset] = p;
      if (auto st = writer->commit(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (auto st = lookupListBeginning->commit(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (auto &writer : writers) {
      if (auto st = writer->commit(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return Status::Ok();
  }

public:
  std::vector<Lookup> lookupTable;
};

} // namespace eglyf
