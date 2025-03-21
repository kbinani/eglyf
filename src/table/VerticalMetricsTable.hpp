#pragma once

namespace eglyf {

class VerticalMetricsTable : public Table {
public:
  struct Metric {
    UFWORD advanceHeight;
    FWORD topSideBearing;
  };

public:
  static Status Read(InputStream &in, uint16_t numGlyphs, uint16_t numOfLongVerMetrics, std::shared_ptr<VerticalMetricsTable> &out) {
    using namespace std;
    if (numGlyphs < numOfLongVerMetrics) {
      return EGLYF_ERROR;
    }
    if (numOfLongVerMetrics == 0) {
      return EGLYF_ERROR;
    }
    uint16_t count = numGlyphs - numOfLongVerMetrics;
    auto ret = make_unique<VerticalMetricsTable>();
    for (uint16_t i = 0; i < numOfLongVerMetrics; i++) {
      Metric m;
      if (!in.u16(&m.advanceHeight)) {
        return EGLYF_ERROR;
      }
      if (!in.i16(&m.topSideBearing)) {
        return EGLYF_ERROR;
      }
      ret->metrics.push_back(m);
    }
    UFWORD const advanceHeight = ret->metrics.back().advanceHeight;
    for (uint16_t i = 0; i < count; i++) {
      Metric m;
      m.advanceHeight = advanceHeight;
      if (!in.i16(&m.topSideBearing)) {
        return EGLYF_ERROR;
      }
      ret->metrics.push_back(m);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT;
  }

  Optional<EncodeResult> encode(uint16_t &numOfLongVerMetrics) const {
    using namespace std;
    if (metrics.empty()) {
      return EncodeResult("");
    }
    numOfLongVerMetrics = metrics.size();
    UFWORD const advanceHeight = metrics.back().advanceHeight;
    for (int i = numOfLongVerMetrics - 1; i >= 0; i--) {
      auto v = metrics[i].advanceHeight;
      if (advanceHeight != v) {
        break;
      }
      numOfLongVerMetrics = i + 1;
    }
    ByteOutputStream out;
    for (uint16_t i = 0; i < numOfLongVerMetrics; i++) {
      auto const &m = metrics[i];
      if (!out.u16(m.advanceHeight)) {
        return EGLYF_NULLOPT;
      }
      if (!out.i16(m.topSideBearing)) {
        return EGLYF_NULLOPT;
      }
    }
    for (uint16_t i = numOfLongVerMetrics; i < metrics.size(); i++) {
      auto const &m = metrics[i];
      if (!out.i16(m.topSideBearing)) {
        return EGLYF_NULLOPT;
      }
    }
    return EncodeResult(out.data());
  }

public:
  std::vector<Metric> metrics;
};

} // namespace eglyf
