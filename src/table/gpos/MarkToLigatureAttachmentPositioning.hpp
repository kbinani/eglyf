#pragma once

namespace eglyf::gpos {

class MarkToLigatureAttachmentPositioning : public Subtable {
public:
  struct ComponentRecord {
    std::vector<std::shared_ptr<Anchor>> ligatureAnchors;
  };

  struct LigatureAttach {
    std::vector<ComponentRecord> componentRecords;

    static Status Read(InputStream &in, LigatureAttach &out, uint16_t markClassCount) {
      using namespace std;
      jassert(in.position() == 0);
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
  };

  struct LigatureArray {
    std::vector<LigatureAttach> ligatureAttaches;

    static Status Read(InputStream &in, LigatureArray &out, uint16_t markClassCount) {
      using namespace std;
      jassert(in.position() == 0);
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
        OffsetInputStream sub(&in);
        LigatureAttach ligatureAttach;
        if (auto st = LigatureAttach::Read(sub, ligatureAttach, markClassCount); !st.ok()) {
          return EGLYF_ERROR;
        }
        out.ligatureAttaches.push_back(ligatureAttach);
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
    Offset16 ligatureCoverageOffset;
    if (!in.o16(&ligatureCoverageOffset)) {
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
    Offset16 ligatureArrayOffset;
    if (!in.o16(&ligatureArrayOffset)) {
      return EGLYF_ERROR;
    }

    auto ret = make_unique<MarkToLigatureAttachmentPositioning>();

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

    {
      if (!in.seek(markArrayOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto st = MarkArray::Read(in, ret->markArray); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    {
      if (!in.seek(ligatureArrayOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto st = LigatureArray::Read(in, ret->ligatureArray, markClassCount); !st.ok()) {
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
  std::shared_ptr<Coverage> ligatureCoverage;
  MarkArray markArray;
  LigatureArray ligatureArray;
};

} // namespace eglyf::gpos
