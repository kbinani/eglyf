#pragma once

namespace eglyf {

// 'vhea'
class VerticalHeaderTable : public Table {
public:
  struct Data10 {
    FWORD ascent;
    FWORD descent;
    FWORD lineGap;
    UFWORD advanceHeightMax;
    FWORD minTopSideBearing;
    FWORD minBottomSideBearing;
    FWORD yMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
    int16_t metricDataFormat;
    uint16_t numOfLongVerMetrics;

    static Optional<Data10> Read(InputStream &in) {
      Data10 r;
      if (!in.i16(&r.ascent)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.descent)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.lineGap)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.advanceHeightMax)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.minTopSideBearing)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.minBottomSideBearing)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.yMaxExtent)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretSlopeRise)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretSlopeRun)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretOffset)) {
        return EGLYF_NULLOPT;
      }
      int16_t reserved;
      for (int i = 0; i < 4; i++) {
        if (!in.i16(&reserved)) {
          return EGLYF_NULLOPT;
        }
      }
      if (!in.i16(&r.metricDataFormat)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.numOfLongVerMetrics)) {
        return EGLYF_NULLOPT;
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.u16(0x0001)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(0x0000)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(ascent)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(descent)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(lineGap)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(advanceHeightMax)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(minTopSideBearing)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(minBottomSideBearing)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(yMaxExtent)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretSlopeRise)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretSlopeRun)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretOffset)) {
        return EGLYF_ERROR;
      }
      int16_t reserved;
      for (int i = 0; i < 4; i++) {
        if (!out.i16(0)) {
          return EGLYF_ERROR;
        }
      }
      if (!out.i16(metricDataFormat)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(numOfLongVerMetrics)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

  struct Data11 {
    FWORD vertTypoAscender;
    FWORD vertTypoDescender;
    FWORD vertTypoLineGap;
    UFWORD advanceHeightMax;
    FWORD minTopSideBearing;
    FWORD minBottomSideBearing;
    FWORD yMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
    int16_t metricDataFormat;
    uint16_t numOfLongVerMetrics;

    static Optional<Data11> Read(InputStream &in) {
      Data11 r;
      if (!in.i16(&r.vertTypoAscender)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.vertTypoDescender)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.vertTypoLineGap)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.advanceHeightMax)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.minTopSideBearing)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.minBottomSideBearing)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.yMaxExtent)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretSlopeRise)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretSlopeRun)) {
        return EGLYF_NULLOPT;
      }
      if (!in.i16(&r.caretOffset)) {
        return EGLYF_NULLOPT;
      }
      int16_t reserved;
      for (int i = 0; i < 4; i++) {
        if (!in.i16(&reserved)) {
          return EGLYF_NULLOPT;
        }
      }
      if (!in.i16(&r.metricDataFormat)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16(&r.numOfLongVerMetrics)) {
        return EGLYF_NULLOPT;
      }
      return r;
    }

    Status write(OutputStream &out) const {
      if (!out.u16(0x0001)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(0x1000)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(vertTypoAscender)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(vertTypoDescender)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(vertTypoLineGap)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(advanceHeightMax)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(minTopSideBearing)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(minBottomSideBearing)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(yMaxExtent)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretSlopeRise)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretSlopeRun)) {
        return EGLYF_ERROR;
      }
      if (!out.i16(caretOffset)) {
        return EGLYF_ERROR;
      }
      int16_t reserved;
      for (int i = 0; i < 4; i++) {
        if (!out.i16(0)) {
          return EGLYF_ERROR;
        }
      }
      if (!out.i16(metricDataFormat)) {
        return EGLYF_ERROR;
      }
      if (!out.u16(numOfLongVerMetrics)) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }
  };

public:
  static Status Read(InputStream &in, std::shared_ptr<VerticalHeaderTable> &out) {
    using namespace std;
    Version16Dot16 dot;
    if (!in.u16(&dot.major)) {
      return EGLYF_ERROR;
    }
    if (dot.major != 0x0001) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&dot.minor)) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<VerticalHeaderTable>();
    switch (dot.minor) {
    case 0x0000:
      if (auto data = Data10::Read(in); data) {
        ret->data = *data;
        break;
      } else {
        return EGLYF_STATUS_PUSH(data.status());
      }
    case 0x1000:
      if (auto data = Data11::Read(in); data) {
        ret->data = *data;
        break;
      } else {
        return EGLYF_STATUS_PUSH(data.status());
      }
    default:
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (holds_alternative<Data10>(data)) {
      auto const &d = get<Data10>(data);
      if (auto st = d.write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else if (holds_alternative<Data11>(data)) {
      auto const &d = get<Data11>(data);
      if (auto st = d.write(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    } else {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

public:
  std::variant<Data10, Data11> data;
};

} // namespace eglyf
