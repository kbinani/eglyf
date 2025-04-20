#pragma once

namespace eglyf::gpos {

class PairAdjustment1 : public Subtable {
public:
  struct PairValue {
    uint16_t secondGlyph;
    ValueRecord valueRecord1;
    ValueRecord valueRecord2;

    static Optional<PairValue> Read(InputStream &in, uint16_t valueFormat1, uint16_t valueFormat2) {
      PairValue ret;
      if (!in.u16(&ret.secondGlyph)) {
        return EGLYF_NULLOPT;
      }
      auto v1 = ValueRecord::Read(in, valueFormat1);
      if (!v1) {
        return EGLYF_NULLOPT_PUSH(v1.status());
      }
      ret.valueRecord1 = *v1;
      auto v2 = ValueRecord::Read(in, valueFormat2);
      if (!v2) {
        return EGLYF_NULLOPT_PUSH(v2.status());
      }
      ret.valueRecord2 = *v2;
      return ret;
    }

    Status write(OutputStream &out, uint16_t valueFormat1, uint16_t valueFormat2) const {
      if (!out.u16(secondGlyph)) {
        return EGLYF_ERROR;
      }
      auto f1 = valueRecord1.format();
      if (!f1) {
        return EGLYF_STATUS_PUSH(f1.status());
      }
      if (*f1 != valueFormat1) {
        return EGLYF_ERROR;
      }
      auto f2 = valueRecord2.format();
      if (!f2) {
        return EGLYF_STATUS_PUSH(f2.status());
      }
      if (*f2 != valueFormat2) {
        return EGLYF_ERROR;
      }
      if (auto st = valueRecord1.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = valueRecord2.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      return Status::Ok();
    }
  };

  struct PairSet {
    std::vector<PairValue> pairValueRecords;

    static Optional<PairSet> Read(InputStream &in, uint16_t valueFormat1, uint16_t valueFormat2) {
      uint16_t pairValueCount;
      if (!in.u16(&pairValueCount)) {
        return EGLYF_NULLOPT;
      }
      PairSet ret;
      for (uint16_t i = 0; i < pairValueCount; i++) {
        auto value = PairValue::Read(in, valueFormat1, valueFormat2);
        if (!value) {
          return EGLYF_NULLOPT_PUSH(value.status());
        }
        ret.pairValueRecords.push_back(*value);
      }
      return ret;
    }

