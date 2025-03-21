#pragma once

namespace eglyf::gpos {

class MarkToLigatureAttachmentPositioning : public Subtable {
public:
  struct ComponentRecord {
    std::vector<std::shared_ptr<Anchor>> ligatureAnchors;

    size_t size() const {
      size_t ret = ligatureAnchors.size() * sizeof(Offset16);
      for (auto const &anchor : ligatureAnchors) {
        if (anchor) {
          ret += anchor->size();
        }
      }
      return ret;
    }
  };

  struct LigatureAttach {
    std::vector<ComponentRecord> componentRecords;

    static Status Read(InputStream &stream, LigatureAttach &out, uint16_t markClassCount) {
      using namespace std;
      OffsetInputStream in(&stream);
      uint16_t componentCount;
      if (!in.u16(&componentCount)) {
        return EGLYF_ERROR;
      }
      vector<vector<Offset16>> ligatureAnchorsOffsets;
      for (uint16_t i = 0; i < componentCount; i++) {
        vector<Offset16> offsets;
        if (!in.o16a(offsets, markClassCount)) {
          return EGLYF_ERROR;
        }
        ligatureAnchorsOffsets.push_back(offsets);
      }
      for (auto const &offsets : ligatureAnchorsOffsets) {
        ComponentRecord record;
        for (auto offset : offsets) {
          if (offset == 0) {
            record.ligatureAnchors.push_back(nullptr);
            continue;
          }
          if (!in.seek(offset)) {
            return EGLYF_ERROR;
          }
          shared_ptr<Anchor> anchor;
          if (auto st = AnchorReader::Read(in, anchor); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          record.ligatureAnchors.push_back(anchor);
        }
        out.componentRecords.push_back(record);
      }
      return Status::Ok();
    }

    Status write(OutputStream &stream, uint16_t markClassCount) const {
      using namespace std;
      auto writer = make_shared<DataFragmentWriter>(&stream);
      if (!writer->sizeU16(componentRecords.size())) {
        return EGLYF_ERROR;
      }
      vector<vector<DataFragmentWriter::Marker16>> ligatureAnchorOffsetsList;
      for (auto const &record : componentRecords) {
        vector<DataFragmentWriter::Marker16> offsets;
        if (record.ligatureAnchors.size() != markClassCount) {
          return EGLYF_ERROR;
        }
        for (uint16_t i = 0; i < markClassCount; i++) {
          auto offset = writer->o16();
          if (!offset) {
            return EGLYF_ERROR;
          }
          offsets.push_back(offset);
        }
        ligatureAnchorOffsetsList.push_back(offsets);
      }
      for (size_t i = 0; i < componentRecords.size(); i++) {
        auto const &record = componentRecords[i];
        auto const &offsets = ligatureAnchorOffsetsList[i];
        for (size_t j = 0; j < record.ligatureAnchors.size(); j++) {
          auto const &anchor = record.ligatureAnchors[j];
          auto offset = offsets[j];
          if (anchor) {
            if (auto st = writer->writeDataFragment(offset, *anchor); !st.ok()) {
              return EGLYF_STATUS_PUSH(st);
            }
          }
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }

    size_t size() const {
      size_t ret = sizeof(uint16_t);
      for (auto const &record : componentRecords) {
        ret += record.size();
      }
      return ret;
    }
  };

  struct LigatureArray {
    std::vector<LigatureAttach> ligatureAttaches;

    static Status Read(InputStream &stream, LigatureArray &out, uint16_t markClassCount) {
      using namespace std;
      OffsetInputStream in(&stream);
      uint16_t ligatureCount;
      if (!in.u16(&ligatureCount)) {
        return EGLYF_ERROR;
      }
      vector<Offset16> ligatureAttachOffsets;
      if (!in.o16a(ligatureAttachOffsets, ligatureCount)) {
        return EGLYF_ERROR;
      }
      for (auto offset : ligatureAttachOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_ERROR;
        }
        LigatureAttach ligatureAttach;
        if (auto st = LigatureAttach::Read(in, ligatureAttach, markClassCount); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        out.ligatureAttaches.push_back(ligatureAttach);
      }
      return Status::Ok();
    }

    Status write(OutputStream &stream, uint16_t markClassCount) const {
      using namespace std;
      auto writer = make_shared<DataFragmentWriter>(&stream);
      if (!writer->sizeU16(ligatureAttaches.size())) {
        return EGLYF_ERROR;
      }
      vector<DataFragmentWriter::Marker16> ligatureAttachOffsets;
      for (size_t i = 0; i < ligatureAttaches.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        ligatureAttachOffsets.push_back(offset);
      }
      for (size_t i = 0; i < ligatureAttaches.size(); i++) {
        auto const &ligatureAttach = ligatureAttaches[i];
        auto offset = ligatureAttachOffsets[i];
        auto st = writer->writeDataFragment(offset, [&](OutputStream &o) {
          return EGLYF_STATUS_PUSH(ligatureAttach.write(o, markClassCount));
        });
        if (!st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }

    size_t size() const {
      size_t ret = sizeof(uint16_t) + ligatureAttaches.size() * sizeof(Offset16);
      for (auto const &attach : ligatureAttaches) {
        ret += attach.size();
      }
      return ret;
    }
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    Offset16 markCoverageOffset;
    if (!in.o16(&markCoverageOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 ligatureCoverageOffset;
    if (!in.o16(&ligatureCoverageOffset)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<MarkToLigatureAttachmentPositioning>();
    if (!in.u16(&ret->markClassCount)) {
      return EGLYF_ERROR;
    }
    Offset16 markArrayOffset;
    if (!in.o16(&markArrayOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 ligatureArrayOffset;
    if (!in.o16(&ligatureArrayOffset)) {
      return EGLYF_ERROR;
    }

    if (!in.seek(markCoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->markCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(ligatureCoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->ligatureCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(markArrayOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = MarkArray::Read(in, ret->markArray); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (!in.seek(ligatureArrayOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = LigatureArray::Read(in, ret->ligatureArray, ret->markClassCount); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto markCoverageOffset = writer->o16();
    if (!markCoverageOffset) {
      return EGLYF_ERROR;
    }
    auto ligatureCoverageOffset = writer->o16();
    if (!ligatureCoverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.u16(markClassCount)) {
      return EGLYF_ERROR;
    }
    auto markArrayOffset = writer->o16();
    if (!markArrayOffset) {
      return EGLYF_ERROR;
    }
    auto ligatureArrayOffset = writer->o16();
    if (!ligatureArrayOffset) {
      return EGLYF_ERROR;
    }

    if (auto st = markCoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = markCoverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = ligatureCoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = ligatureCoverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = markArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = markArray.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = ligatureArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = ligatureArray.write(out, markClassCount); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t) + 4 * sizeof(Offset16);
    ret += markCoverage->size();
    ret += ligatureCoverage->size();
    ret += markArray.size();
    ret += ligatureArray.size();
    return ret;
  }

public:
  std::shared_ptr<Coverage> markCoverage;
  std::shared_ptr<Coverage> ligatureCoverage;
  uint16_t markClassCount;
  MarkArray markArray;
  LigatureArray ligatureArray;
};

} // namespace eglyf::gpos
