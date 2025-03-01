#pragma once

namespace ksesh {

// 'head'
class FontHeaderTable : public Table {
public:
  static std::shared_ptr<FontHeaderTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<FontHeaderTable>();
    r->majorVersion = in.u16();
    r->minorVersion = in.u16();
    r->fontRevision.value = in.u32();
    r->checksumAdjustment = in.u32();
    r->magicNumber = in.u32();
    r->flags = in.u16();
    r->unitsPerEm = in.u16();
    r->created = in.i64();
    r->modified = in.i64();
    r->xMin = in.i16();
    r->yMin = in.i16();
    r->xMax = in.i16();
    r->yMax = in.i16();
    r->macStyle = in.u16();
    r->lowestRecPPEM = in.u16();
    r->fontDirectionHint = in.i16();
    r->indexToLocFormat = in.i16();
    r->glyphDataFormat = in.i16();
    if (in.ok()) {
      return r;
    } else {
      return nullptr;
    }
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
    auto data = out.data();
    EncodeResult er;
    er.length = data.size();
    if (er.length % 4 != 0) {
      data.resize(er.length + (4 - (er.length % 4)));
    }
    er.data = data;
    return er;
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