    Status write(OutputStream &out, uint16_t valueFormat1, uint16_t valueFormat2) const {
      if (!out.sizeU16(pairValueRecords.size())) {
        return EGLYF_ERROR;
      }
      for (auto const &pairValue : pairValueRecords) {
        if (auto st = pairValue.write(out, valueFormat1, valueFormat2); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return Status::Ok();
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<PairAdjustment1>();
    if (!in.u16(&ret->valueFormat1)) {
      return EGLYF_ERROR;
    }
    uint16_t valueFormat2;
    if (!in.u16(&ret->valueFormat2)) {
      return EGLYF_ERROR;
    }
    uint16_t pairSetCount;
    if (!in.u16(&pairSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> pairSetOffsets;
    if (!in.o16a(pairSetOffsets, pairSetCount)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (auto offset : pairSetOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      auto pairSet = PairSet::Read(in, ret->valueFormat1, ret->valueFormat2);
      if (!pairSet) {
        return EGLYF_STATUS_PUSH(pairSet.status());
      }
      ret->pairSets.push_back(*pairSet);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &stream, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<DataFragmentWriter>(&stream);
    if (!writer->u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!writer->u16(valueFormat1)) {
      return EGLYF_ERROR;
    }
    if (!writer->u16(valueFormat2)) {
      return EGLYF_ERROR;
    }
    if (!writer->sizeU16(pairSets.size())) {
      return EGLYF_ERROR;
    }
    vector<DataFragmentWriter::Marker16> pairSetOffsets;
    for (auto const &pairSet : pairSets) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      pairSetOffsets.push_back(offset);
    }

    if (auto st = writer->writeDataFragment(coverageOffset, *coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (size_t i = 0; i < pairSets.size(); i++) {
      auto const &pairSet = pairSets[i];
      auto offset = pairSetOffsets[i];
      auto st = writer->writeDataFragment(offset, [&](OutputStream &o) {
        return EGLYF_STATUS_PUSH(pairSet.write(o, valueFormat1, valueFormat2));
      });
      if (!st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  uint16_t valueFormat1;
  uint16_t valueFormat2;
  std::vector<PairSet> pairSets;
};

class PairAdjustment2 : public Subtable {
public:
  struct Class2 {
    ValueRecord valueRecord1;
    ValueRecord valueRecord2;

    static Optional<Class2> Read(InputStream &in, uint16_t valueFormat1, uint16_t valueFormat2) {
      auto v1 = ValueRecord::Read(in, valueFormat1);
      if (!v1) {
        return EGLYF_NULLOPT_PUSH(v1.status());
      }
      auto v2 = ValueRecord::Read(in, valueFormat2);
      if (!v2) {
        return EGLYF_NULLOPT_PUSH(v2.status());
      }
      Class2 ret;
      ret.valueRecord1 = *v1;
      ret.valueRecord2 = *v2;
      return ret;
    }

    Status write(OutputStream &out, uint16_t valueFormat1, uint16_t valueFormat2) const {
      auto f1 = valueRecord1.format();
      if (!f1) {
        return EGLYF_STATUS_PUSH(f1.status());
      }
      if (*f1 != valueFormat1) {
        return EGLYF_ERROR;
      }
      auto f2 = valueRecord2.format();
      if (!f2) {
        return EGLYF_ERROR;
      }
      if (*f2 != valueFormat2) {
        return EGLYF_ERROR;
      }
      if (auto st = valueRecord1.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = valueRecord2.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      return Status::Ok();
    }
  };

  struct Class1 {
    std::vector<Class2> class2Records;

    static Optional<Class1> Read(InputStream &in, uint16_t class2Count, uint16_t valueFormat1, uint16_t valueFormat2) {
      Class1 ret;
      for (uint16_t i = 0; i < class2Count; i++) {
        auto class2 = Class2::Read(in, valueFormat1, valueFormat2);
        if (!class2) {
          return EGLYF_NULLOPT_PUSH(class2.status());
        }
        ret.class2Records.push_back(*class2);
      }
      return ret;
    }

    Status write(OutputStream &out, uint16_t valueFormat1, uint16_t valueFormat2) const {
      for (auto const &class2 : class2Records) {
        if (auto st = class2.write(out, valueFormat1, valueFormat2); !st.ok()) {
          return EGLYF_ERROR;
        }
      }
      return Status::Ok();
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<PairAdjustment2>();
    if (!in.u16(&ret->valueFormat1)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&ret->valueFormat2)) {
      return EGLYF_ERROR;
    }
    Offset16 classDef1Offset;
    if (!in.o16(&classDef1Offset)) {
      return EGLYF_ERROR;
    }
    Offset16 classDef2Offset;
    if (!in.o16(&classDef2Offset)) {
      return EGLYF_ERROR;
    }
    uint16_t class1Count;
    if (!in.u16(&class1Count)) {
      return EGLYF_ERROR;
    }
    uint16_t class2Count;
    if (!in.u16(&class2Count)) {
      return EGLYF_ERROR;
    }
    for (uint16_t i = 0; i < class1Count; i++) {
      auto class1 = Class1::Read(in, class2Count, ret->valueFormat1, ret->valueFormat2);
      if (!class1) {
        return EGLYF_STATUS_PUSH(class1.status());
      }
      ret->class1Records.push_back(*class1);
    }

    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(classDef1Offset)) {
      return EGLYF_ERROR;
    }
    if (auto st = ClassDef::Read(in, ret->classDef1); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(classDef2Offset)) {
      return EGLYF_ERROR;
    }
    if (auto st = ClassDef::Read(in, ret->classDef2); !st.ok()) {
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
    if (!out.u16(valueFormat1)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(valueFormat2)) {
      return EGLYF_ERROR;
    }
    auto classDef1Offset = writer->o16();
    if (!classDef1Offset) {
      return EGLYF_ERROR;
    }
    auto classDef2Offset = writer->o16();
    if (!classDef2Offset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(class1Records.size())) {
      return EGLYF_ERROR;
    }
    if (class1Records.empty()) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(class1Records.front().class2Records.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &class1 : class1Records) {
      if (auto st = class1.write(out, valueFormat1, valueFormat2); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = classDef1Offset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = classDef1->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = classDef2Offset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = classDef2->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return EGLYF_STATUS_PUSH(writer->commit());
  }

public:
  uint16_t valueFormat1;
  uint16_t valueFormat2;
  std::shared_ptr<ClassDef> classDef1;
  std::shared_ptr<ClassDef> classDef2;
  std::vector<Class1> class1Records;
};

class PairAdjustment {
  PairAdjustment() = delete;

public:
  static Status Read(InputStream &stream, std::shared_ptr<Subtable> &out) {
    OffsetInputStream in(&stream);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(PairAdjustment1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(PairAdjustment2::Read(in, out));
    }
    return EGLYF_ERROR;
  }
};

} // namespace eglyf::gpos
