#pragma once

namespace eglyf {

struct SizeVariants {
  struct Resize {
    int16_t dx;
    int16_t dy;
    float scale;
    int16_t lsb;
  };

  Resize transform(int xLevel, int yLevel, int16_t hfu, int16_t vfu, int16_t base) const {
    using namespace std;
    auto width = bounds.width();
    auto height = bounds.height();
    float xScale = hfu * xLevel / (float)width;
    float yScale = vfu * yLevel / (float)height;
    Resize ret;
    ret.scale = min({1.0f, xScale, yScale});
    int16_t xMid = (bounds.xMin + bounds.xMax) / 2;
    if (ret.scale < 1) {
      ret.dx = (int16_t)round(-xMid * ret.scale);
      ret.dy = (int16_t)round((base - bounds.yMin) * ret.scale);
      ret.lsb = (int16_t)round((bounds.xMin - xMid) * ret.scale);
    } else {
      ret.dx = -xMid;
      ret.dy = base - bounds.yMin;
      ret.lsb = bounds.xMin - xMid;
    }
    return ret;
  }

  int hGrids;
  int vGrids;
  Rect<int16_t> bounds;
  std::shared_ptr<Glyph> base;
  std::map<WxH, std::shared_ptr<Glyph>> variants;
};

} // namespace eglyf
