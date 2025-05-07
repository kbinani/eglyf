#pragma once

namespace eglyf {

template <class T>
class Line {
public:
  Line(T x0, T y0, T x1, T y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

  bool intersects(Line const &o) const {
    auto [t, u] = Intersection(*this, o);
    if (t && u) {
      return 0 <= *t && *t <= 1 && 0 <= *u && *u <= 1;
    } else {
      return false;
    }
  }

  Vec<double> get(double t) const {
    T dx = x1 - x0;
    T dy = y1 - y0;
    double x = x0 + dx * t;
    double y = y0 + dy * t;
    return Vec<double>(x, y);
  }

  static std::pair<std::optional<double>, std::optional<double>> Intersection(Line const &a, Line const &b) {
    using namespace std;
    double ax0 = a.x0;
    double ay0 = a.y0;
    double ax1 = a.x1;
    double ay1 = a.y1;
    double bx0 = b.x0;
    double by0 = b.y0;
    double bx1 = b.x1;
    double by1 = b.y1;
    double D = (ax1 - ax0) * (by1 - by0) - (ay1 - ay0) * (bx1 - bx0);
    if (fabs(D) <= numeric_limits<double>::epsilon()) {
      return make_pair<optional<double>, optional<double>>(nullopt, nullopt);
    }
    double t = ((bx0 - ax0) * (by1 - by0) - (by0 - ay0) * (bx1 - bx0)) / D;
    double u = ((bx0 - ax0) * (ay1 - ay0) - (by0 - ay0) * (ax1 - ax0)) / D;
    return make_pair<optional<double>, optional<double>>(t, u);
  }

  Line<T> transformed(Transform<T> const &txm) const {
    Vec<T> p0(x0, y0);
    Vec<T> p1(x1, y1);
    auto p0_ = p0.transformed(txm);
    auto p1_ = p1.transformed(txm);
    return Line<T>(p0_.x, p0_.y, p1_.x, p1_.y);
  }

  void toSvg(std::ostream &out, std::string const &color) const {
    using namespace std;
    out << format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}" stroke="{}" />)", x0, y0, x1, y1, color) << endl;
  }

  Rect<double> boundingBox() const {
    using namespace std;
    return Rect<double>(min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1));
  }

public:
  T x0;
  T y0;
  T x1;
  T y1;
};

} // namespace eglyf
