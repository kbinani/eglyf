#pragma once

namespace eglyf::gpos {

class CursiveAttachment : public Subtable {
public:
  struct EntryExit {
    std::shared_ptr<Anchor> entryAnchor;
    std::shared_ptr<Anchor> exitAnchor;
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t entryExitCount;
    if (!in.u16(&entryExitCount)) {
      return EGLYF_ERROR;
    }
    vector<pair<Offset16, Offset16>> offsets;
    for (uint16_t i = 0; i < entryExitCount; i++) {
      Offset16 entry;
      Offset16 exit;
      if (!in.o16(&entry)) {
        return EGLYF_ERROR;
      }
      if (!in.o16(&exit)) {
        return EGLYF_ERROR;
      }
      offsets.emplace_back(entry, exit);
    }
    auto ret = make_unique<CursiveAttachment>();
    for (auto [entry, exit] : offsets) {
      EntryExit record;
      if (entry > 0) {
        if (!in.seek(entry)) {
          return EGLYF_ERROR;
        }
        if (auto st = AnchorReader::Read(in, record.entryAnchor); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (exit > 0) {
        if (!in.seek(exit)) {
          return EGLYF_ERROR;
        }
        if (auto st = AnchorReader::Read(in, record.exitAnchor); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      ret->entryExitRecords.push_back(record);
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &stream, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<DataFragmentWriter>(&stream);
    if (!writer->u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!writer->sizeU16(entryExitRecords.size())) {
      return EGLYF_ERROR;
    }
    vector<pair<DataFragmentWriter::Marker16, DataFragmentWriter::Marker16>> offsets;
    for (auto const &record : entryExitRecords) {
      auto entry = writer->o16();
      if (!entry) {
        return EGLYF_ERROR;
      }
      auto exit = writer->o16();
      if (!exit) {
        return EGLYF_ERROR;
      }
      offsets.emplace_back(entry, exit);
    }
    for (size_t i = 0; i < entryExitRecords.size(); i++) {
      auto const &record = entryExitRecords[i];
      auto [entry, exit] = offsets[i];
      if (record.entryAnchor) {
        if (auto st = writer->writeDataFragment(entry, *record.entryAnchor); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (record.exitAnchor) {
        if (auto st = writer->writeDataFragment(exit, *record.exitAnchor); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }
    if (auto st = writer->writeDataFragment(coverageOffset, *coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t) + sizeof(Offset16);
    ret += coverage->size();
    ret += (2 * sizeof(Offset16)) * entryExitRecords.size();
    for (auto const &record : entryExitRecords) {
      if (record.entryAnchor) {
        ret += record.entryAnchor->size();
      }
      if (record.exitAnchor) {
        ret += record.exitAnchor->size();
      }
    }
    return ret;
  }

public:
  std::vector<EntryExit> entryExitRecords;
};

} // namespace eglyf::gpos
