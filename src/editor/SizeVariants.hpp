#pragma once

namespace eglyf {

struct SizeVariants {
  struct Resize {
    int16_t dx;
    int16_t dy;
    float scale;
    int16_t lsb;
  };

  Resize transform(int xLevel, int yLevel, int16_t hfu, int16_t vfu, int16_t base, int16_t lineWidth) const {
    return Transform(bounds, xLevel, yLevel, hfu, vfu, base, lineWidth);
  }

  static Resize Transform(Rect<int16_t> const &bounds, int xLevel, int yLevel, int16_t hfu, int16_t vfu, int16_t base, int16_t lineWidth) {
    using namespace std;
    int16_t margin = lineWidth / 2;
    auto width = bounds.width();
    auto height = bounds.height();
    double xScale = (hfu * xLevel - 2 * margin) / (double)width;
    double yScale = (vfu * yLevel - 2 * margin) / (double)height;
    double scale = min({1.0, xScale, yScale});
    double xMid = (bounds.xMin + bounds.xMax) * 0.5;
    double yMid = (bounds.yMin + bounds.yMax) * 0.5;
    double yBefore = yMid;
    double yAfter = base - margin + vfu * yLevel * 0.5;
    Resize ret;
    ret.scale = scale;
    ret.dx = (int16_t)round(-xMid * scale);
    ret.dy = (int16_t)round((yAfter - yBefore) * scale);
    ret.lsb = (int16_t)round((bounds.xMin - xMid) * scale);
    return ret;
  }

  WxH size;
  Rect<int16_t> bounds;
  std::shared_ptr<Glyph> base;
  std::map<WxH, std::shared_ptr<Glyph>> variants;
};

} // namespace eglyf
