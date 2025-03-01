#pragma once

namespace ksesh {

// 'head'
class FontHeaderTable : public Table {
public:
  static std::shared_ptr<FontHeaderTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<FontHeaderTable>();
    if (!in.u16(&r->majorVersion)) {
      return nullptr;
    }
    if (!in.u16(&r->minorVersion)) {
      return nullptr;
    }
    if (!in.u32(&r->fontRevision.value)) {
      return nullptr;
    }
    if (!in.u32(&r->checksumAdjustment)) {
      return nullptr;
    }
    if (!in.u32(&r->magicNumber)) {
      return nullptr;
    }
    if (!in.u16(&r->flags)) {
      return nullptr;
    }
    if (!in.u16(&r->unitsPerEm)) {
      return nullptr;
    }
    if (!in.i64(&r->created)) {
      return nullptr;
    }
    if (!in.i64(&r->modified)) {
      return nullptr;
    }
    if (!in.i16(&r->xMin)) {
      return nullptr;
    }
    if (!in.i16(&r->yMin)) {
      return nullptr;
    }
    if (!in.i16(&r->xMax)) {
      return nullptr;
    }
    if (!in.i16(&r->yMax)) {
      return nullptr;
    }
    if (!in.u16(&r->macStyle)) {
      return nullptr;
    }
    if (!in.u16(&r->lowestRecPPEM)) {
      return nullptr;
    }
    if (!in.i16(&r->fontDirectionHint)) {
      return nullptr;
    }
    if (!in.i16(&r->indexToLocFormat)) {
      return nullptr;
    }
    if (!in.i16(&r->glyphDataFormat)) {
      return nullptr;
    }
    return r;
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(majorVersion)) {
      return nullopt;
    }
    if (!out.u16(minorVersion)) {
      return nullopt;
    }
    if (!out.u32(fontRevision.value)) {
      return nullopt;
    }
    if (!out.u32(checksumAdjustment)) {
      return nullopt;
    }
    if (!out.u32(magicNumber)) {
      return nullopt;
    }
    if (!out.u16(flags)) {
      return nullopt;
    }
    if (!out.u16(unitsPerEm)) {
      return nullopt;
    }
    if (!out.i64(created)) {
      return nullopt;
    }
    if (!out.i64(modified)) {
      return nullopt;
    }
    if (!out.i16(xMin)) {
      return nullopt;
    }
    if (!out.i16(yMin)) {
      return nullopt;
    }
    if (!out.i16(xMax)) {
      return nullopt;
    }
    if (!out.i16(yMax)) {
      return nullopt;
    }
    if (!out.u16(macStyle)) {
      return nullopt;
    }
    if (!out.u16(lowestRecPPEM)) {
      return nullopt;
    }
    if (!out.i16(fontDirectionHint)) {
      return nullopt;
    }
    if (!out.i16(indexToLocFormat)) {
      return nullopt;
    }
    if (!out.i16(glyphDataFormat)) {
      return nullopt;
    }
    return EncodeResult(out.data());
  }

public:
  uint16_t majorVersion;
  uint16_t minorVersion;
  Fixed fontRevision;
  uint32_t checksumAdjustment;
  uint32_t magicNumber;
  uint16_t flags;
  uint16_t unitsPerEm;
  LONGDATETIME created;
  LONGDATETIME modified;
  int16_t xMin;
  int16_t yMin;
  int16_t xMax;
  int16_t yMax;
  uint16_t macStyle;
  uint16_t lowestRecPPEM;
  int16_t fontDirectionHint;
  int16_t indexToLocFormat;
  int16_t glyphDataFormat;
};

} // namespace ksesh
