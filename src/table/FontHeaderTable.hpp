#pragma once

namespace eglyf {

// 'head'
class FontHeaderTable : public Table {
public:
  static Status Read(InputStream &in, std::shared_ptr<FontHeaderTable> &out) {
    using namespace std;
    auto r = make_unique<FontHeaderTable>();
    if (!in.u16(&r->majorVersion)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->minorVersion)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->fontRevision.value)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->checksumAdjustment)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->magicNumber)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->flags)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->unitsPerEm)) {
      return EGLYF_ERROR;
    }
    if (!in.i64(&r->created)) {
      return EGLYF_ERROR;
    }
    if (!in.i64(&r->modified)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->xMin)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->yMin)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->xMax)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->yMax)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->macStyle)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->lowestRecPPEM)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->fontDirectionHint)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->indexToLocFormat)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->glyphDataFormat)) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(majorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(minorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(fontRevision.value)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(checksumAdjustment)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(magicNumber)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(flags)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(unitsPerEm)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i64(created)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i64(modified)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(xMin)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(yMin)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(xMax)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(yMax)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(macStyle)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(lowestRecPPEM)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(fontDirectionHint)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(indexToLocFormat)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(glyphDataFormat)) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

  Status clone(std::shared_ptr<FontHeaderTable> &out) const {
    return EGLYF_STATUS_PUSH(defaultClone<FontHeaderTable>(out));
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

} // namespace eglyf
