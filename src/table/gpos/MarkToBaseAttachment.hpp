#pragma once

namespace eglyf::gpos {

class MarkToBaseAttachment : public Subtable {
public:
  struct BaseRecord {
    std::vector<std::shared_ptr<Anchor>> baseAnchors;
  };

  struct BaseArray {
    std::vector<BaseRecord> baseRecords;

    static Status Read(InputStream &in, BaseArray &out, uint16_t markClassCount) {
      using namespace std;
      uint16_t baseCount;
      jassert(in.position() == 0);
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
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    jassert(in.position() == 0);
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
      OffsetInputStream sub(&in);
      if (auto st = MarkArray::Read(sub, ret->markArray); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    {
      if (!in.seek(baseArrayOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto st = BaseArray::Read(sub, ret->baseArray, markClassCount); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    return EGLYF_ERROR;
  }

  size_t size() const override {
    return 0;
  }

public:
  std::shared_ptr<Coverage> markCoverage;
  std::shared_ptr<Coverage> baseCoverage;
  MarkArray markArray;
  BaseArray baseArray;
};

} // namespace eglyf::gpos
