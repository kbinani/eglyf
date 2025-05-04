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

  size_t getTWhenX(double x, std::array<double, 2> &t) const {
    using namespace std;
    QuadraticEquation qe(p0.x - 2 * p1.x + p2.x,
                         -2 * p0.x + 2 * p1.x,
                         p0.x - x);
    int count = qe.roots(t);
    if (count == 0) {
      return 0;
    }
    for (int i = 0; i < count;) {
      if (t[i] < 0 || 1 < t[i]) {
        for (int j = i; j < count - 1; j++) {
          t[j] = t[j + 1];
        }
        count--;
      } else {
        i++;
      }
    }
    return count;
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

  bool intersects(Rect<T> const &b) const {
    return intersectsH(b.yMin, b.xMin, b.xMax) ||
           intersectsH(b.yMax, b.xMin, b.xMax) ||
           intersectsV(b.xMin, b.yMin, b.yMax) ||
           intersectsV(b.xMax, b.yMin, b.yMax);
  }

  bool intersectsH(T y, T x0, T x1) const {
    using namespace std;
    QuadraticEquation q(p0.y - 2 * p1.y + p2.y, -2 * p0.y + 2 * p1.y, p0.y - y);
    array<double, 2> roots;
    size_t num = q.roots(roots);
    if (num == 0) {
      return false;
    }
    for (size_t i = 0; i < num; i++) {
      double t = roots[i];
      if (t < 0 || 1 < t) {
        continue;
      }
      double v = x.get(t);
      if (x0 <= v && v <= x1) {
        return true;
      }
    }
    return false;
  }

  bool intersectsV(T x, T y0, T y1) const {
    using namespace std;
    QuadraticEquation q(p0.x - 2 * p1.x + p2.x, -2 * p0.x + 2 * p1.x, p0.x - x);
    array<double, 2> roots;
    size_t num = q.roots(roots);
    if (num == 0) {
      return false;
    }
    for (size_t i = 0; i < num; i++) {
      double t = roots[i];
      if (t < 0 || 1 < t) {
        continue;
      }
      double v = y.get(t);
      if (y0 <= v && v <= y1) {
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
      if (r.contains(lineA.x0, lineA.y0) || r.contains(lineA.x1, lineA.y1)) {
        return true;
      }
      return lineA.intersects(up) || lineA.intersects(left) || lineA.intersects(bottom) || lineA.intersects(right);
    } else if (holds_alternative<QuadraticBezier<T>>(a)) {
      auto const &bezierA = std::get<QuadraticBezier<T>>(a);
      if (r.contains(bezierA.p0) || r.contains(bezierA.p2)) {
        return true;
      }
      return bezierA.intersects(r);
    } else [[unlikely]] {
      return false;
    }
  }

  void toSvg(std::ostream &out, std::string const &color) const {
    using namespace std;
    stringstream d;
    d << "M " << p0.x << "," << p0.y;
    d << " Q " << p1.x << "," << p1.y << " " << p2.x << "," << p2.y;
    out << format(R"(<path d="{}" stroke="{}" fill="none" />)", d.str(), color) << endl;
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
