#pragma once

namespace eglyf {

// 'GDEF'
class GlyphDefinitionTable : public Table {
public:
  struct AttachPoint {
    std::vector<uint16_t> pointIndices;

    static Optional<AttachPoint> Read(InputStream &in) {
      AttachPoint ret;
      uint16_t pointCount;
      if (!in.u16(&pointCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(ret.pointIndices, pointCount)) {
        return EGLYF_NULLOPT;
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      if (!out.sizeU16(pointIndices.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(pointIndices)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

  struct AttachList {
    std::shared_ptr<Coverage> coverage;
    std::vector<AttachPoint> attachPoints;

    static Optional<AttachList> Read(InputStream &stream) {
      using namespace std;
      OffsetInputStream in(&stream);
      Offset16 coverageOffset;
      if (!in.o16(&coverageOffset)) {
        return EGLYF_NULLOPT;
      }
      uint16_t glyphCount;
      if (!in.u16(&glyphCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> attachPointOffsets;
      if (!in.o16a(attachPointOffsets, glyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.seek(coverageOffset)) {
        return EGLYF_NULLOPT;
      }
      AttachList ret;
      if (auto st = CoverageReader::Read(in, ret.coverage); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      for (auto offset : attachPointOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto attachPoint = AttachPoint::Read(in); attachPoint) {
          ret.attachPoints.push_back(*attachPoint);
        } else {
          return EGLYF_NULLOPT_PUSH(attachPoint.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &out) const {
      using namespace std;
      auto writer = make_shared<OffsetWriter>(out);
      auto coverageOffset = writer->o16();
      if (!coverageOffset) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(attachPoints.size())) {
        return EGLYF_ERROR;
      }
      vector<OffsetWriter::Handle16> attachPointOffsets;
      for (size_t i = 0; i < attachPoints.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        attachPointOffsets.push_back(offset);
      }
      if (auto st = coverageOffset->mark(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (auto st = coverage->write(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      for (size_t i = 0; i < attachPoints.size(); i++) {
        auto const &attachPoint = attachPoints[i];
        auto offset = attachPointOffsets[i];
        if (auto st = offset->mark(); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        if (auto st = attachPoint.write(out); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }
  };

  class CaretValue {
  public:
    virtual ~CaretValue() {}
    virtual Status write(OutputStream &) const = 0;
  };

  class CaretValue1 : public CaretValue {
  public:
    static Status Read(InputStream &in, std::shared_ptr<CaretValue> &out) {
      using namespace std;
      auto ret = make_unique<CaretValue1>();
      if (!in.i16(&ret->coordinate)) {
        return EGLYF_ERROR;
      }
      out.reset(ret.release());
      return Status::Ok();
    }

    Status write(OutputStream &out) const override {
      if (!out.u16(1)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(coordinate)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }

  public:
    int16_t coordinate;
  };

  class CaretValue2 : public CaretValue {
  public:
    static Status Read(InputStream &in, std::shared_ptr<CaretValue> &out) {
      using namespace std;
      auto ret = make_unique<CaretValue2>();
      if (!in.u16(&ret->caretValuePointIndex)) {
        return EGLYF_ERROR;
      }
      out.reset(ret.release());
      return Status::Ok();
    }

    Status write(OutputStream &out) const override {
      if (!out.u16(2)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(caretValuePointIndex)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }

  public:
    uint16_t caretValuePointIndex;
  };

  class CaretValueReader {
    CaretValueReader() = delete;

  public:
    static Status Read(InputStream &in, std::shared_ptr<CaretValue> &out) {
      using namespace std;
      uint16_t format;
      if (!in.u16(&format)) {
        return EGLYF_ERROR;
      }
      switch (format) {
      case 1:
        return EGLYF_STATUS_PUSH(CaretValue1::Read(in, out));
      case 2:
        return EGLYF_STATUS_PUSH(CaretValue2::Read(in, out));
      case 3:
        return EGLYF_ERROR_WHAT("Unsupported format: CaretValueFormat3");
      default:
        return EGLYF_ERROR;
      }
    }
  };

  struct LigGlyph {
    std::vector<std::shared_ptr<CaretValue>> caretValues;

    static Optional<LigGlyph> Read(InputStream &stream) {
      using namespace std;
      OffsetInputStream in(&stream);
      uint16_t caretCount;
      if (!in.u16(&caretCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> caretValueOffsets;
      if (!in.u16a(caretValueOffsets, caretCount)) {
        return EGLYF_NULLOPT;
      }
      LigGlyph ret;
      for (auto offset : caretValueOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        shared_ptr<CaretValue> caretValue;
        if (auto st = CaretValueReader::Read(in, caretValue); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        ret.caretValues.push_back(caretValue);
      }
      return ret;
    }

    Status write(OutputStream &stream) const {
      using namespace std;
      auto writer = make_shared<DataFragmentWriter>(&stream);
      if (!writer->sizeU16(caretValues.size())) {
        return EGLYF_ERROR;
      }
      vector<DataFragmentWriter::Marker16> caretValueOffsets;
      for (size_t i = 0; i < caretValues.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        caretValueOffsets.push_back(offset);
      }
      for (size_t i = 0; i < caretValues.size(); i++) {
        auto const &caretValue = caretValues[i];
        auto offset = caretValueOffsets[i];
        if (auto st = writer->writeDataFragment(offset, *caretValue); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }
  };

  struct LigCaretList {
    std::shared_ptr<Coverage> coverage;
    std::vector<LigGlyph> ligGlyphs;

    static Optional<LigCaretList> Read(InputStream &stream) {
      using namespace std;
      OffsetInputStream in(&stream);
      LigCaretList ret;
      Offset16 coverageOffset;
      if (!in.o16(&coverageOffset)) {
        return EGLYF_NULLOPT;
      }
      uint16_t ligGlyphCount;
      if (!in.u16(&ligGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      vector<Offset16> ligGlyphOffsets;
      if (!in.o16a(ligGlyphOffsets, ligGlyphCount)) {
        return EGLYF_NULLOPT;
      }
      if (!in.seek(coverageOffset)) {
        return EGLYF_NULLOPT;
      }
      if (auto st = CoverageReader::Read(in, ret.coverage); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      for (auto offset : ligGlyphOffsets) {
        if (!in.seek(offset)) {
          return EGLYF_NULLOPT;
        }
        if (auto ligGlyph = LigGlyph::Read(in); ligGlyph) {
          ret.ligGlyphs.push_back(*ligGlyph);
        } else {
          return EGLYF_NULLOPT_PUSH(ligGlyph.status());
        }
      }
      return ret;
    }

    Status write(OutputStream &stream) const {
      using namespace std;
      auto writer = make_shared<DataFragmentWriter>(&stream);
      auto coverageOffset = writer->o16();
      if (!coverageOffset) {
        return EGLYF_ERROR;
      }
      if (!writer->sizeU16(ligGlyphs.size())) {
        return EGLYF_ERROR;
      }
      vector<DataFragmentWriter::Marker16> ligGlyphOffsets;
      for (size_t i = 0; i < ligGlyphs.size(); i++) {
        auto offset = writer->o16();
        if (!offset) {
          return EGLYF_ERROR;
        }
        ligGlyphOffsets.push_back(offset);
      }
      if (auto st = writer->writeDataFragment(coverageOffset, *coverage); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      for (size_t i = 0; i < ligGlyphs.size(); i++) {
        auto const &ligGlyph = ligGlyphs[i];
        auto offset = ligGlyphOffsets[i];
        if (auto st = writer->writeDataFragment(offset, ligGlyph); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      return EGLYF_STATUS_PUSH(writer->commit());
    }
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<GlyphDefinitionTable> &out) {
    using namespace std;
    OffsetInputStream in(&stream);
    auto ret = make_unique<GlyphDefinitionTable>();
    if (!in.u16(&ret->majorVersion)) {
      return EGLYF_ERROR;
    }
    if (ret->majorVersion != 1) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&ret->minorVersion)) {
      return EGLYF_ERROR;
    }
    if (ret->minorVersion != 0 && ret->minorVersion != 2 && ret->minorVersion != 3) {
      return EGLYF_ERROR;
    }
    Offset16 glyphClassDefOffset;
    if (!in.o16(&glyphClassDefOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 attachListOffset;
    if (!in.o16(&attachListOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 ligCaretListOffset;
    if (!in.o16(&ligCaretListOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 markAttachClassDefOffset;
    if (!in.o16(&markAttachClassDefOffset)) {
      return EGLYF_ERROR;
    }
    Offset16 markGlyphSetsDefOffset = 0;
    if (ret->minorVersion > 1) {
      if (!in.o16(&markGlyphSetsDefOffset)) {
        return EGLYF_ERROR;
      }
    }
    Offset16 itemVarStoreOffset = 0;
    if (ret->minorVersion > 2) {
      if (!in.o16(&itemVarStoreOffset)) {
        return EGLYF_ERROR;
      }
    }

    if (glyphClassDefOffset > 0) {
      if (!in.seek(glyphClassDefOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = ClassDefReader::Read(in, ret->glyphClassDef); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (attachListOffset > 0) {
      if (!in.seek(attachListOffset)) {
        return EGLYF_ERROR;
      }
      if (auto attachList = AttachList::Read(in); attachList) {
        ret->attachList = *attachList;
      } else {
        return EGLYF_STATUS_PUSH(attachList.status());
      }
    }

    if (ligCaretListOffset > 0) {
      if (!in.seek(ligCaretListOffset)) {
        return EGLYF_ERROR;
      }
      if (auto ligCaretList = LigCaretList::Read(in); ligCaretList) {
        ret->ligCaretList = *ligCaretList;
      } else {
        return EGLYF_STATUS_PUSH(ligCaretList.status());
      }
    }

    if (markAttachClassDefOffset > 0) {
      if (!in.seek(markAttachClassDefOffset)) {
        return EGLYF_ERROR;
      }
      if (auto st = ClassDefReader::Read(in, ret->markAttachClassDef); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    if (markGlyphSetsDefOffset > 0) {
      if (!in.seek(markGlyphSetsDefOffset)) {
        return EGLYF_ERROR;
      }
      if (auto markGlyphSets = MarkGlyphSets::Read(in); markGlyphSets) {
        ret->markGlyphSets = *markGlyphSets;
      } else {
        return EGLYF_STATUS_PUSH(markGlyphSets.status());
      }
    }

    if (itemVarStoreOffset > 0) {
      return EGLYF_ERROR_WHAT("Unsupported format: itemVarStore");
    }

    out.reset(ret.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    if (majorVersion != 1) {
      return EGLYF_NULLOPT;
    }
    if (minorVersion != 0 && minorVersion != 2 && minorVersion != 3) {
      return EGLYF_NULLOPT;
    }
    ByteOutputStream out;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(majorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(minorVersion)) {
      return EGLYF_NULLOPT;
    }
    auto glyphClassDefOffset = writer->o16();
    if (!glyphClassDefOffset) {
      return EGLYF_NULLOPT;
    }
    auto attachListOffset = writer->o16();
    if (!attachListOffset) {
      return EGLYF_NULLOPT;
    }
    auto ligCaretListOffset = writer->o16();
    if (!ligCaretListOffset) {
      return EGLYF_NULLOPT;
    }
    auto markAttachClassDefOffset = writer->o16();
    if (!markAttachClassDefOffset) {
      return EGLYF_NULLOPT;
    }
    OffsetWriter::Handle16 markGlyphSetsDefOffset;
    if (minorVersion > 1) {
      markGlyphSetsDefOffset = writer->o16();
      if (!markGlyphSetsDefOffset) {
        return EGLYF_NULLOPT;
      }
    }
    OffsetWriter::Handle16 itemVarStoreOffset;
    if (minorVersion > 2) {
      itemVarStoreOffset = writer->o16();
      if (!itemVarStoreOffset) {
        return EGLYF_NULLOPT;
      }
    }
    if (glyphClassDef) {
      if (auto st = glyphClassDefOffset->mark(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      if (auto st = glyphClassDef->write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else {
      if (auto st = glyphClassDefOffset->null(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (attachList) {
      if (auto st = attachListOffset->mark(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      if (auto st = attachList->write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else {
      if (auto st = attachListOffset->null(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (ligCaretList) {
      if (auto st = ligCaretListOffset->mark(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      if (auto st = ligCaretList->write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else {
      if (auto st = ligCaretListOffset->null(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (markAttachClassDef) {
      if (auto st = markAttachClassDefOffset->mark(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
      if (auto st = markAttachClassDef->write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else {
      if (auto st = markAttachClassDefOffset->null(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (minorVersion > 1) {
      if (markGlyphSets) {
        if (auto st = markGlyphSetsDefOffset->mark(); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
        if (auto st = markGlyphSets->write(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      } else {
        if (auto st = markGlyphSetsDefOffset->null(); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      }
    }
    if (minorVersion > 2) {
      if (auto st = itemVarStoreOffset->null(); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (auto st = writer->commit(); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }

    return EncodeResult(out.data());
  }

public:
  uint16_t majorVersion;
  uint16_t minorVersion;
  std::shared_ptr<ClassDef> glyphClassDef;
  std::optional<AttachList> attachList;
  std::optional<LigCaretList> ligCaretList;
  std::shared_ptr<ClassDef> markAttachClassDef;
  std::optional<MarkGlyphSets> markGlyphSets;
};

} // namespace eglyf
