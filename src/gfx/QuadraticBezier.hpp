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

  double length() const {
    // https://stackoverflow.com/questions/11854907/calculate-the-length-of-a-segment-of-a-quadratic-bezier
    double x0 = p0.x;
    double x1 = p1.x;
    double x2 = p2.x;
    double y0 = p0.y;
    double y1 = p1.y;
    double y2 = p2.y;
    double A = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    double B = (x1 - x0) * (x0 - 2 * x1 + x2) + (y1 - y0) * (y0 - 2 * y1 + y2);
    double C = (x0 - 2 * x1 + x2) * (x0 - 2 * x1 + x2) + (y0 - 2 * y1 + y2) * (y0 - 2 * y1 + y2);
    return (C + B) / C * sqrt(A + 2 * B + C) - B / C * sqrt(A) + (A * C - B * B) / pow(C, 1.5f) * log((C + B + sqrt(C * (A + 2 * B + C))) / (B + sqrt(C * A)));
  }

  bool intersects(QuadraticBezier const &a, double toleranceLength) const {
    using namespace std;

    QuadraticBezier const &b = *this;
    Rect<double> boundsA = a.boundingBox();
    Rect<double> boundsB = b.boundingBox();
    if (!boundsA.intersects(boundsB)) {
      return false;
    }

    double la = a.length();
    double lb = b.length();
    int na = max(1, (int)ceil(la / toleranceLength));
    int nb = max(1, (int)ceil(lb / toleranceLength));

    for (int ia = 0; ia < na; ia++) {
      double ta0 = ia / (double)na;
      double ta1 = (ia + 1) / (double)na;
      Line sa(a.x.get(ta0), a.y.get(ta0), a.x.get(ta1), a.y.get(ta1));
      for (int ib = 0; ib < nb; ib++) {
        double tb0 = ib / (double)nb;
        double tb1 = (ib + 1) / (double)nb;
        Line sb(b.x.get(tb0), b.y.get(tb0), b.x.get(tb1), b.y.get(tb1));
        if (sa.intersects(sb)) {
          return true;
        }
      }
    }
    return false;
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

  static bool Intersects(std::variant<Line<T>, QuadraticBezier<T>> a, std::variant<Line<T>, QuadraticBezier<T>> b, double toleranceLength) {
    using namespace std;
    if (holds_alternative<Line<T>>(a)) {
      auto const &lineA = std::get<Line<T>>(a);
      if (holds_alternative<Line<T>>(b)) {
        auto const &lineB = std::get<Line<T>>(b);
        return lineA.intersects(lineB);
      } else if (holds_alternative<QuadraticBezier<T>>(b)) {
        auto const &bezierB = std::get<QuadraticBezier<T>>(b);
        return bezierB.intersects(lineA);
      } else [[unlikely]] {
        return false;
      }
    } else if (holds_alternative<QuadraticBezier<T>>(a)) {
      auto const &bezierA = std::get<QuadraticBezier<T>>(a);
      if (holds_alternative<Line<T>>(b)) {
        auto const &lineB = std::get<Line<T>>(b);
        return bezierA.intersects(lineB);
      } else if (holds_alternative<QuadraticBezier<T>>(b)) {
        auto const &bezierB = std::get<QuadraticBezier<T>>(b);
        return bezierA.intersects(bezierB, toleranceLength);
      } else [[unlikely]] {
        return false;
      }
    } else [[unlikely]] {
      return false;
    }
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
