#pragma once

namespace eglyf::gsub {

class ChainedContextsSubstitution1 : public Subtable {
public:
  struct ChainedSequenceRule {
    std::vector<uint16_t> backtrackSequence;
    std::vector<uint16_t> inputSequence;
    std::vector<uint16_t> lookaheadSequence;
    std::vector<SequenceLookup> seqLookupRecords;

    static std::optional<ChainedSequenceRule> Read(InputStream &in) {
      using namespace std;
      ChainedSequenceRule r;

      uint16_t backtrackGlyphCount;
      if (!in.u16(&backtrackGlyphCount)) {
        return nullopt;
      }
      r.backtrackSequence.reserve(backtrackGlyphCount);
      for (uint16_t i = 0; i < backtrackGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.backtrackSequence.push_back(v);
      }

      uint16_t inputGlyphCount;
      if (!in.u16(&inputGlyphCount)) {
        return nullopt;
      }
      if (inputGlyphCount < 0) {
        return nullopt;
      }
      r.inputSequence.reserve(inputGlyphCount - 1);
      for (uint16_t i = 1; i < inputGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.inputSequence.push_back(v);
      }

      uint16_t lookaheadGlyphCount;
      if (!in.u16(&lookaheadGlyphCount)) {
        return nullopt;
      }
      r.lookaheadSequence.reserve(lookaheadGlyphCount);
      for (uint16_t i = 0; i < lookaheadGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.lookaheadSequence.push_back(v);
      }

      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return nullopt;
      }
      r.seqLookupRecords.reserve(seqLookupCount);
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto l = SequenceLookup::Read(in); l) {
          r.seqLookupRecords.push_back(*l);
        } else {
          return nullopt;
        }
      }

      return r;
    }
  };

  struct ChainedSequenceRuleSet {
    static std::optional<ChainedSequenceRuleSet> Read(InputStream &in) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t chainedSeqRuleCount;
      if (!in.u16(&chainedSeqRuleCount)) {
        return nullopt;
      }
      ChainedSequenceRuleSet r;
      r.rules.reserve(chainedSeqRuleCount);
      vector<Offset16> offsets;
      for (uint16_t i = 0; i < chainedSeqRuleCount; i++) {
        Offset16 v;
        if (!in.o16(&v)) {
          return nullopt;
        }
        offsets.push_back(v);
      }
      for (uint16_t offset : offsets) {
        if (!in.seek(offset)) {
          return nullopt;
        }
        if (auto rule = ChainedSequenceRule::Read(in); rule) {
          r.rules.push_back(*rule);
        } else {
          return nullopt;
        }
      }
      return r;
    }

    std::vector<ChainedSequenceRule> rules;
  };

public:
  static std::shared_ptr<ChainedContextsSubstitution1> Read(InputStream &in) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    uint16_t chainedSeqRuleSetCount;
    if (!in.u16(&chainedSeqRuleSetCount)) {
      return nullptr;
    }
    vector<Offset16> chainedSeqRuleSetOffsets;
    chainedSeqRuleSetOffsets.reserve(chainedSeqRuleSetCount);
    for (uint16_t i = 0; i < chainedSeqRuleSetCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      chainedSeqRuleSetOffsets.push_back(v);
    }
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    auto r = make_shared<ChainedContextsSubstitution1>();
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    for (auto offset : chainedSeqRuleSetOffsets) {
      if (offset == 0) {
        r->ruleSets.push_back(nullopt);
        continue;
      }
      if (!in.seek(offset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto ruleSet = ChainedSequenceRuleSet::Read(sub); ruleSet) {
        r->ruleSets.push_back(*ruleSet);
      } else {
        return nullptr;
      }
    }

    return r;
  }

  bool write(OutputStream &out) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> chainedSeqRuleSetOffsets;
    for (auto const &ruleSet : ruleSets) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      chainedSeqRuleSetOffsets.push_back(h);
    }

    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }

    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto chainedSequenceRuleSetBeginning = make_shared<OffsetWriter>(out);
      auto const &ruleSet = ruleSets[i];
      auto handle = chainedSeqRuleSetOffsets[i];
      if (!ruleSet) {
        if (!handle->null()) {
          return false;
        }
        continue;
      }
      if (!handle->mark()) {
        return false;
      }
      if (!out.sizeU16(ruleSet->rules.size())) {
        return false;
      }
      vector<OffsetWriter::Handle16> chainedSeqRuleOffsets;
      for (auto const &rule : ruleSet->rules) {
        auto h = chainedSequenceRuleSetBeginning->o16();
        if (!h) {
          return false;
        }
        chainedSeqRuleOffsets.push_back(h);
      }
      for (size_t j = 0; j < ruleSet->rules.size(); j++) {
        auto const &rule = ruleSet->rules[j];
        auto h = chainedSeqRuleOffsets[j];
        if (!h->mark()) {
          return false;
        }
        if (!out.sizeU16(rule.backtrackSequence.size())) {
          return false;
        }
        if (!out.u16a(rule.backtrackSequence)) {
          return false;
        }
        if (!out.sizeU16(rule.inputSequence.size() + 1)) {
          return false;
        }
        if (!out.u16a(rule.inputSequence)) {
          return false;
        }
        if (!out.sizeU16(rule.lookaheadSequence.size())) {
          return false;
        }
        if (!out.u16a(rule.lookaheadSequence)) {
          return false;
        }
        for (auto const &seq : rule.seqLookupRecords) {
          if (!seq.write(out)) {
            return false;
          }
        }
      }
      if (!chainedSequenceRuleSetBeginning->commit()) {
        return false;
      }
    }

    return beginning->commit();
  }

