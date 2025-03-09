#pragma once

namespace eglyf::gsub {

class Ligature : public Subtable {
public:
  struct LigatureTable {
    uint16_t ligatureGlyph;
    std::vector<uint16_t> componentGlyphIDs;

    static Optional<LigatureTable> Read(InputStream &in) {
      using namespace std;
      LigatureTable r;
      if (!in.u16(&r.ligatureGlyph)) {
        return EGLYF_NULLOPT;
      }
      uint16_t componentCount;
      if (!in.u16(&componentCount)) {
        return EGLYF_NULLOPT;
      }
      if (componentCount < 0) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.componentGlyphIDs, componentCount - 1)) {
        return EGLYF_NULLOPT;
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.u16(ligatureGlyph)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(componentGlyphIDs.size() + 1)) {
        return EGLYF_ERROR;
      }
      if (out.u16a(componentGlyphIDs)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }
  };

  struct LigatureSet {
    std::vector<LigatureTable> ligatures;

    static Optional<LigatureSet> Read(InputStream &in) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t ligatureCount;
      if (!in.u16(&ligatureCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> ligatureOffsets;
      if (!in.o16a(ligatureOffsets, ligatureCount)) {
        return EGLYF_NULLOPT;
      }
      LigatureSet r;
      for (Offset16 offset : ligatureOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto l = LigatureTable::Read(in); l) {
          r.ligatures.push_back(*l);
        } else {
          return EGLYF_NULLOPT_PUSH(l.status());
        }
      }
      return r;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto beginning = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(ligatures.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> ligatureOffsets;
      for (size_t i = 0; i < ligatures.size(); i++) {
        auto offset = beginning->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        ligatureOffsets.push_back(offset);
      }
      for (size_t i = 0; i < ligatures.size(); i++) {
        auto const &ligature = ligatures[i];
        auto offset = ligatureOffsets[i];
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = ligature.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(beginning->commit());
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return EGLYF_ERROR;
    }
    uint16_t ligatureSetCount;
    if (!in.u16(&ligatureSetCount)) {
      return EGLYF_ERROR;
    }
    vector<Offset16> ligatureSetOffsets;
    if (!in.o16a(ligatureSetOffsets, ligatureSetCount)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<Ligature>();
    if (!in.seek(coverageOffset)) {
      return EGLYF_ERROR;
    }
    if (auto st = CoverageReader::Read(in, r->coverage); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (Offset16 offset : ligatureSetOffsets) {
      if (!in.seek(offset)) {
        return EGLYF_ERROR;
      }
      OffsetInputStream sub(in);
      if (auto s = LigatureSet::Read(sub); s) {
        r->ligatureSets.push_back(*s);
      } else {
        return EGLYF_STATUS_PUSH(s.status());
      }
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
    if (!out.sizeU16(ligatureSets.size())) {
      return EGLYF_ERROR;
    }
    vector<OffsetWriter::Handle16> ligatureSetOffsets;
    for (size_t i = 0; i < ligatureSets.size(); i++) {
      auto h = beginning->o16();
      if (!h) {
        return EGLYF_ERROR;
      }
      ligatureSetOffsets.push_back(h);
    }
    for (size_t i = 0; i < ligatureSets.size(); i++) {
      auto const &ligatureSet = ligatureSets[i];
      auto offset = ligatureSetOffsets[i];
      if (auto st = offset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = ligatureSet.write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    if (auto st = coverageOffset->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = coverage->write(out); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(beginning->commit());
  }

public:
  std::vector<LigatureSet> ligatureSets;
};

} // namespace eglyf::gsub
