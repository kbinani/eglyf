#pragma once

namespace ksesh::otf {

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
    uint16_t count = numGlyphs - numberOfHMetrics;
    auto r = make_shared<HorizontalMetricsTable>();
    r->hMetrics.reserve(numberOfHMetrics);
    for (uint16_t i = 0; i < numberOfHMetrics; i++) {
      LongHorMetric m;
      if (!in.u16(&m.advanceWidth)) {
        return nullptr;
      }
      if (!in.i16(&m.lsb)) {
        return nullptr;
      }
      r->hMetrics.push_back(m);
    }
    r->leftSideBearings.reserve(count);
    for (uint16_t i = 0; i < count; i++) {
      FWORD v;
      if (!in.i16(&v)) {
        return nullptr;
      }
      r->leftSideBearings.push_back(v);
    }
    return r;
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    for (auto const &m : hMetrics) {
      if (!out.u16(m.advanceWidth)) {
        return nullopt;
      }
      if (!out.i16(m.lsb)) {
        return nullopt;
      }
    }
    for (FWORD const &v : leftSideBearings) {
      if (!out.i16(v)) {
        return nullopt;
      }
    }
    return EncodeResult(out.data());
  }

public:
  std::vector<LongHorMetric> hMetrics;
  std::vector<FWORD> leftSideBearings;
};

} // namespace ksesh::otf
