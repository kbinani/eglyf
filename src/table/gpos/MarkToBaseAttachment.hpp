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

    size_t size() const {
      size_t ret = sizeof(uint16_t);
      for (auto const &record : baseRecords) {
        ret += record.baseAnchors.size() * sizeof(Offset16);
        for (auto const &anchor : record.baseAnchors) {
          if (anchor) {
            ret += anchor->size();
          }
        }
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
    if (auto st = CoverageReader::Read(in, ret->markCoverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(baseCoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->baseCoverage); !st.ok()) {
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
    auto baseCoverageOffset = writer->o16();
    if (!baseCoverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.u16(baseArray.markClassCount)) {
      return EGLYF_ERROR;
    }
    auto markArrayOffset = writer->o16();
    if (!markArrayOffset) {
      return EGLYF_ERROR;
    }
    auto baseArrayOffset = writer->o16();
    if (!baseArrayOffset) {
      return EGLYF_ERROR;
    }

    if (auto st = markCoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = markCoverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = baseCoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = baseCoverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = markArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = markArray.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = baseArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = baseArray.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t) + 4 * sizeof(Offset16);
    ret += markCoverage->size();
    ret += baseCoverage->size();
    ret += markArray.size();
    ret += baseArray.size();
    return ret;
  }

public:
  std::shared_ptr<Coverage> markCoverage;
  std::shared_ptr<Coverage> baseCoverage;
  MarkArray markArray;
  BaseArray baseArray;
};

} // namespace eglyf::gpos
