#pragma once

namespace eglyf::gpos {

class MarkToMarkAttachmentPositioning : public Subtable {
public:
  struct Mark2 {
    std::vector<std::shared_ptr<Anchor>> mark2Anchors;
  };

  struct Mark2Array {
    std::vector<Mark2> mark2Records;

    static Status Read(InputStream &in, Mark2Array &out, uint16_t markClassCount) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t mark2Count;
      if (!in.u16(&mark2Count)) {
        return EGLYF_ERROR;
      }
      vector<vector<Offset16>> mark2AnchorsOffsets;
      for (uint16_t i = 0; i < mark2Count; i++) {
        vector<Offset16> offsets;
        if (!in.o16a(offsets, markClassCount)) {
          return EGLYF_ERROR;
        }
        mark2AnchorsOffsets.push_back(offsets);
      }
      for (auto const &offsets : mark2AnchorsOffsets) {
        Mark2 mark2;
        for (auto offset : offsets) {
          if (!in.seek(offset)) {
            return EGLYF_ERROR;
          }
          shared_ptr<Anchor> anchor;
          if (auto st = AnchorReader::Read(in, anchor); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
          mark2.mark2Anchors.push_back(anchor);
        }
        out.mark2Records.push_back(mark2);
      }
      return Status::Ok();
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    Offset16 mark1CoverageOffset;
    if (!in.o16(&mark1CoverageOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 mark2CoverageOffset;
    if (!in.o16(&mark2CoverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t markClassCount;
    if (!in.u16(&markClassCount)) {
      return EGLYF_ERROR;
    }
    Offset16 mark1ArrayOffset;
    if (!in.o16(&mark1ArrayOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 mark2ArrayOffset;
    if (!in.o16(&mark2ArrayOffset)) {
      return EGLYF_ERROR;
    }

    auto ret = make_unique<MarkToMarkAttachmentPositioning>();

    if (!in.seek(mark1CoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->mark1Coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(mark2CoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->mark2Coverage); !st.ok()) {
      return EGLYF_ERROR;
    }

    {
      if (!in.seek(mark1ArrayOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto st = MarkArray::Read(sub, ret->mark1Array); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    {
      if (!in.seek(mark2ArrayOffset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto st = Mark2Array::Read(sub, ret->mark2Array, markClassCount); !st.ok()) {
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
  std::shared_ptr<Coverage> mark1Coverage;
  std::shared_ptr<Coverage> mark2Coverage;
  MarkArray mark1Array;
  Mark2Array mark2Array;
};

} // namespace eglyf::gpos
