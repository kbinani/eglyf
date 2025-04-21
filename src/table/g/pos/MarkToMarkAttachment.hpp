#pragma once

namespace eglyf::gpos {

class MarkToMarkAttachment : public Subtable {
public:
  struct Mark2 {
    std::vector<std::shared_ptr<Anchor>> mark2Anchors;
  };

  struct Mark2Array {
    uint16_t markClassCount;
    std::vector<Mark2> mark2Records;

    static Status Read(InputStream &stream, Mark2Array &out, uint16_t markClassCount) {
      using namespace std;
      OffsetInputStream in(&stream);
      out.markClassCount = markClassCount;
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
          if (offset == 0) {
            mark2.mark2Anchors.push_back(nullptr);
            continue;
          }
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

    Status write(OutputStream &stream) const {
      using namespace std;
      auto writer = make_shared<DataFragmentWriter>(&stream);
      if (!writer->sizeU16(mark2Records.size())) {
        return EGLYF_ERROR;
      }
      vector<vector<DataFragmentWriter::Marker16>> mark2AnchorOffsetsList;
      for (Mark2 const &mark : mark2Records) {
        if (mark.mark2Anchors.size() != markClassCount) {
          return EGLYF_ERROR;
        }
        vector<DataFragmentWriter::Marker16> mark2AnchorOffsets;
        for (size_t i = 0; i < mark.mark2Anchors.size(); i++) {
          auto offset = writer->o16();
          if (!offset) {
            return EGLYF_ERROR;
          }
          mark2AnchorOffsets.push_back(offset);
        }
        mark2AnchorOffsetsList.push_back(mark2AnchorOffsets);
      }
      for (size_t i = 0; i < mark2Records.size(); i++) {
        Mark2 const &mark = mark2Records[i];
        auto const &offsets = mark2AnchorOffsetsList[i];
        for (size_t j = 0; j < mark.mark2Anchors.size(); j++) {
          auto const &anchor = mark.mark2Anchors[j];
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

    auto ret = make_unique<MarkToMarkAttachment>();

    if (!in.seek(mark1CoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = Coverage::Read(in, ret->mark1Coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(mark2CoverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = Coverage::Read(in, ret->mark2Coverage); !st.ok()) {
      return EGLYF_ERROR;
    }

    {
      if (!in.seek(mark1ArrayOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = MarkArray::Read(in, ret->mark1Array); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    {
      if (!in.seek(mark2ArrayOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = Mark2Array::Read(in, ret->mark2Array, markClassCount); !st.ok()) {
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
    auto mark1CoverageOffset = writer->o16();
    if (!mark1CoverageOffset) {
      return EGLYF_ERROR;
    }
    auto mark2CoverageOffset = writer->o16();
    if (!mark2CoverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.u16(mark2Array.markClassCount)) {
      return EGLYF_ERROR;
    }
    auto mark1ArrayOffset = writer->o16();
    if (!mark1ArrayOffset) {
      return EGLYF_ERROR;
    }
    auto mark2ArrayOffset = writer->o16();
    if (!mark2ArrayOffset) {
      return EGLYF_ERROR;
    }

    if (auto st = mark1CoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = mark1Coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = mark2CoverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = mark2Coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = mark1ArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = mark1Array.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = mark2ArrayOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = mark2Array.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  std::shared_ptr<Coverage> mark1Coverage;
  std::shared_ptr<Coverage> mark2Coverage;
  MarkArray mark1Array;
  Mark2Array mark2Array;
};

} // namespace eglyf::gpos
