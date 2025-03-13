#pragma once

namespace eglyf::gsub {

class ChainedContexts1 : public Subtable {
public:
  struct ChainedSequenceRule {
    std::vector<uint16_t> backtrackSequence;
    std::vector<uint16_t> inputSequence;
    std::vector<uint16_t> lookaheadSequence;
    std::vector<SequenceLookup> seqLookupRecords;

    static Optional<ChainedSequenceRule> Read(InputStream &in) {
      using namespace std;
      ChainedSequenceRule r;

      uint16_t backtrackGlyphCount;
      if (!in.u16(&backtrackGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.backtrackSequence, backtrackGlyphCount)) {
        return EGLYF_NULLOPT;
      }

      uint16_t inputGlyphCount;
      if (!in.u16(&inputGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (inputGlyphCount < 1) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.inputSequence, inputGlyphCount - 1)) {
        return EGLYF_NULLOPT;
      }

      uint16_t lookaheadGlyphCount;
      if (!in.u16(&lookaheadGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.lookaheadSequence, lookaheadGlyphCount)) {
        return EGLYF_NULLOPT;
      }

      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return EGLYF_NULLOPT;
      }
      r.seqLookupRecords.reserve(seqLookupCount);
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto l = SequenceLookup::Read(in); l) {
          r.seqLookupRecords.push_back(*l);
        } else {
          return EGLYF_NULLOPT_PUSH(l.status());
        }
      }

      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(backtrackSequence.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(backtrackSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(inputSequence.size() + 1)) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(inputSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(lookaheadSequence.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(lookaheadSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(seqLookupRecords.size())) {
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
      size_t ret = sizeof(uint16_t) + backtrackSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + inputSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + lookaheadSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + seqLookupRecords.size() * (sizeof(uint16_t) * 2);
      return ret;
    }
  };

  struct ChainedSequenceRuleSet {
    static Status Read(InputStream &in, std::shared_ptr<ChainedSequenceRuleSet> &out) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t chainedSeqRuleCount;
      if (!in.u16(&chainedSeqRuleCount)) {
        return EGLYF_ERROR;
      }
      auto r = make_unique<ChainedSequenceRuleSet>();
      vector<Offset16> offsets;
      if (!in.o16a(offsets, chainedSeqRuleCount)) {
        return EGLYF_ERROR;
      }
      for (uint16_t offset : offsets) {
        if (!in.seek(offset)) {
          return EGLYF_ERROR;
        }
        if (auto rule = ChainedSequenceRule::Read(in); rule) {
          r->rules.push_back(*rule);
        } else {
          return EGLYF_STATUS_PUSH(rule.status());
        }
      }
      out.reset(r.release());
      return Status::Ok();
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(rules.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> chainedSeqRuleOffsets;
      for (size_t i = 0; i < rules.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        chainedSeqRuleOffsets.push_back(offset);
      }
      for (size_t i = 0; i < rules.size(); i++) {
        auto const &rule = rules[i];
        auto offset = chainedSeqRuleOffsets[i];
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

    std::vector<ChainedSequenceRule> rules;
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t chainedSeqRuleSetCount;
    if (!in.u16(&chainedSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> chainedSeqRuleSetOffsets;
    if (!in.o16a(chainedSeqRuleSetOffsets, chainedSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<ChainedContexts1>();
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    map<Offset16, shared_ptr<ChainedSequenceRuleSet>> ruleSets;
    for (auto offset : chainedSeqRuleSetOffsets) {
      if (offset == 0) {
        r->ruleSets.push_back(nullptr);
        continue;
      }
      auto found = ruleSets.find(offset);
      if (found != ruleSets.end()) {
        r->ruleSets.push_back(found->second);
        continue;
      }
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      shared_ptr<ChainedSequenceRuleSet> ruleSet;
      if (auto st = ChainedSequenceRuleSet::Read(sub, ruleSet); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      r->ruleSets.push_back(ruleSet);
      ruleSets[offset] = ruleSet;
    }

    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return EGLYF_ERROR;
    }
    auto chainedSeqRuleSetWriter = SharedListWriter<ChainedSequenceRuleSet>::WriteOffsets16(out, beginning, ruleSets);
    if (auto st = chainedSeqRuleSetWriter->writeList(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return EGLYF_STATUS_PUSH(beginning->commit());
  }

  size_t size() const override {
    using namespace std;
    size_t ret = sizeof(uint16_t) + sizeof(Offset16) + sizeof(uint16_t) + ruleSets.size() * sizeof(Offset16);
    ret += coverage->size();
    set<shared_ptr<ChainedSequenceRuleSet>> distinct;
    for (auto const &ruleSet : ruleSets) {
      if (ruleSet) {
        distinct.insert(ruleSet);
      }
    }
    for (auto const &ruleSet : distinct) {
      ret += ruleSet->size();
    }
    return ret;
  }

public:
  std::vector<std::shared_ptr<ChainedSequenceRuleSet>> ruleSets;
};

class ChainedContexts2 : public Subtable {
public:
  struct ChainedClassSequenceRule {
    static Optional<ChainedClassSequenceRule> Read(InputStream &in) {
      using namespace std;
      ChainedClassSequenceRule r;

      uint16_t backtrackGlyphCount;
      if (!in.u16(&backtrackGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.backtrackSequence, backtrackGlyphCount)) {
        return EGLYF_NULLOPT;
      }

      uint16_t inputGlyphCount;
      if (!in.u16(&inputGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (inputGlyphCount == 0) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.inputSequence, inputGlyphCount - 1)) {
        return EGLYF_NULLOPT;
      }

      uint16_t lookaheadGlyphCount;
      if (!in.u16(&lookaheadGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.lookaheadSequence, lookaheadGlyphCount)) {
        return EGLYF_NULLOPT;
      }

      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return EGLYF_NULLOPT;
      }
      r.seqLookupRecords.reserve(seqLookupCount);
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto req = SequenceLookup::Read(in); req) {
          r.seqLookupRecords.push_back(*req);
        } else {
          return EGLYF_NULLOPT_PUSH(req.status());
        }
      }
      return r;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      if (!out.sizeU16(backtrackSequence.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(backtrackSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(inputSequence.size() + 1)) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(inputSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(lookaheadSequence.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(lookaheadSequence)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(seqLookupRecords.size())) {
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
      size_t ret = sizeof(uint16_t) + backtrackSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + inputSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + lookaheadSequence.size() * sizeof(uint16_t);
      ret += sizeof(uint16_t) + (2 * sizeof(uint16_t)) * seqLookupRecords.size();
      return ret;
    }

  public:
    std::vector<uint16_t> backtrackSequence;
    std::vector<uint16_t> inputSequence;
    std::vector<uint16_t> lookaheadSequence;
    std::vector<SequenceLookup> seqLookupRecords;
  };

  struct ChainedClassSequenceRuleSet {
    static Optional<ChainedClassSequenceRuleSet> Read(InputStream &in) {
      using namespace std;
      jassert(in.position() == 0);
      ChainedClassSequenceRuleSet r;
      uint16_t chainedClassSeqRuleCount;
      if (!in.u16(&chainedClassSeqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> chainedClassSeqRuleOffsets;
      if (!in.o16a(chainedClassSeqRuleOffsets, chainedClassSeqRuleCount)) {
        return EGLYF_NULLOPT;
      }
      for (auto offset : chainedClassSeqRuleOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto v = ChainedClassSequenceRule::Read(in); v) {
          r.rules.push_back(*v);
        } else {
          return EGLYF_NULLOPT_PUSH(v.status());
        }
      }
      return r;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto beginning = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(rules.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> handles;
      for (auto const &rule : rules) {
        auto h = beginning->o16();
        if (!h) {
          return EGLYF_ERROR;
        }
        handles.push_back(h);
      }
      for (size_t i = 0; i < rules.size(); i++) {
        auto const &rule = rules[i];
        auto handle = handles[i];
        if (auto st = handle->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = rule.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(beginning->commit());
    }

    size_t size() const {
      size_t ret = sizeof(uint16_t) + rules.size() * sizeof(Offset16);
      for (auto const &rule : rules) {
        ret += rule.size();
      }
      return ret;
    }

    std::vector<ChainedClassSequenceRule> rules;
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 backtrackClassDefOffset;
    if (!in.o16(&backtrackClassDefOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 inputClassDefOffset;
    if (!in.o16(&inputClassDefOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 lookaheadClassDefOffset;
    if (!in.o16(&lookaheadClassDefOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t chainedClassSeqRuleSetCount;
    if (!in.u16(&chainedClassSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> chainedClassSeqRuleSetOffsets;
    if (!in.o16a(chainedClassSeqRuleSetOffsets, chainedClassSeqRuleSetCount)) {
      return EGLYF_ERROR;
    }

    auto r = make_unique<ChainedContexts2>();

    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (backtrackClassDefOffset > 0) {
      if (!in.seek(backtrackClassDefOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = ClassDefReader::Read(in, r->backtrackClassDef); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (!in.seek(inputClassDefOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = ClassDefReader::Read(in, r->inputClassDef); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (!in.seek(lookaheadClassDefOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = ClassDefReader::Read(in, r->lookaheadClassDef); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    r->ruleSets.reserve(chainedClassSeqRuleSetCount);
    for (Offset16 offset : chainedClassSeqRuleSetOffsets) {
      if (offset == 0) {
        r->ruleSets.push_back(nullopt);
        continue;
      }
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(&in);
      if (auto rule = ChainedClassSequenceRuleSet::Read(sub); rule) {
        r->ruleSets.push_back(*rule);
      } else {
        return EGLYF_STATUS_PUSH(rule.status());
      }
    }

    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(2)) {
      return EGLYF_ERROR;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return EGLYF_ERROR;
    }
    auto backtrackClassDefOffset = beginning->o16();
    if (!backtrackClassDefOffset) {
      return EGLYF_ERROR;
    }
    auto inputClassDefOffset = beginning->o16();
    if (!inputClassDefOffset) {
      return EGLYF_ERROR;
    }
    auto lookaheadClassDefOffset = beginning->o16();
    if (!lookaheadClassDefOffset) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> chainedClassSeqRuleSetOffsets;
    for (auto const &ruleSet : ruleSets) {
      auto h = beginning->o16();
      if (!h) {
        return EGLYF_ERROR;
      }
      chainedClassSeqRuleSetOffsets.push_back(h);
    }

    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (backtrackClassDef) {
      if (auto st = backtrackClassDefOffset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = backtrackClassDef->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else {
      if (auto st = backtrackClassDefOffset->null(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (auto st = inputClassDefOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = inputClassDef->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (auto st = lookaheadClassDefOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = lookaheadClassDef->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto const &ruleSet = ruleSets[i];
      auto offset = chainedClassSeqRuleSetOffsets[i];
      if (!ruleSet) {
        if (auto st = offset->null(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        continue;
      }
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = ruleSet->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return EGLYF_STATUS_PUSH(beginning->commit());
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t) + 4 * sizeof(Offset16) + sizeof(uint16_t) + ruleSets.size() * sizeof(Offset16);
    ret += coverage->size();
    if (backtrackClassDef) {
      ret += backtrackClassDef->size();
    }
    ret += inputClassDef->size();
    ret += lookaheadClassDef->size();
    for (auto const &ruleSet : ruleSets) {
      if (ruleSet) {
        ret += ruleSet->size();
      }
    }
    return ret;
  }

public:
  // NOTE: backtrackClassDef is not mentioned as nullable in the reference, but some fonts does not have backtrackClassDef
  //  https://learn.microsoft.com/en-us/typography/opentype/spec/chapter2#chained-sequence-context-format-2-class-based-glyph-contexts
  std::shared_ptr<ClassDef> backtrackClassDef;
  std::shared_ptr<ClassDef> inputClassDef;
  std::shared_ptr<ClassDef> lookaheadClassDef;
  std::vector<std::optional<ChainedClassSequenceRuleSet>> ruleSets;
};

class ChainedContexts3 : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    auto r = make_unique<ChainedContexts3>();

    uint16_t backtrackGlyphCount;
    if (!in.u16(&backtrackGlyphCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> backtrackCoverageOffsets;
    if (!in.o16a(backtrackCoverageOffsets, backtrackGlyphCount)) {
      return EGLYF_ERROR;
    }

    uint16_t inputGlyphCount;
    if (!in.u16(&inputGlyphCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> inputCoverageOffsets;
    if (!in.o16a(inputCoverageOffsets, inputGlyphCount)) {
      return EGLYF_ERROR;
    }

    uint16_t lookaheadGlyphCount;
    if (!in.u16(&lookaheadGlyphCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> lookaheadCoverageOffsets;
    if (!in.o16a(lookaheadCoverageOffsets, lookaheadGlyphCount)) {
      return EGLYF_ERROR;
    }

    uint16_t seqLookupCount;
    if (!in.u16(&seqLookupCount)) {
      return EGLYF_ERROR;
    }
    r->seqLookups.reserve(seqLookupCount);
    for (uint16_t i = 0; i < seqLookupCount; i++) {
      if (auto seq = SequenceLookup::Read(in); seq) {
        r->seqLookups.push_back(*seq);
      } else {
        return EGLYF_STATUS_PUSH(seq.status());
      }
    }

    for (Offset16 offset : backtrackCoverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); st.ok()) {
        r->backtrackCoverage.push_back(cov);
      } else {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    for (Offset16 offset : inputCoverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); st.ok()) {
        r->inputCoverage.push_back(cov);
      } else {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    for (Offset16 offset : lookaheadCoverageOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      shared_ptr<Coverage> cov;
      if (auto st = CoverageReader::Read(in, cov); st.ok()) {
        r->lookaheadCoverage.push_back(cov);
      } else {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    out.reset(r.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(3)) {
      return EGLYF_ERROR;
    }

    if (!out.sizeU16(backtrackCoverage.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> backtrackCoverageOffsets;
    for (auto const &it : backtrackCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return EGLYF_ERROR;
      }
      backtrackCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(inputCoverage.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> inputCoverageOffsets;
    for (auto const &it : inputCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return EGLYF_ERROR;
      }
      inputCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(lookaheadCoverage.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> lookaheadCoverageOffsets;
    for (auto const &it : lookaheadCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return EGLYF_ERROR;
      }
      lookaheadCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(seqLookups.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &seq : seqLookups) {
      if (auto st = seq.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (size_t i = 0; i < backtrackCoverage.size(); i++) {
      auto const &cov = backtrackCoverage[i];
      auto handle = backtrackCoverageOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (size_t i = 0; i < inputCoverage.size(); i++) {
      auto const &cov = inputCoverage[i];
      auto handle = inputCoverageOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    for (size_t i = 0; i < lookaheadCoverage.size(); i++) {
      auto const &cov = lookaheadCoverage[i];
      auto handle = lookaheadCoverageOffsets[i];
      if (auto st = handle->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = cov->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return EGLYF_STATUS_PUSH(beginning->commit());
  }

  size_t size() const override {
    size_t ret = sizeof(uint16_t);
    ret += sizeof(uint16_t) + backtrackCoverage.size() * sizeof(Offset16);
    for (auto const &cov : backtrackCoverage) {
      ret += cov->size();
    }
    ret += sizeof(uint16_t) + inputCoverage.size() * sizeof(Offset16);
    for (auto const &cov : inputCoverage) {
      ret += cov->size();
    }
    ret += sizeof(uint16_t) + lookaheadCoverage.size() * sizeof(Offset16);
    for (auto const &cov : lookaheadCoverage) {
      ret += cov->size();
    }
    ret += sizeof(uint16_t) + (2 * sizeof(uint16_t)) * seqLookups.size();
    return ret;
  }

public:
  std::vector<std::shared_ptr<Coverage>> backtrackCoverage;
  std::vector<std::shared_ptr<Coverage>> inputCoverage;
  std::vector<std::shared_ptr<Coverage>> lookaheadCoverage;
  std::vector<SequenceLookup> seqLookups;
};

class ChainedContexts {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format == 1) {
      return EGLYF_STATUS_PUSH(ChainedContexts1::Read(in, out));
    } else if (format == 2) {
      return EGLYF_STATUS_PUSH(ChainedContexts2::Read(in, out));
    } else if (format == 3) {
      return EGLYF_STATUS_PUSH(ChainedContexts3::Read(in, out));
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf::gsub
