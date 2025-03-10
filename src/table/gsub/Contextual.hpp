#pragma once

namespace eglyf::gsub {

class Contextual1 : public Subtable {
public:
  struct SequenceRule {
    std::vector<uint16_t> inputSequence;
    std::vector<SequenceLookup> seqLookupRecords;

    static Optional<SequenceRule> Read(InputStream &in) {
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (glyphCount < 1) {
        return EGLYF_NULLOPT;
      }
      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return EGLYF_NULLOPT;
      }
      SequenceRule ret;
      if (!in.u16a(ret.inputSequence, glyphCount - 1)) {
        return EGLYF_NULLOPT;
      }
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto seq = SequenceLookup::Read(in); seq) {
          ret.seqLookupRecords.push_back(*seq);
        } else {
          return EGLYF_NULLOPT_PUSH(seq.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(inputSequence.size() + 1)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(seqLookupRecords.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(inputSequence)) {
        return EGLYF_ERROR;
      }
      for (auto const &seq : seqLookupRecords) {
        if (auto st = seq.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return Status::Ok();
    }

    size_t size() const {
      return sizeof(uint16_t) * 2 + inputSequence.size() * sizeof(uint16_t) + (2 * sizeof(uint16_t)) * seqLookupRecords.size();
    }
  };

  struct SequenceRuleSet {
    std::vector<SequenceRule> rules;

    static Optional<SequenceRuleSet> Read(InputStream &in) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t seqRuleCount;
      if (!in.u16(&seqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> seqRuleOffsets;
      if (!in.o16a(seqRuleOffsets, seqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      SequenceRuleSet ret;
      for (auto offset : seqRuleOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto rule = SequenceRule::Read(in); rule) {
          ret.rules.push_back(*rule);
        } else {
          return EGLYF_NULLOPT_PUSH(rule.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(rules.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> seqRuleOffsets;
      for (size_t i = 0; i < rules.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        seqRuleOffsets.push_back(offset);
      }
      for (size_t i = 0; i < rules.size(); i++) {
        auto const &rule = rules[i];
        auto offset = seqRuleOffsets[i];
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = rule.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }

    size_t size() const {
      size_t ret = sizeof(uint16_t) + rules.size() * sizeof(Offset16);
      for (auto const &rule : rules) {
        ret += rule.size();
      }
      return ret;
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t seqRuleSetCount;
    if (!in.u16(&seqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> seqRuleSetOffsets;
    if (!in.o16a(seqRuleSetOffsets, seqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<Contextual1>();
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (auto offset : seqRuleSetOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto ruleSet = SequenceRuleSet::Read(sub); ruleSet) {
        ret->ruleSets.push_back(*ruleSet);
      } else {
        return EGLYF_STATUS_PUSH(ruleSet.status());
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
    auto coverageOffset = writer->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> seqRuleSetOffsets;
    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      seqRuleSetOffsets.push_back(offset);
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto const &ruleSet = ruleSets[i];
      auto offset = seqRuleSetOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = ruleSet.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t) + sizeof(Offset16) + sizeof(uint16_t) + ruleSets.size() * sizeof(Offset16);
    ret += coverage->size();
    for (auto const &ruleSet : ruleSets) {
      ret += ruleSet.size();
    }
    return ret;
  }

public:
  std::vector<SequenceRuleSet> ruleSets;
};

class Contextual2 : public Subtable {
public:
  struct ClassSeqRule {
    std::vector<uint16_t> inputSequence;
    std::vector<SequenceLookup> seqLookupRecords;

    static Optional<ClassSeqRule> Read(InputStream &in) {
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (glyphCount < 1) {
        return EGLYF_NULLOPT;
      }
      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return EGLYF_NULLOPT;
      }
      ClassSeqRule ret;
      if (!in.o16a(ret.inputSequence, glyphCount - 1)) {
        return EGLYF_NULLOPT;
      }
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto lookup = SequenceLookup::Read(in); lookup) {
          ret.seqLookupRecords.push_back(*lookup);
        } else {
          return EGLYF_NULLOPT_PUSH(lookup.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(inputSequence.size() + 1)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(seqLookupRecords.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(inputSequence)) {
        return EGLYF_ERROR;
      }
      for (auto const &lookup : seqLookupRecords) {
        if (auto st = lookup.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return Status::Ok();
    }

    size_t size() const {
      return 2 * sizeof(uint16_t) + inputSequence.size() * sizeof(uint16_t) + (2 * sizeof(uint16_t)) * seqLookupRecords.size();
    }
  };

  struct ClassSeqRuleSet {
    std::vector<ClassSeqRule> rules;

    static Optional<ClassSeqRuleSet> Read(InputStream &in) {
      using namespace std;
      uint16_t classSeqRuleCount;
      if (!in.u16(&classSeqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> classSeqRuleOffsets;
      if (!in.o16a(classSeqRuleOffsets, classSeqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      ClassSeqRuleSet ret;
      for (auto offset : classSeqRuleOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto rule = ClassSeqRule::Read(in); rule) {
          ret.rules.push_back(*rule);
        } else {
          return EGLYF_NULLOPT_PUSH(rule.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(rules.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> classSeqRuleOffsets;
      for (size_t i = 0; i < rules.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        classSeqRuleOffsets.push_back(offset);
      }
      for (size_t i = 0; i < rules.size(); i++) {
        auto const &rule = rules[i];
        auto offset = classSeqRuleOffsets[i];
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = rule.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }

    size_t size() const {
      size_t ret = sizeof(uint16_t) + rules.size() * sizeof(Offset16);
      for (auto const &rule : rules) {
        ret += rule.size();
      }
      return ret;
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 classDefOffset;
    if (!in.o16(&classDefOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t classSeqRuleSetCount;
    if (!in.u16(&classSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> classSeqRuleSetOffsets;
    if (!in.o16a(classSeqRuleSetOffsets, classSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<Contextual2>();
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, ret->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(classDefOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = ClassDefReader::Read(in, ret->classDef); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (auto offset : classSeqRuleSetOffsets) {
      if (offset == 0) {
        ret->ruleSets.push_back(nullopt);
        continue;
      }
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto ruleSet = ClassSeqRuleSet::Read(sub); ruleSet) {
        ret->ruleSets.push_back(*ruleSet);
      } else {
        return EGLYF_STATUS_PUSH(ruleSet.status());
      }
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
    auto classDefOffset = writer->o16();
    if (!classDefOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> classSeqRuleSetOffsets;
    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      classSeqRuleSetOffsets.push_back(offset);
    }

    if (!coverage) {
      return EGLYF_ERROR;
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = classDefOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = classDef->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto const &ruleSet = ruleSets[i];
      auto offset = classSeqRuleSetOffsets[i];
      if (ruleSet) {
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = ruleSet->write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      } else {
        if (auto st = offset->null(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }

    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t) + 2 * sizeof(Offset16) + sizeof(uint16_t) + ruleSets.size() * sizeof(Offset16);
    ret += coverage->size();
    ret += classDef->size();
    for (auto const &ruleSet : ruleSets) {
      if (ruleSet) {
        ret += ruleSet->size();
      }
    }
    return ret;
  }

public:
  std::shared_ptr<ClassDef> classDef;
  std::vector<std::optional<ClassSeqRuleSet>> ruleSets;
};

class Contextual3 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    uint16_t glyphCount;
    if (!in.u16(&glyphCount)) {
      return EGLYF_ERROR;
    }
    uint16_t seqLookupCount;
    if (!in.u16(&seqLookupCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> coverageOffsets;
    if (!in.o16a(coverageOffsets, glyphCount)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<Contextual3>();
    for (uint16_t i = 0; i < seqLookupCount; i++) {
      if (auto lookup = SequenceLookup::Read(in); lookup) {
        ret->seqLookupRecords.push_back(*lookup);
      } else {
        return EGLYF_STATUS_PUSH(lookup.status());
      }
    }
    for (auto offset : coverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      } else {
        ret->coverages.push_back(cov);
      }
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(3)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(coverages.size())) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(seqLookupRecords.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> coverageOffsets;
    for (size_t i = 0; i < coverages.size(); i++) {
      auto offset = writer->o16();
      if (!offset) {
        return EGLYF_ERROR;
      }
      coverageOffsets.push_back(offset);
    }
    for (auto const &lookup : seqLookupRecords) {
      if (auto st = lookup.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    for (size_t i = 0; i < coverages.size(); i++) {
      auto const &cov = coverages[i];
      auto offset = coverageOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  size_t size() const override {
    size_t ret = 3 * sizeof(uint16_t) + coverages.size() * sizeof(Offset16);
    for (auto const &cov : coverages) {
      ret += cov->size();
    }
    ret += (2 * sizeof(uint16_t)) * seqLookupRecords.size();
    return ret;
  }

public:
  std::vector<std::shared_ptr<Coverage>> coverages;
  std::vector<SequenceLookup> seqLookupRecords;
};

class ContextualReader {
  ContextualReader() = delete;

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(Contextual1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(Contextual2::Read(in, out));
    } else if (format == 3) {
      return EGLYF_STATUS_PUSH(Contextual3::Read(in, out));
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf::gsub
