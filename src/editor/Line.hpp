#pragma once

namespace eglyf {

class Line {
public:
  Line(float x0, float y0, float x1, float y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

  bool intersects(Line const &o) const {
    using namespace std;
    float ax0 = x0;
    float ay0 = y0;
    float ax1 = x1;
    float ay1 = y1;
    float bx0 = o.x0;
    float by0 = o.y0;
    float bx1 = o.x1;
    float by1 = o.y1;
    float D = (ax1 - ax0) * (by1 - by0) - (ay1 - ay0) * (bx1 - bx0);
    if (fabs(D) <= numeric_limits<float>::epsilon()) {
      return false;
    }
    float t = ((bx0 - ax0) * (by1 - by0) - (by0 - ay0) * (bx1 - bx0)) / D;
    float u = ((bx0 - ax0) * (ay1 - ay0) - (by0 - ay0) * (ax1 - ax0)) / D;
    return 0 <= t && t <= 1 && 0 <= u && u <= 1;
  }

public:
  float x0;
  float y0;
  float x1;
  float y1;
};

} // namespace eglyf
