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

public:
  std::vector<ChainedSequenceRuleSet> ruleSets;
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

public:
  std::shared_ptr<ClassDef> backtrackClassDef;
  std::shared_ptr<ClassDef> inputClassDef;
  std::shared_ptr<ClassDef> lookaheadClassDef;
  std::vector<ChainedClassSequenceRuleSet> ruleSets;
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
      // TODO:
      return nullptr;
    }
    return nullptr;
  }
};

} // namespace eglyf::gsub
