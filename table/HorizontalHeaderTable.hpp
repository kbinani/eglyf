#pragma once

namespace ksesh::otf {

// 'hhea'
class HorizontalHeaderTable : public Table {
public:
  static std::shared_ptr<HorizontalHeaderTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<HorizontalHeaderTable>();
    if (!in.u16(&r->majorVersion)) {
      return nullptr;
    }
    if (!in.u16(&r->minorVersion)) {
      return nullptr;
    }
    if (r->majorVersion != 1 || r->minorVersion != 0) {
      return nullptr;
    }
    if (!in.i16(&r->ascender)) {
      return nullptr;
    }
    if (!in.i16(&r->descender)) {
      return nullptr;
    }
    if (!in.i16(&r->lineGap)) {
      return nullptr;
    }
    if (!in.u16(&r->advanceWidthMax)) {
      return nullptr;
    }
    if (!in.i16(&r->minLeftSideBearing)) {
      return nullptr;
    }
    if (!in.i16(&r->minRightSideBearing)) {
      return nullptr;
    }
    if (!in.i16(&r->xMaxExtent)) {
      return nullptr;
    }
    if (!in.i16(&r->caretSlopeRise)) {
      return nullptr;
    }
    if (!in.i16(&r->caretSlopeRun)) {
      return nullptr;
    }
    if (!in.i16(&r->caretOffset)) {
      return nullptr;
    }
    int16_t reserved;
    if (!in.i16(&reserved)) {
      return nullptr;
    }
    if (!in.i16(&reserved)) {
      return nullptr;
    }
    if (!in.i16(&reserved)) {
      return nullptr;
    }
    if (!in.i16(&reserved)) {
      return nullptr;
    }
    if (!in.i16(&r->metricDataFormat)) {
      return nullptr;
    }
    if (!in.u16(&r->numberOfHMetrics)) {
      return nullptr;
    }
    return r;
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    if (majorVersion != 1 || minorVersion != 0) {
      return nullopt;
    }
    ByteOutputStream out;
    if (!out.u16(majorVersion)) {
      return nullopt;
    }
    if (!out.u16(minorVersion)) {
      return nullopt;
    }
    if (!out.i16(ascender)) {
      return nullopt;
    }
    if (!out.i16(descender)) {
      return nullopt;
    }
    if (!out.i16(lineGap)) {
      return nullopt;
    }
    if (!out.u16(advanceWidthMax)) {
      return nullopt;
    }
    if (!out.i16(minLeftSideBearing)) {
      return nullopt;
    }
    if (!out.i16(minRightSideBearing)) {
      return nullopt;
    }
    if (!out.i16(xMaxExtent)) {
      return nullopt;
    }
    if (!out.i16(caretSlopeRise)) {
      return nullopt;
    }
    if (!out.i16(caretSlopeRun)) {
      return nullopt;
    }
    if (!out.i16(caretOffset)) {
      return nullopt;
    }
    int16_t reserved = 0;
    if (!out.i16(reserved)) {
      return nullopt;
    }
    if (!out.i16(reserved)) {
      return nullopt;
    }
    if (!out.i16(reserved)) {
      return nullopt;
    }
    if (!out.i16(reserved)) {
      return nullopt;
    }
    if (!out.i16(metricDataFormat)) {
      return nullopt;
    }
    if (!out.u16(numberOfHMetrics)) {
      return nullopt;
    }
    return EncodeResult(out.data());
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

} // namespace ksesh::otf
