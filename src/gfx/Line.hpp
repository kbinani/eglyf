#pragma once

namespace eglyf {

template <class T>
class Line {
public:
  Line(T x0, T y0, T x1, T y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

  bool intersects(Line const &o) const {
    using namespace std;
    double ax0 = x0;
    double ay0 = y0;
    double ax1 = x1;
    double ay1 = y1;
    double bx0 = o.x0;
    double by0 = o.y0;
    double bx1 = o.x1;
    double by1 = o.y1;
    double D = (ax1 - ax0) * (by1 - by0) - (ay1 - ay0) * (bx1 - bx0);
    if (fabs(D) <= numeric_limits<double>::epsilon()) {
      return false;
    }
    double t = ((bx0 - ax0) * (by1 - by0) - (by0 - ay0) * (bx1 - bx0)) / D;
    double u = ((bx0 - ax0) * (ay1 - ay0) - (by0 - ay0) * (ax1 - ax0)) / D;
    return 0 <= t && t <= 1 && 0 <= u && u <= 1;
  }

  Line<T> transformed(Transform<T> const &txm) const {
    Vec<T> p0(x0, y0);
    Vec<T> p1(x1, y1);
    auto p0_ = p0.transformed(txm);
    auto p1_ = p1.transformed(txm);
    return Line<T>(p0_.x, p0_.y, p1_.x, p1_.y);
  }

  void toSvg(std::ostream &out) const {
    using namespace std;
    out << format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}" stroke="black" />)", x0, y0, x1, y1) << endl;
  }

public:
  T x0;
  T y0;
  T x1;
  T y1;
};

} // namespace eglyf