public:
  std::vector<std::optional<ChainedSequenceRuleSet>> ruleSets;
};

class ChainedContextsSubstitution2 : public Subtable {
public:
  struct ChainedClassSequenceRule {
    static std::optional<ChainedClassSequenceRule> Read(InputStream &in) {
      using namespace std;
      ChainedClassSequenceRule r;

      uint16_t backtrackGlyphCount;
      if (!in.u16(&backtrackGlyphCount)) {
        return nullopt;
      }
      r.backtrackSequence.reserve(backtrackGlyphCount);
      for (uint16_t i = 0; i < backtrackGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.backtrackSequence.push_back(v);
      }

      uint16_t inputGlyphCount;
      if (!in.u16(&inputGlyphCount)) {
        return nullopt;
      }
      if (inputGlyphCount == 0) {
        return nullopt;
      }
      r.inputSequence.reserve(inputGlyphCount - 1);
      for (uint16_t i = 1; i < inputGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.inputSequence.push_back(v);
      }

      uint16_t lookaheadGlyphCount;
      if (!in.u16(&lookaheadGlyphCount)) {
        return nullopt;
      }
      r.lookaheadSequence.reserve(lookaheadGlyphCount);
      for (uint16_t i = 0; i < lookaheadGlyphCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.lookaheadSequence.push_back(v);
      }

      uint16_t seqLookupCount;
      if (!in.u16(&seqLookupCount)) {
        return nullopt;
      }
      r.seqLookupRecords.reserve(seqLookupCount);
      for (uint16_t i = 0; i < seqLookupCount; i++) {
        if (auto req = SequenceLookup::Read(in); req) {
          r.seqLookupRecords.push_back(*req);
        } else {
          return nullopt;
        }
      }
      return r;
    }

    bool write(OutputStream &out) const {
      using namespace std;
      if (!out.sizeU16(backtrackSequence.size())) {
        return false;
      }
      if (!out.u16a(backtrackSequence)) {
        return false;
      }
      if (!out.sizeU16(inputSequence.size() + 1)) {
        return false;
      }
      if (!out.u16a(inputSequence)) {
        return false;
      }
      if (!out.sizeU16(lookaheadSequence.size())) {
        return false;
      }
      if (!out.u16a(lookaheadSequence)) {
        return false;
      }
      if (!out.sizeU16(seqLookupRecords.size())) {
        return false;
      }
      for (auto const &seq : seqLookupRecords) {
        if (!seq.write(out)) {
          return false;
        }
      }
      return true;
    }

  public:
    std::vector<uint16_t> backtrackSequence;
    std::vector<uint16_t> inputSequence;
    std::vector<uint16_t> lookaheadSequence;
    std::vector<SequenceLookup> seqLookupRecords;
  };

  struct ChainedClassSequenceRuleSet {
    static std::optional<ChainedClassSequenceRuleSet> Read(InputStream &in) {
      using namespace std;
      ChainedClassSequenceRuleSet r;
      uint16_t chainedClassSeqRuleCount;
      if (!in.u16(&chainedClassSeqRuleCount)) {
        return nullopt;
      }
      r.rules.reserve(chainedClassSeqRuleCount);
      for (uint16_t i = 0; i < chainedClassSeqRuleCount; i++) {
        if (auto v = ChainedClassSequenceRule::Read(in); v) {
          r.rules.push_back(*v);
        } else {
          return nullopt;
        }
      }
      return r;
    }

