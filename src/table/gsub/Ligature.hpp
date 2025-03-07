#pragma once

namespace eglyf::gsub {

class Ligature : public Subtable {
public:
  struct LigatureTable {
    uint16_t ligatureGlyph;
    std::vector<uint16_t> componentGlyphIDs;

    static std::optional<LigatureTable> Read(InputStream &in) {
      using namespace std;
      LigatureTable r;
      if (!in.u16(&r.ligatureGlyph)) {
        return nullopt;
      }
      uint16_t componentCount;
      if (!in.u16(&componentCount)) {
        return nullopt;
      }
      if (componentCount < 0) {
        return nullopt;
      }
      r.componentGlyphIDs.reserve(componentCount - 1);
      for (uint16_t i = 1; i < componentCount; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.componentGlyphIDs.push_back(v);
      }
      return r;
    }

    bool write(OutputStream &out) const {
      if (!out.u16(ligatureGlyph)) {
        return false;
      }
      if (!out.sizeU16(componentGlyphIDs.size() + 1)) {
        return false;
      }
      return out.u16a(componentGlyphIDs);
    }
  };

  struct LigatureSet {
    std::vector<LigatureTable> ligatures;

    static std::optional<LigatureSet> Read(InputStream &in) {
      using namespace std;
      jassert(in.position() == 0);
      uint16_t ligatureCount;
      if (!in.u16(&ligatureCount)) {
        return nullopt;
      }
      vector<Offset16> ligatureOffsets;
      ligatureOffsets.reserve(ligatureCount);
      for (uint16_t i = 0; i < ligatureCount; i++) {
        Offset16 v;
        if (!in.o16(&v)) {
          return nullopt;
        }
        ligatureOffsets.push_back(v);
      }
      LigatureSet r;
      for (Offset16 offset : ligatureOffsets) {
        if (!in.seek(offset)) {
          return nullopt;
        }
        if (auto l = LigatureTable::Read(in); l) {
          r.ligatures.push_back(*l);
        } else {
          return nullopt;
        }
      }
      return r;
    }

    bool write(OutputStream &out) const {
      using namespace std;
      auto beginning = make_shared<OffsetWriter>(out);
      if (!out.sizeU16(ligatures.size())) {
        return false;
      }
      vector<OffsetWriter::Handle16> ligatureOffsets;
      for (size_t i = 0; i < ligatures.size(); i++) {
        auto offset = beginning->o16();
        if (!offset) {
          return false;
        }
        ligatureOffsets.push_back(offset);
      }
      for (size_t i = 0; i < ligatures.size(); i++) {
        auto const &ligature = ligatures[i];
        auto offset = ligatureOffsets[i];
        if (!offset->mark()) {
          return false;
        }
        if (!ligature.write(out)) {
          return false;
        }
      }
      return beginning->commit();
    }
  };

public:
  static std::shared_ptr<Ligature> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format != 1) {
      return nullptr;
    }
    Offset16 coverageOffset;
    if (!in.o16(&coverageOffset)) {
      return nullptr;
    }
    uint16_t ligatureSetCount;
    if (!in.u16(&ligatureSetCount)) {
      return nullptr;
    }
    vector<Offset16> ligatureSetOffsets;
    ligatureSetOffsets.reserve(ligatureSetCount);
    for (uint16_t i = 0; i < ligatureSetCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullptr;
      }
      ligatureSetOffsets.push_back(v);
    }
    auto r = make_shared<Ligature>();
    if (!in.seek(coverageOffset)) {
      return nullptr;
    }
    if (auto cov = CoverageReader::Read(in); cov) {
      r->coverage = cov;
    } else {
      return nullptr;
    }
    for (Offset16 offset : ligatureSetOffsets) {
      if (!in.seek(offset)) {
        return nullptr;
      }
      OffsetInputStream sub(in);
      if (auto s = LigatureSet::Read(sub); s) {
        r->ligatureSets.push_back(*s);
      } else {
        return nullptr;
      }
    }
    return r;
  }

  bool write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &) override {
    using namespace std;
    auto beginning = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return false;
    }
    auto coverageOffset = beginning->o16();
    if (!coverageOffset) {
      return false;
    }
    if (!out.sizeU16(ligatureSets.size())) {
      return false;
    }
    vector<OffsetWriter::Handle16> ligatureSetOffsets;
    for (size_t i = 0; i < ligatureSets.size(); i++) {
      auto h = beginning->o16();
      if (!h) {
        return false;
      }
      ligatureSetOffsets.push_back(h);
    }
    for (size_t i = 0; i < ligatureSets.size(); i++) {
      auto const &ligatureSet = ligatureSets[i];
      auto offset = ligatureSetOffsets[i];
      if (!offset->mark()) {
        return false;
      }
      if (!ligatureSet.write(out)) {
        return false;
      }
    }
    if (!coverageOffset->mark()) {
      return false;
    }
    if (!coverage->write(out)) {
      return false;
    }
    return beginning->commit();
  }

public:
  std::vector<LigatureSet> ligatureSets;
};

} // namespace eglyf::gsub
