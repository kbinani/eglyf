#pragma once

namespace eglyf {

template <class T>
class QuadraticBezier {
public:
  QuadraticBezier(Vec<T> p0, Vec<T> p1, Vec<T> p2) : p0(p0), p1(p1), p2(p2), x(p0.x - 2 * p1.x + p2.x, -2 * p0.x + 2 * p1.x, p0.x), y(p0.y - 2 * p1.y + p2.y, -2 * p0.y + 2 * p1.y, p0.y) {}

  QuadraticBezier rotatedCCW90() const {
    return QuadraticBezier(p0.rotatedCCW90(), p1.rotatedCCW90(), p2.rotatedCCW90());
  }

  QuadraticBezier rotatedCW90() const {
    return QuadraticBezier(p0.rotatedCW90(), p1.rotatedCW90(), p2.rotatedCW90());
  }

  QuadraticBezier translated(T dx, T dy) const {
    return QuadraticBezier(p0.translated(dx, dy), p1.translated(dx, dy), p2.translated(dx, dy));
  }

  void getTWhenX(double x, std::vector<double> &t) const {
    using namespace std;
    QuadraticEquation qe(p0.x - 2 * p1.x + p2.x,
                         -2 * p0.x + 2 * p1.x,
                         p0.x - x);
    qe.roots(t);
    t.erase(ranges::remove_if(t, [](double t) { return t < 0 || 1 < t; }).begin(), t.end());
    ranges::sort(t);
  }

  Vec<T> get(double t) const {
    return Vec<T>(x.get(t), y.get(t));
  }

  std::pair<QuadraticBezier<T>, QuadraticBezier<T>> cut(double t) const {
    using namespace std;
    Vec<T> m = get(t);
    QuadraticBezier<T> c1(p0, NewVec((1 - t) * p0.x + t * p1.x, (1 - t) * p0.y + t * p1.y), m);
    QuadraticBezier<T> c2(m, NewVec((1 - t) * p1.x + t * p2.x, (1 - t) * p1.y + t * p2.y), p2);
    return make_pair(c1, c2);
  }

  bool intersects(Line<T> const &b) const {
    using namespace std;
    double a0x = p0.x;
    double a0y = p0.y;
    double a1x = p1.x;
    double a1y = p1.y;
    double a2x = p2.x;
    double a2y = p2.y;

    double b0x = b.x0;
    double b0y = b.y0;
    double b1x = b.x1;
    double b1y = b.y1;

    double vx = b1x - b0x;
    double vy = b1y - b0y;
    double nx = vy;
    double ny = -vx;

    double num = vx * vx + vy * vy;
    if (num <= numeric_limits<double>::epsilon()) {
      return false;
    }

    double A = nx * (a0x - 2 * a1x + a2x) + ny * (a0y - 2 * a1y + a2y);
    double B = 2 * (nx * (a1x - a0x) + nx * (a1y - a0y));
    double C = nx * (a0x - b0x) + ny * (a0y - b0y);

    double D = B * B - 4 * A * C;
    if (D < 0) {
      return false;
    }
    double t0 = (-B + sqrt(D)) / (2 * A);
    double t1 = (-B - sqrt(D)) / (2 * A);
    if (t1 < t0) {
      swap(t0, t1);
    }
    if (0 <= t0 && t0 <= 1) {
      double t = t0;
      double s = (((1 - t) * (1 - t) * a0x + 2 * t * (1 - t) * a1x + t * t * a2x - b0x) * vx + ((1 - t) * (1 - t) * a0y + 2 * t * (1 - t) * a1y + t * t * a2y - b0y) * vy) / num;
      if (0 <= s && s <= 1) {
        return true;
      }
    }
    if (0 <= t1 && t1 <= 1) {
      double t = t1;
      double s = (((1 - t) * (1 - t) * a0x + 2 * t * (1 - t) * a1x + t * t * a2x - b0x) * vx + ((1 - t) * (1 - t) * a0y + 2 * t * (1 - t) * a1y + t * t * a2y - b0y) * vy) / num;
      if (0 <= s && s <= 1) {
        return true;
      }
    }
    return false;
  }

  Rect<double> boundingBox() const {
    auto [xMin, xMax] = x.minmax(0, 1);
    auto [yMin, yMax] = y.minmax(0, 1);
    return Rect<double>(xMin, yMin, xMax, yMax);
  }

  QuadraticBezier<T> transformed(Transform<T> const &txm) const {
    return QuadraticBezier<T>(p0.transformed(txm),
                              p1.transformed(txm),
                              p2.transformed(txm));
  }

  static std::array<QuadraticBezier<T>, 4> LeftCartouche(Vec<int16_t> center, int16_t height, int16_t width) {
    using namespace std;
    double a = height / 2.0;
    double b = width;
    double c = a * (2 - sqrt(3.0));
    double d = b / sqrt(3.0);
    QuadraticBezier<T> upper1(NewVec(a, 0),
                              NewVec(a, d),
                              NewVec(a / 2, sqrt(3.0) / 2 * b));
    QuadraticBezier<T> upper2(NewVec(a / 2, sqrt(3.0) / 2 * b),
                              NewVec(c, b),
                              NewVec(0, b));
    QuadraticBezier<T> lower1(NewVec(0, b),
                              NewVec(-c, b),
                              NewVec(-a / 2, sqrt(3.0) / 2 * b));
    QuadraticBezier<T> lower2(NewVec(-a / 2, sqrt(3.0) / 2 * b),
                              NewVec(-a, d),
                              NewVec(-a, 0));
    return {
        upper1.rotatedCCW90().translated(center.x, center.y),
        upper2.rotatedCCW90().translated(center.x, center.y),
        lower1.rotatedCCW90().translated(center.x, center.y),
        lower2.rotatedCCW90().translated(center.x, center.y)};
  }

  static bool Intersects(std::variant<Line<T>, QuadraticBezier<T>> a, Rect<T> const &r) {
    using namespace std;
    Line<T> up(r.xMin, r.yMax, r.xMax, r.yMax);
    Line<T> left(r.xMin, r.yMax, r.xMin, r.yMin);
    Line<T> bottom(r.xMin, r.yMin, r.xMax, r.yMin);
    Line<T> right(r.xMax, r.yMax, r.xMax, r.yMin);
    if (holds_alternative<Line<T>>(a)) {
      auto const &lineA = std::get<Line<T>>(a);
      return lineA.intersects(up) || lineA.intersects(left) || lineA.intersects(bottom) || lineA.intersects(right);
    } else if (holds_alternative<QuadraticBezier<T>>(a)) {
      auto const &bezierA = std::get<QuadraticBezier<T>>(a);
      return bezierA.intersects(up) || bezierA.intersects(left) || bezierA.intersects(bottom) || bezierA.intersects(right);
    } else [[unlikely]] {
      return false;
    }
  }

  void toSvg(std::ostream &out) const {
    using namespace std;
    stringstream d;
    d << "M " << p0.x << "," << p0.y;
    d << " Q " << p1.x << "," << p1.y << " " << p2.x << "," << p2.y;
    out << format(R"(<path d="{}" stroke="black" fill="none" />)", d.str()) << endl;
  }

private:
  static Vec<int16_t> NewVec(double x, double y) {
    return Vec<int16_t>((int16_t)round(x), (int16_t)round(y));
  }

public:
  Vec<T> const p0;
  Vec<T> const p1;
  Vec<T> const p2;
  QuadraticEquation const x;
  QuadraticEquation const y;
};

} // namespace eglyf