    bool write(OutputStream &out) const {
      using namespace std;
      auto beginning = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(rules.size())) {
        return false;
      }
      vector<OffsetWriter::Handle16> handles;
      for (auto const &rule : rules) {
        auto h = beginning->o16();
        if (!h) {
          return false;
        }
        handles.push_back(h);
      }
      for (size_t i = 0; i < rules.size(); i++) {
        auto const &rule = rules[i];
        auto handle = handles[i];
        if (!handle->mark()) {
          return false;
        }
        if (!rule.write(out)) {
          return false;
        }
      }
      return beginning->commit();
    }

    std::vector<ChainedClassSequenceRule> rules;
  };

public:
  static std::shared_ptr<ChainedContextsSubstitution2> Read(InputStream &in) {
    using namespace std;
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    Offset16 backtrackClassDefOffset;
    if (!in.o16(&backtrackClassDefOffset)) {
      return nullptr;
    }
    Offset16 inputClassDefOffset;
    if (!in.o16(&inputClassDefOffset)) {
      return nullptr;
    }
    Offset16 lookaheadClassDefOffset;
    if (!in.o16(&lookaheadClassDefOffset)) {
      return nullptr;
    }
    uint16_t chainedClassSeqRuleSetCount;
    if (!in.o16(&chainedClassSeqRuleSetCount)) {
      return nullptr;
    }
    vector<Offset16> chainedClassSeqRuleSetOffsets;
    chainedClassSeqRuleSetOffsets.reserve(chainedClassSeqRuleSetCount);
    for (uint16_t i = 0; i < chainedClassSeqRuleSetCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      chainedClassSeqRuleSetOffsets.push_back(v);
    }

    auto r = make_shared<ChainedContextsSubstitution2>();

    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }

    if (!in.seek(backtrackClassDefOffset)) {
      return nullptr;
    }
    if (auto def = ClassDefReader::Read(in); def) {
      r->backtrackClassDef = def;
    } else {
      return nullptr;
    }

    if (!in.seek(inputClassDefOffset)) {
      return nullptr;
    }
    if (auto def = ClassDefReader::Read(in); def) {
      r->inputClassDef = def;
    } else {
      return nullptr;
    }

    if (!in.seek(lookaheadClassDefOffset)) {
      return nullptr;
    }
    if (auto def = ClassDefReader::Read(in); def) {
      r->lookaheadClassDef = def;
    } else {
      return nullptr;
    }

    r->ruleSets.reserve(chainedClassSeqRuleSetCount);
    for (Offset16 offset : chainedClassSeqRuleSetOffsets) {
      if (offset == 0) {
        r->ruleSets.push_back(nullopt);
        continue;
      }
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto rule = ChainedClassSequenceRuleSet::Read(in); rule) {
        r->ruleSets.push_back(*rule);
      } else {
        return nullptr;
      }
    }

    return r;
  }

  bool write(OutputStream &out) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(2)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    auto backtrackClassDefOffset = beginning->o16();
    if (!backtrackClassDefOffset) {
      return false;
    }
    auto inputClassDefOffset = beginning->o16();
    if (!inputClassDefOffset) {
      return false;
    }
    auto lookaheadClassDefOffset = beginning->o16();
    if (!lookaheadClassDefOffset) {
      return false;
    }
    if (!out.sizeU16(ruleSets.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> chainedClassSeqRuleSetOffsets;
    for (auto const &ruleSet : ruleSets) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      chainedClassSeqRuleSetOffsets.push_back(h);
    }

    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }

    if (!backtrackClassDefOffset->mark()) {
      return false;
    }
    if (!backtrackClassDef->write(out)) {
      return false;
    }

    if (!inputClassDefOffset->mark()) {
      return false;
    }
    if (!inputClassDef->write(out)) {
      return false;
    }

    if (!lookaheadClassDefOffset->mark()) {
      return false;
    }
    if (!lookaheadClassDef->write(out)) {
      return false;
    }

    for (size_t i = 0; i < ruleSets.size(); i++) {
      auto const &ruleSet = ruleSets[i];
      auto offset = chainedClassSeqRuleSetOffsets[i];
      if (!ruleSet) {
        if (!offset->null()) {
          return false;
        }
        continue;
      }
      if (!offset->mark()) {
        return false;
      }
      if (!ruleSet->write(out)) {
        return false;
      }
    }

    return beginning->commit();
  }

public:
  std::shared_ptr<ClassDef> backtrackClassDef;
  std::shared_ptr<ClassDef> inputClassDef;
  std::shared_ptr<ClassDef> lookaheadClassDef;
  std::vector<std::optional<ChainedClassSequenceRuleSet>> ruleSets;
};

