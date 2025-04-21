#pragma once

namespace eglyf::gpos {

class MarkToBaseAttachment : public Subtable {
public:
  struct BaseRecord {
    std::vector<std::shared_ptr<Anchor>> baseAnchors;
  };

  struct BaseArray {
    uint16_t markClassCount;
    std::vector<BaseRecord> baseRecords;

    static Status Read(InputStream &stream, BaseArray &out, uint16_t markClassCount) {
      using namespace std;
      out.markClassCount = markClassCount;
      out.baseRecords.clear();

      OffsetInputStream in(&stream);

      uint16_t baseCount;
      if (!in.u16(&baseCount)) {
        return EGLYF_ERROR;
      }
      vector<vector<Offset16>> baseAnchorOffsets;
      for (uint16_t i = 0; i < baseCount; i++) {
        vector<Offset16> offsets;
        if (!in.o16a(offsets, markClassCount)) {
          return EGLYF_ERROR;
        }
        baseAnchorOffsets.push_back(offsets);
      }
      for (auto const &offsets : baseAnchorOffsets) {
        BaseRecord record;
        for (auto offset : offsets) {
          if (offset == 0) {
            record.baseAnchors.push_back(nullptr);
            continue;
          }
          if (!in.seek(offset)) {
            return EGLYF_ERROR;
          }
          shared_ptr<Anchor> anchor;
          if (auto st = AnchorReader::Read(in, anchor); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          record.baseAnchors.push_back(anchor);
        }
        out.baseRecords.push_back(record);
      }
      return Status::Ok();
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(baseRecords.size())) {
        return EGLYF_ERROR;
      }
      vector<vector<OffsetWriter::Handle16>> baseAnchorOffsetsList;
      for (auto const &record : baseRecords) {
        if (record.baseAnchors.size() != markClassCount) {
          return EGLYF_ERROR;
        }
        vector<OffsetWriter::Handle16> offsets;
        for (size_t i = 0; i < record.baseAnchors.size(); i++) {
          auto offset = writer->o16();
          if (!offset) {
            return EGLYF_ERROR;
          }
          offsets.push_back(offset);
        }
        baseAnchorOffsetsList.push_back(offsets);
      }
      for (size_t i = 0; i < baseRecords.size(); i++) {
        auto const &record = baseRecords[i];
        auto const &offsets = baseAnchorOffsetsList[i];
        for (size_t j = 0; j < record.baseAnchors.size(); j++) {
          auto const &anchor = record.baseAnchors[j];
          auto offset = offsets[j];
          if (anchor) {
            if (auto st = offset->mark(); !st.ok()) {
              return EGLYF_STATUS_PUSH(st);
            }
            if (auto st = anchor->write(out); !st.ok()) {
              return EGLYF_STATUS_PUSH(st);
            }
          } else {
            if (auto st = offset->null(); !st.ok()) {
              return EGLYF_STATUS_PUSH(st);
            }
          }
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
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
    Offset16 baseCoverageOffset;
    if (!in.o16(&baseCoverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t markClassCount;
    if (!in.u16(&markClassCount)) {
      return EGLYF_ERROR;
    }
    Offset16 markArrayOffset;
    if (!in.o16(&markArrayOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 baseArrayOffset;
    if (!in.o16(&baseArrayOffset)) {
      return EGLYF_ERROR;
    }

    auto ret = make_unique<MarkToBaseAttachment>();

    if (!in.seek(markCoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = Coverage::Read(in, ret->markCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(baseCoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = Coverage::Read(in, ret->baseCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    {
      if (!in.seek(markArrayOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = MarkArray::Read(in, ret->markArray); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    {
      if (!in.seek(baseArrayOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = BaseArray::Read(in, ret->baseArray, markClassCount); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &stream, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto out = make_shared<DataFragmentWriter>(&stream);
    if (!out->u16(1)) {
      return EGLYF_ERROR;
    }
    auto markCoverageOffset = out->o16();
    if (!markCoverageOffset) {
      return EGLYF_ERROR;
    }
    auto baseCoverageOffset = out->o16();
    if (!baseCoverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out->u16(baseArray.markClassCount)) {
      return EGLYF_ERROR;
    }
    auto markArrayOffset = out->o16();
    if (!markArrayOffset) {
      return EGLYF_ERROR;
    }
    auto baseArrayOffset = out->o16();
    if (!baseArrayOffset) {
      return EGLYF_ERROR;
    }

    if (auto st = out->writeDataFragment(markCoverageOffset, *markCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = out->writeDataFragment(baseCoverageOffset, *baseCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = out->writeDataFragment(markArrayOffset, markArray); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = out->writeDataFragment(baseArrayOffset, baseArray); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return EGLYF_STATUS_PUSH(out->commit());
  }

  std::optional<Attachment> findAttachment(uint16_t receptorGlyphID, uint16_t ligandGlyphID) const {
    using namespace std;
    auto baseIndex = baseCoverage->index(receptorGlyphID);
    if (baseIndex == Coverage::Npos) {
      return nullopt;
    }
    auto markIndex = markCoverage->index(ligandGlyphID);
    if (markIndex == Coverage::Npos) {
      return nullopt;
    }
    auto const &markRecord = markArray.markRecords[markIndex];
    auto const markClass = markRecord.markClass;
    auto const &markAnchor = markRecord.markAnchor;

    auto const &baseRecord = baseArray.baseRecords[baseIndex];
    auto const &baseAnchor = baseRecord.baseAnchors[markClass];

    Attachment ret;
    ret.receptor = baseAnchor;
    ret.ligand = markAnchor;

    return ret;
  }

public:
  std::shared_ptr<Coverage> markCoverage;
  std::shared_ptr<Coverage> baseCoverage;
  MarkArray markArray;
  BaseArray baseArray;
};

} // namespace eglyf::gpos
