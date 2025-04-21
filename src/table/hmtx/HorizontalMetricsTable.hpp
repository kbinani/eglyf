#pragma once

namespace eglyf::hmtx {

// 'hmtx'
class HorizontalMetricsTable : public Table {
public:
  struct LongHorMetric {
    UFWORD advanceWidth;
    FWORD lsb;
  };

public:
  static Status Read(InputStream &in, uint16_t numGlyphs, uint16_t numberOfHMetrics, std::shared_ptr<HorizontalMetricsTable> &out) {
    using namespace std;
    if (numGlyphs < numberOfHMetrics) {
      return EGLYF_ERROR;
    }
    if (numberOfHMetrics == 0) {
      return EGLYF_ERROR;
    }
    uint16_t count = numGlyphs - numberOfHMetrics;
    auto r = make_unique<HorizontalMetricsTable>();
    r->metrics.reserve(numGlyphs);
    for (uint16_t i = 0; i < numberOfHMetrics; i++) {
      LongHorMetric m;
      if (!in.u16(&m.advanceWidth)) {
        return EGLYF_ERROR;
      }
      if (!in.i16(&m.lsb)) {
        return EGLYF_ERROR;
      }
      r->metrics.push_back(m);
    }
    UFWORD const advanceWidth = r->metrics.back().advanceWidth;
    for (uint16_t i = 0; i < count; i++) {
      LongHorMetric m;
      m.advanceWidth = advanceWidth;
      if (!in.i16(&m.lsb)) {
        return EGLYF_ERROR;
      }
      r->metrics.push_back(m);
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT;
  }

  Optional<EncodeResult> encode(uint16_t &numberOfHMetrics) const {
    using namespace std;
    if (metrics.empty()) {
      return EncodeResult("");
    }
    numberOfHMetrics = metrics.size();
    UFWORD const advanceWidth = metrics.back().advanceWidth;
    for (int i = numberOfHMetrics - 1; i >= 0; i--) {
      auto v = metrics[i].advanceWidth;
      if (advanceWidth != v) {
        break;
      }
      numberOfHMetrics = i + 1;
    }
    ByteOutputStream out;
    for (uint16_t i = 0; i < numberOfHMetrics; i++) {
      auto const &m = metrics[i];
      if (!out.u16(m.advanceWidth)) {
        return EGLYF_NULLOPT;
      }
      if (!out.i16(m.lsb)) {
        return EGLYF_NULLOPT;
      }
    }
    for (uint16_t i = numberOfHMetrics; i < metrics.size(); i++) {
      auto const &m = metrics[i];
      if (!out.i16(m.lsb)) {
        return EGLYF_NULLOPT;
      }
    }
    return EncodeResult(out.data());
  }

  Status clone(std::shared_ptr<HorizontalMetricsTable> &out) const {
    uint16_t numberOfHMetrics = 0;
    auto encoded = encode(numberOfHMetrics);
    if (!encoded) {
      return EGLYF_STATUS_PUSH(encoded.status());
    }
    uint16_t numGlyphs = metrics.size();
    ByteInputStream in(encoded->data);
    return EGLYF_STATUS_PUSH(Read(in, numGlyphs, numberOfHMetrics, out));
  }

public:
  std::vector<LongHorMetric> metrics;
};

} // namespace eglyf::hmtx
