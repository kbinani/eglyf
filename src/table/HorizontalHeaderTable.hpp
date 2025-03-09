#pragma once

namespace eglyf {

// 'hhea'
class HorizontalHeaderTable : public Table {
public:
  static Status Read(InputStream &in, std::shared_ptr<HorizontalHeaderTable> &out) {
    using namespace std;
    auto r = make_unique<HorizontalHeaderTable>();
    if (!in.u16(&r->majorVersion)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->minorVersion)) {
      return EGLYF_ERROR;
    }
    if (r->majorVersion != 1 || r->minorVersion != 0) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ascender)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->descender)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->lineGap)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->advanceWidthMax)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->minLeftSideBearing)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->minRightSideBearing)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->xMaxExtent)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->caretSlopeRise)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->caretSlopeRun)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->caretOffset)) {
      return EGLYF_ERROR;
    }
    int16_t reserved;
    if (!in.i16(&reserved)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&reserved)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&reserved)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&reserved)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->metricDataFormat)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->numberOfHMetrics)) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    if (majorVersion != 1 || minorVersion != 0) {
      return EGLYF_NULLOPT;
    }
    ByteOutputStream out;
    if (!out.u16(majorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(minorVersion)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ascender)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(descender)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(lineGap)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(advanceWidthMax)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(minLeftSideBearing)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(minRightSideBearing)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(xMaxExtent)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(caretSlopeRise)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(caretSlopeRun)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(caretOffset)) {
      return EGLYF_NULLOPT;
    }
    int16_t reserved = 0;
    if (!out.i16(reserved)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(reserved)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(reserved)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(reserved)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(metricDataFormat)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(numberOfHMetrics)) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

  Status clone(std::shared_ptr<HorizontalHeaderTable> &out) const {
    return EGLYF_STATUS_PUSH(defaultClone<HorizontalHeaderTable>(out));
  }

public:
  uint16_t majorVersion;
  uint16_t minorVersion;
  FWORD ascender;
  FWORD descender;
  FWORD lineGap;
  UFWORD advanceWidthMax;
  FWORD minLeftSideBearing;
  FWORD minRightSideBearing;
  FWORD xMaxExtent;
  int16_t caretSlopeRise;
  int16_t caretSlopeRun;
  int16_t caretOffset;
  int16_t metricDataFormat;
  uint16_t numberOfHMetrics;
};

} // namespace eglyf