class ChainedContextsSubstitution3 : public Subtable {
public:
  static std::shared_ptr<ChainedContextsSubstitution3> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<ChainedContextsSubstitution3>();

    uint16_t backtrackGlyphCount;
    if (!in.u16(&backtrackGlyphCount)) {
      return nullptr;
    }
    vector<Offset16> backtrackCoverageOffsets;
    backtrackCoverageOffsets.reserve(backtrackGlyphCount);
    for (uint16_t i = 0; i < backtrackGlyphCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      backtrackCoverageOffsets.push_back(v);
    }

    uint16_t inputGlyphCount;
    if (!in.u16(&inputGlyphCount)) {
      return nullptr;
    }
    vector<Offset16> inputCoverageOffsets;
    inputCoverageOffsets.reserve(inputGlyphCount);
    for (uint16_t i = 0; i < inputGlyphCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      inputCoverageOffsets.push_back(v);
    }

    uint16_t lookaheadGlyphCount;
    if (!in.u16(&lookaheadGlyphCount)) {
      return nullptr;
    }
    vector<Offset16> lookaheadCoverageOffsets;
    lookaheadCoverageOffsets.reserve(lookaheadGlyphCount);
    for (uint16_t i = 0; i < lookaheadGlyphCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      lookaheadCoverageOffsets.push_back(v);
    }

    uint16_t seqLookupCount;
    if (!in.u16(&seqLookupCount)) {
      return nullptr;
    }
    r->seqLookups.reserve(seqLookupCount);
    for (uint16_t i = 0; i < seqLookupCount; i++) {
      if (auto seq = SequenceLookup::Read(in); seq) {
        r->seqLookups.push_back(*seq);
      } else {
        return nullptr;
      }
    }

    for (Offset16 offset : backtrackCoverageOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto cov = CoverageReader::Read(in); cov) {
        r->backtrackCoverage.push_back(cov);
      } else {
        return nullptr;
      }
    }
    for (Offset16 offset : inputCoverageOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto cov = CoverageReader::Read(in); cov) {
        r->inputCoverage.push_back(cov);
      } else {
        return nullptr;
      }
    }
    for (Offset16 offset : lookaheadCoverageOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      if (auto cov = CoverageReader::Read(in); cov) {
        r->lookaheadCoverage.push_back(cov);
      } else {
        return nullptr;
      }
    }

    return r;
  }

  bool write(OutputStream &out) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(3)) {
      return false;
    }

    if (!out.sizeU16(backtrackCoverage.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> backtrackCoverageOffsets;
    for (auto const &it : backtrackCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      backtrackCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(inputCoverage.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> inputCoverageOffsets;
    for (auto const &it : inputCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      inputCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(lookaheadCoverage.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> lookaheadCoverageOffsets;
    for (auto const &it : lookaheadCoverage) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      lookaheadCoverageOffsets.push_back(h);
    }

    if (!out.sizeU16(seqLookups.size())) {
      return false;
    }
    for (auto const &seq : seqLookups) {
      if (!seq.write(out)) {
        return false;
      }
    }

    for (size_t i = 0; i < backtrackCoverage.size(); i++) {
      auto const &cov = backtrackCoverage[i];
      auto handle = backtrackCoverageOffsets[i];
      if (!handle->mark()) {
        return false;
      }
      if (!cov->write(out)) {
        return false;
      }
    }

    for (size_t i = 0; i < inputCoverage.size(); i++) {
      auto const &cov = inputCoverage[i];
      auto handle = inputCoverageOffsets[i];
      if (!handle->mark()) {
        return false;
      }
      if (!cov->write(out)) {
        return false;
      }
    }

    for (size_t i = 0; i < lookaheadCoverage.size(); i++) {
      auto const &cov = lookaheadCoverage[i];
      auto handle = lookaheadCoverageOffsets[i];
      if (!handle->mark()) {
        return false;
      }
      if (!cov->write(out)) {
        return false;
      }
    }

    return beginning->commit();
  }

public:
  std::vector<std::shared_ptr<Coverage>> backtrackCoverage;
  std::vector<std::shared_ptr<Coverage>> inputCoverage;
  std::vector<std::shared_ptr<Coverage>> lookaheadCoverage;
  std::vector<SequenceLookup> seqLookups;
};

class ChainedContextsSubstitution {
public:
  static std::shared_ptr<Subtable> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format == 1) {
      return ChainedContextsSubstitution1::Read(in);
    } else if (format == 2) {
      return ChainedContextsSubstitution2::Read(in);
    } else if (format == 3) {
      return ChainedContextsSubstitution3::Read(in);
    } else {
      return nullptr;
    }
  }
};

} // namespace eglyf::gsub
