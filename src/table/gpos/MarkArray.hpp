#pragma once

namespace eglyf::gpos {

class MarkRecord {
public:
  uint16_t markClass;
  std::shared_ptr<Anchor> markAnchor;
};

class MarkArray {
public:
  static Status Read(InputStream &stream, MarkArray &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    uint16_t markCount;
    if (!in.u16(&markCount)) {
      return EGLYF_ERROR;
    }
    struct Record {
      uint16_t markClass;
      Offset16 markAnchorOffset;
    };
    vector<Record> markRecords;
    for (uint16_t i = 0; i < markCount; i++) {
      Record record;
      if (!in.u16(&record.markClass)) {
        return EGLYF_ERROR;
      }
      if (!in.o16(&record.markAnchorOffset)) {
        return EGLYF_ERROR;
      }
      markRecords.push_back(record);
    }
    for (auto const &record : markRecords) {
      if (!in.seek(record.markAnchorOffset)) {
        return EGLYF_ERROR;
      }
      MarkRecord r;
      r.markClass = record.markClass;
      if (auto st = AnchorReader::Read(in, r.markAnchor); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      out.markRecords.push_back(r);
    }
    return Status::Ok();
  }

  Status write(OutputStream &out) const {
    using namespace std;
    auto writer = make_shared<DataFragmentWriter>(&out);
    if (!writer->sizeU16(markRecords.size())) {
      return EGLYF_ERROR;
    }
    vector<DataFragmentWriter::Marker16> markAnchorOffsets;
    for (MarkRecord const &record : markRecords) {
      if (!writer->u16(record.markClass)) {
        return EGLYF_ERROR;
      }
      auto markAnchorOffset = writer->o16();
      if (!markAnchorOffset) {
        return EGLYF_ERROR;
      }
      markAnchorOffsets.push_back(markAnchorOffset);
    }
    for (size_t i = 0; i < markRecords.size(); i++) {
      auto const &record = markRecords[i];
      auto offset = markAnchorOffsets[i];
      if (auto st = writer->writeDataFragment({offset}, *record.markAnchor); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const {
    size_t ret = sizeof(uint16_t) + markRecords.size() * (sizeof(uint16_t) + sizeof(Offset16));
    for (auto const &record : markRecords) {
      ret += record.markAnchor->size();
    }
    return ret;
  }

public:
  std::vector<MarkRecord> markRecords;
};

} // namespace eglyf::gpos
