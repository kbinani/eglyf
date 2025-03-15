#pragma once

namespace eglyf::gpos {

class SingleAdjustment1 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t valueFormat;
    if (!in.u16(&valueFormat)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<SingleAdjustment1>();
    if (auto valueRecord = ValueRecord::Read(in, valueFormat); valueRecord) {
      ret->valueRecord = *valueRecord;
    } else {
      return EGLYF_STATUS_PUSH(valueRecord.status());
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

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    return EGLYF_ERROR;
  }

  size_t size() const override {
    return 0;
  }

public:
  ValueRecord valueRecord;
};

class SingleAdjustment2 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t valueFormat;
    if (!in.u16(&valueFormat)) {
      return EGLYF_ERROR;
    }
    uint16_t valueCount;
    if (!in.u16(&valueCount)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<SingleAdjustment2>();
    for (uint16_t i = 0; i < valueCount; i++) {
      if (auto record = ValueRecord::Read(in, valueFormat); record) {
        ret->valueRecords.push_back(*record);
      } else {
        return EGLYF_STATUS_PUSH(record.status());
      }
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

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    return EGLYF_ERROR;
  }

  size_t size() const override {
    return 0;
  }

public:
  std::vector<ValueRecord> valueRecords;
};

class SingleAdjustment {
  SingleAdjustment() = delete;

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(SingleAdjustment1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(SingleAdjustment2::Read(in, out));
    }
    return EGLYF_ERROR;
  }
};

} // namespace eglyf::gpos
