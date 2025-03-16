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
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    auto valueFormat = valueRecord.format();
    if (valueFormat) {
      if (!out.u16(*valueFormat)) {
        return EGLYF_ERROR;
      }
    } else {
      return EGLYF_STATUS_PUSH(valueFormat.status());
    }
    if (auto st = valueRecord.write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 2 * sizeof(uint16_t) + sizeof(Offset16) + valueRecord.size();
    ret += coverage->size();
    return ret;
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
    auto ret = make_unique<SingleAdjustment2>();
    if (!in.u16(&ret->valueFormat)) {
      return EGLYF_ERROR;
    }
    uint16_t valueCount;
    if (!in.u16(&valueCount)) {
      return EGLYF_ERROR;
    }
    for (uint16_t i = 0; i < valueCount; i++) {
      if (auto record = ValueRecord::Read(in, ret->valueFormat); record) {
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
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.u16(valueFormat)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(valueRecords.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &record : valueRecords) {
      auto fmt = record.format();
      if (!fmt) {
        return EGLYF_STATUS_PUSH(fmt.status());
      }
      if (valueFormat != *fmt) {
        return EGLYF_ERROR;
      }
      if (auto st = record.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 3 * sizeof(uint16_t) + sizeof(Offset16);
    for (auto const &record : valueRecords) {
      ret += record.size();
    }
    ret += coverage->size();
    return ret;
  }

public:
  uint16_t valueFormat;
  std::vector<ValueRecord> valueRecords;
};

class SingleAdjustment {
  SingleAdjustment() = delete;

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    OffsetInputStream in(&stream);
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
