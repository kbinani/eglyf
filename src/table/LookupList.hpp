#pragma once

namespace eglyf {

class LookupList {
public:
  struct Lookup {
    uint16_t lookupType;
    uint16_t lookupFlag;
    Offset16 lookupOffset;
    std::vector<Offset16> subtableOffsets;
    uint16_t markFilteringSet;
  };

public:
  static Optional<LookupList> Read(InputStream &stream) {
    using namespace std;
    OffsetInputStream in(&stream);
    LookupList ret;
    uint16_t lookupCount;
    if (!in.u16(&lookupCount)) {
      return EGLYF_NULLOPT;
    }
    vector<Offset16> lookupOffsets;
    if (!in.o16a(lookupOffsets, lookupCount)) {
      return EGLYF_NULLOPT;
    }
    map<Offset16, shared_ptr<Lookup>> lookups;
    for (uint16_t lookupOffset : lookupOffsets) {
      auto found = lookups.find(lookupOffset);
      if (found != lookups.end()) {
        ret.lookupTable.push_back(found->second);
        continue;
      }
      auto l = make_shared<Lookup>();
      l->lookupOffset = lookupOffset;
      if (!in.seek(lookupOffset)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&l->lookupType)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&l->lookupFlag)) {
        return EGLYF_NULLOPT;
      }
      uint16_t subTableCount;
      if (!in.u16(&subTableCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.o16a(l->subtableOffsets, subTableCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&l->markFilteringSet)) {
        return EGLYF_NULLOPT;
      }
      lookups[lookupOffset] = l;
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
      if (lookupTable[i]->subtableOffsets.size() != subtables[i].size()) {
        return EGLYF_ERROR;
      }
    }

    deque<pair<shared_ptr<Subtable>, vector<OffsetWriter::Handle16>>> handles;
    vector<shared_ptr<OffsetWriter>> writers;

    auto lookupListBeginning = make_shared<OffsetWriter>(out);
    if (!out.sizeU16(lookupTable.size())) {
      return EGLYF_ERROR;
    }
    map<shared_ptr<Lookup>, pair<vector<shared_ptr<Subtable>>, vector<OffsetWriter::Handle16>>> lookupOffsets;
    for (size_t i = 0; i < lookupTable.size(); i++) {
      auto const &lookup = lookupTable[i];
      auto const &tables = subtables[i];
      auto handle = lookupListBeginning->o16();
      if (!handle) {
        return EGLYF_ERROR;
      }
      if (lookupOffsets[lookup].first.empty()) {
        lookupOffsets[lookup].first = tables;
      } else {
        if (lookupOffsets[lookup].first.size() != tables.size()) {
          return EGLYF_ERROR;
        }
        for (size_t j = 0; j < tables.size(); j++) {
          if (tables[j] != lookupOffsets[lookup].first[j]) {
            return EGLYF_ERROR;
          }
        }
      }
      lookupOffsets[lookup].second.push_back(handle);
    }

    for (auto [lookup, tableAndOffsets] : lookupOffsets) {
      auto lookupTableBeginning = make_shared<OffsetWriter>(out);
      writers.push_back(lookupTableBeginning);

      auto [tables, offsets] = tableAndOffsets;

      for (auto offset : offsets) {
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }

      if (!out.u16(lookup->lookupType)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(lookup->lookupFlag)) {
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
        auto found = ranges::find_if(handles, [&table](auto const &it) { return it.first == table; });
        if (found == handles.end()) {
          vector<OffsetWriter::Handle16> hs;
          hs.push_back(h);
          handles.push_back(make_pair(table, hs));
        } else {
          found->second.push_back(h);
        }
      }
      if (!out.u16(lookup->markFilteringSet)) {
        return EGLYF_ERROR;
      }
    }

    map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> extensions;
    while (!handles.empty()) {
      ranges::sort(handles, [](auto const &a, auto const &b) -> bool {
        static auto Distance = [](pair<shared_ptr<Subtable>, vector<OffsetWriter::Handle16>> const &it) -> int64_t {
          int64_t minOffset = numeric_limits<uint32_t>::max();
          for (auto const &offset : it.second) {
            auto current = offset->current();
            if (current) {
              minOffset = (std::min)(minOffset, *current);
            }
          }
          size_t size = it.first->size();
          return minOffset + size;
        };
        int64_t distanceA = Distance(a);
        int64_t distanceB = Distance(b);
        return distanceA < distanceB;
      });
      auto [table, handleList] = handles.front();

      for (auto handle : handleList) {
        if (auto st = handle->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      auto pos = out.position();
      if (auto st = table->write(out, extensions); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      handles.pop_front();
    }

    for (auto [table, p] : extensions) {
      auto [writer, offset] = p;
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      map<shared_ptr<Subtable>, pair<shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> ex;
      auto pos = out.position();
      if (auto st = table->write(out, ex); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      jassert(pos + table->size() == out.position());
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
  std::vector<std::shared_ptr<Lookup>> lookupTable;
};

} // namespace eglyf
