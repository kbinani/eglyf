#pragma once

namespace eglyf {

// 'hmtx'
class HorizontalMetricsTable : public Table {
public:
  struct LongHorMetric {
    UFWORD advanceWidth;
    FWORD lsb;
  };

public:
  static std::shared_ptr<HorizontalMetricsTable> Read(InputStream &in, uint16_t numGlyphs, uint16_t numberOfHMetrics) {
    using namespace std;
    if (numGlyphs < numberOfHMetrics) {
      return nullptr;
    }
    if (numberOfHMetrics == 0) {
      return nullptr;
    }
    uint16_t count = numGlyphs - numberOfHMetrics;
    auto r = make_shared<HorizontalMetricsTable>();
    r->metrics.reserve(numGlyphs);
    for (uint16_t i = 0; i < numberOfHMetrics; i++) {
      LongHorMetric m;
      if (!in.u16(&m.advanceWidth)) {
        return nullptr;
      }
      if (!in.i16(&m.lsb)) {
        return nullptr;
      }
      r->metrics.push_back(m);
    }
    UFWORD const advanceWidth = r->metrics.back().advanceWidth;
    for (uint16_t i = 0; i < count; i++) {
      LongHorMetric m;
      m.advanceWidth = advanceWidth;
      if (!in.i16(&m.lsb)) {
        return nullptr;
      }
      r->metrics.push_back(m);
    }
    return r;
  }

  std::optional<EncodeResult> encode() const override {
    return std::nullopt;
  }

  std::optional<EncodeResult> encode(uint16_t &numberOfHMetrics) const {
    using namespace std;
    if (metrics.empty()) {
      return EncodeResult("");
    }
    numberOfHMetrics = metrics.size();
    UFWORD const advanceWidth = metrics.back().advanceWidth;
    for (int i = numberOfHMetrics - 1; i >= 0; i--) {
      if (advanceWidth != metrics[i].advanceWidth) {
        break;
      }
      numberOfHMetrics = i;
    }
    ByteOutputStream out;
    for (uint16_t i = 0; i < numberOfHMetrics; i++) {
      auto const &m = metrics[i];
      if (!out.u16(m.advanceWidth)) {
        return nullopt;
      }
      if (!out.i16(m.lsb)) {
        return nullopt;
      }
    }
    for (uint16_t i = numberOfHMetrics; i < metrics.size(); i++) {
      auto const &m = metrics[i];
      if (!out.i16(m.lsb)) {
        return nullopt;
      }
    }
    return EncodeResult(out.data());
  }

  std::shared_ptr<HorizontalMetricsTable> clone() const {
    uint16_t numberOfHMetrics = 0;
    auto encoded = encode(numberOfHMetrics);
    if (!encoded) {
      return nullptr;
    }
    uint16_t numGlyphs = metrics.size();
    ByteInputStream in(encoded->data);
    return Read(in, numGlyphs, numberOfHMetrics);
  }

public:
  std::vector<LongHorMetric> metrics;
};

} // namespace eglyf
