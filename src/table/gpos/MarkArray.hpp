#pragma once

namespace eglyf::gpos {

class MarkRecord {
public:
  uint16_t markClass;
  std::shared_ptr<Anchor> markAnchor;
};

class MarkArray {
public:
  static Status Read(InputStream &in, MarkArray &out) {
    using namespace std;
    jassert(in.position() == 0);
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

public:
  std::vector<MarkRecord> markRecords;
};

} // namespace eglyf::gpos
