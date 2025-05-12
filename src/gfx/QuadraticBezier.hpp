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
    QuadraticEquation qe(p0.x - 2 * p1.x + p2.x,
                         -2 * p0.x + 2 * p1.x,
                         p0.x - x);
    int count = qe.roots(t);
    if (count == 0) {
      return 0;
    }
    if (count > 2) {
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

  size_t getTWhenY(double y, std::array<double, 2> &t) const {
    QuadraticEquation qe(p0.y - 2 * p1.y + p2.y,
                         -2 * p0.y + 2 * p1.y,
                         p0.y - y);
    int count = qe.roots(t);
    if (count == 0) {
      return 0;
    }
    if (count > 2) {
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

  QuadraticBezier<T> cut(double t0, double t1) const {
    Vec<T> q0 = get(t0);
    Vec<T> q2 = get(t1);
    Vec<T> q1 = NewVec((1 - t0 - t1 + t0 * t1) * p0.x + (t0 + t1 - 2 * t0 * t1) * p1.x + t0 * t1 * p2.x,
                       (1 - t0 - t1 + t0 * t1) * p0.y + (t0 + t1 - 2 * t0 * t1) * p1.y + t0 * t1 * p2.y);
    return QuadraticBezier<T>(q0, q1, q2);
  }

  Vec<double> normal(double t, Vec<T> center) const {
    double x = -(1 - t) * (p1.y - p0.y) - t * (p2.y - p1.y);
    double y = (1 - t) * (p1.x - p0.x) + t * (p2.x - p1.x);
    double len = hypot(x, y);
    auto p = get(t);
    double dx = p.x - center.x;
    double dy = p.y - center.y;
    double lenp = hypot(dx, dy);
    double cosA = (x * dx + y * dy) / (len * lenp);
    double cosB = (-x * dx - y * dy) / (len * lenp);
    if (cosA > cosB) {
      return Vec<double>(x / len, y / len);
    } else {
      return Vec<double>(-x / len, -y / len);
    }
  }

  double length(double t = 1) const {
    // https://stackoverflow.com/questions/11854907/calculate-the-length-of-a-segment-of-a-quadratic-bezier
    using namespace std;
    double x0 = p0.x;
    double x1 = p1.x;
    double x2 = p2.x;
    double y0 = p0.y;
    double y1 = p1.y;
    double y2 = p2.y;
    double ax = x0 - x1 - x1 + x2;
    double ay = y0 - y1 - y1 + y2;
    double bx = x1 + x1 - x0 - x0;
    double by = y1 + y1 - y0 - y0;
    double A = 4.0 * ((ax * ax) + (ay * ay));
    double B = 4.0 * ((ax * bx) + (ay * by));
    double C = (bx * bx) + (by * by);
    if (fabs(A) <= numeric_limits<double>::epsilon()) {
      return sqrt(C) * t;
    }
    double b = B / (2.0 * A);
    double c = C / A;
    double u = t + b;
    double k = c - (b * b);
    double ret = 0.5 * sqrt(A) * ((u * sqrt((u * u) + k)) - (b * sqrt((b * b) + k)) + (k * log(fabs((u + sqrt((u * u) + k)) / (b + sqrt((b * b) + k))))));
    return ret;
  }

  // Find the parameter t corresponding to the normalized arc-length s.
  // When t := curve.inverseArcLength(s), curve.cut(0, t).length() / curve.length() should be s.
  double inverseArcLength(double s) {
    using namespace std;
    double ax = p0.x - 2 * p1.x + p2.x;
    double ay = p0.y - 2 * p1.y + p2.y;
    double bx = 2 * (p1.x - p0.x);
    double by = 2 * (p1.y - p0.y);
    double A = ax * ax + ay * ay;
    double B = ax * bx + ay * by;
    double C = bx * bx + by * by;
    double len = length(1);
    double t = s;
    double next = t - (length(t) - s * len) / sqrt((A * t + B) * t + C);
    int count = 0;
    while (fabs(next - t) > 1e-12 && count < 100) {
      t = next;
      next = t - (length(t) - s * len) / sqrt((A * t + B) * t + C);
      count++;
    }
    return next;
  }

  bool intersects(Rect<T> const &b) const {
    using namespace std;

    if (b.contains(p0) || b.contains(p2)) {
      return true;
    }

    Rect<T> bb = boundingBox();
    if (!bb.intersects(b)) {
      return false;
    }

    constexpr int n = 9; // 2^k + 1
    constexpr T div = T(1) / n;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    if constexpr (is_same_v<T, float>) {
      float32x4_t xmin = vdupq_n_f32(b.xMin);
      float32x4_t xmax = vdupq_n_f32(b.xMax);
      float32x4_t ymin = vdupq_n_f32(b.yMin);
      float32x4_t ymax = vdupq_n_f32(b.yMax);

      float32x4_t x_a2 = vdupq_n_f32(x.a2);
      float32x4_t x_a1 = vdupq_n_f32(x.a1);
      float32x4_t x_a0 = vdupq_n_f32(x.a0);

      float32x4_t y_a2 = vdupq_n_f32(y.a2);
      float32x4_t y_a1 = vdupq_n_f32(y.a1);
      float32x4_t y_a0 = vdupq_n_f32(y.a0);

      for (int i = 1; i < n; i += 4) {
        float32x4_t t = {
            i * div,
            (i + 1) * div,
            (i + 2) * div,
            (i + 3) * div};

        float32x4_t t2 = vmulq_f32(t, t);

        float32x4_t x_t2 = vmulq_f32(x_a2, t2);
        float32x4_t x_t1 = vmulq_f32(x_a1, t);
        float32x4_t x_coords = vaddq_f32(vaddq_f32(x_t2, x_t1), x_a0);

        float32x4_t y_t2 = vmulq_f32(y_a2, t2);
        float32x4_t y_t1 = vmulq_f32(y_a1, t);
        float32x4_t y_coords = vaddq_f32(vaddq_f32(y_t2, y_t1), y_a0);

        uint32x4_t x_greater = vcgeq_f32(x_coords, xmin);
        uint32x4_t x_less = vcleq_f32(x_coords, xmax);
        uint32x4_t y_greater = vcgeq_f32(y_coords, ymin);
        uint32x4_t y_less = vcleq_f32(y_coords, ymax);

        uint32x4_t x_inside = vandq_u32(x_greater, x_less);
        uint32x4_t y_inside = vandq_u32(y_greater, y_less);
        uint32x4_t inside = vandq_u32(x_inside, y_inside);

        if (inside[0] | inside[1] | inside[2] | inside[3]) {
          return true;
        }
      }

      return false;
    } else if constexpr (is_same_v<T, double>) {
      float64x2_t xmin = vdupq_n_f64(b.xMin);
      float64x2_t xmax = vdupq_n_f64(b.xMax);
      float64x2_t ymin = vdupq_n_f64(b.yMin);
      float64x2_t ymax = vdupq_n_f64(b.yMax);

      float64x2_t x_a2 = vdupq_n_f64(x.a2);
      float64x2_t x_a1 = vdupq_n_f64(x.a1);
      float64x2_t x_a0 = vdupq_n_f64(x.a0);

      float64x2_t y_a2 = vdupq_n_f64(y.a2);
      float64x2_t y_a1 = vdupq_n_f64(y.a1);
      float64x2_t y_a0 = vdupq_n_f64(y.a0);

      for (int i = 1; i < n; i += 2) {
        float64x2_t t = {i * div, (i + 1) * div};

        float64x2_t t2 = vmulq_f64(t, t);

        float64x2_t x_t2 = vmulq_f64(x_a2, t2);
        float64x2_t x_t1 = vmulq_f64(x_a1, t);
        float64x2_t x_coords = vaddq_f64(vaddq_f64(x_t2, x_t1), x_a0);

        float64x2_t y_t2 = vmulq_f64(y_a2, t2);
        float64x2_t y_t1 = vmulq_f64(y_a1, t);
        float64x2_t y_coords = vaddq_f64(vaddq_f64(y_t2, y_t1), y_a0);

        uint64x2_t x_greater = vcgeq_f64(x_coords, xmin);
        uint64x2_t x_less = vcleq_f64(x_coords, xmax);
        uint64x2_t y_greater = vcgeq_f64(y_coords, ymin);
        uint64x2_t y_less = vcleq_f64(y_coords, ymax);

        uint64x2_t x_inside = vandq_u64(x_greater, x_less);
        uint64x2_t y_inside = vandq_u64(y_greater, y_less);
        uint64x2_t inside = vandq_u64(x_inside, y_inside);

        if (inside[0] | inside[1]) {
          return true;
        }
      }

      return false;
    }
#elif defined(_WIN32)
    if constexpr (is_same_v<T, float>) {
      __m128 xmin = _mm_set1_ps(b.xMin);
      __m128 xmax = _mm_set1_ps(b.xMax);
      __m128 ymin = _mm_set1_ps(b.yMin);
      __m128 ymax = _mm_set1_ps(b.yMax);

      __m128 x_a2 = _mm_set1_ps(x.a2);
      __m128 x_a1 = _mm_set1_ps(x.a1);
      __m128 x_a0 = _mm_set1_ps(x.a0);

      __m128 y_a2 = _mm_set1_ps(y.a2);
      __m128 y_a1 = _mm_set1_ps(y.a1);
      __m128 y_a0 = _mm_set1_ps(y.a0);

      for (int i = 1; i < n; i += 4) {
        __m128 t = _mm_set_ps(
            (i + 3) * div,
            (i + 2) * div,
            (i + 1) * div,
            i * div);
        __m128 t2 = _mm_mul_ps(t, t);

        __m128 x_t2 = _mm_mul_ps(x_a2, t2);
        __m128 x_t1 = _mm_mul_ps(x_a1, t);
        __m128 x_coords = _mm_add_ps(_mm_add_ps(x_t2, x_t1), x_a0);

        __m128 y_t2 = _mm_mul_ps(y_a2, t2);
        __m128 y_t1 = _mm_mul_ps(y_a1, t);
        __m128 y_coords = _mm_add_ps(_mm_add_ps(y_t2, y_t1), y_a0);

        __m128 x_greater = _mm_cmpge_ps(x_coords, xmin);
        __m128 x_less = _mm_cmple_ps(x_coords, xmax);
        __m128 y_greater = _mm_cmpge_ps(y_coords, ymin);
        __m128 y_less = _mm_cmple_ps(y_coords, ymax);

        __m128 x_inside = _mm_and_ps(x_greater, x_less);
        __m128 y_inside = _mm_and_ps(y_greater, y_less);
        __m128 inside = _mm_and_ps(x_inside, y_inside);

        if (_mm_movemask_ps(inside) != 0) {
          return true;
        }
      }
    } else if constexpr (is_same_v<T, double>) {
      __m128d xmin = _mm_set1_pd(b.xMin);
      __m128d xmax = _mm_set1_pd(b.xMax);
      __m128d ymin = _mm_set1_pd(b.yMin);
      __m128d ymax = _mm_set1_pd(b.yMax);

      __m128d x_a2 = _mm_set1_pd(x.a2);
      __m128d x_a1 = _mm_set1_pd(x.a1);
      __m128d x_a0 = _mm_set1_pd(x.a0);

      __m128d y_a2 = _mm_set1_pd(y.a2);
      __m128d y_a1 = _mm_set1_pd(y.a1);
      __m128d y_a0 = _mm_set1_pd(y.a0);

      for (int i = 1; i < n; i += 2) {
        __m128d t = _mm_set_pd((i + 1) * div, i * div);
        __m128d t2 = _mm_mul_pd(t, t);

        __m128d x_t2 = _mm_mul_pd(x_a2, t2);
        __m128d x_t1 = _mm_mul_pd(x_a1, t);
        __m128d x_coords = _mm_add_pd(_mm_add_pd(x_t2, x_t1), x_a0);

        __m128d y_t2 = _mm_mul_pd(y_a2, t2);
        __m128d y_t1 = _mm_mul_pd(y_a1, t);
        __m128d y_coords = _mm_add_pd(_mm_add_pd(y_t2, y_t1), y_a0);

        __m128d x_greater = _mm_cmpge_pd(x_coords, xmin);
        __m128d x_less = _mm_cmple_pd(x_coords, xmax);
        __m128d y_greater = _mm_cmpge_pd(y_coords, ymin);
        __m128d y_less = _mm_cmple_pd(y_coords, ymax);

        __m128d x_inside = _mm_and_pd(x_greater, x_less);
        __m128d y_inside = _mm_and_pd(y_greater, y_less);
        __m128d inside = _mm_and_pd(x_inside, y_inside);

        if (_mm_movemask_pd(inside) != 0) {
          return true;
        }
      }
    }
#endif

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

  Rect<T> boundingBox() const {
    using namespace std;
    T xMin = min(p0.x, p2.x);
    T yMin = min(p0.y, p2.y);
    T xMax = max(p0.x, p2.x);
    T yMax = max(p0.y, p2.y);
    if (xMin <= p1.x && p1.x <= xMax && yMin <= p1.y && p1.y <= yMax) {
      return Rect<T>(xMin, yMin, xMax, yMax);
    }
    auto [x0, x1] = x.minmax(0, 1);
    auto [y0, y1] = y.minmax(0, 1);
    return Rect<T>(x0, y0, x1, y1);
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
  Vec<T> p0;
  Vec<T> p1;
  Vec<T> p2;
  QuadraticEquation x;
  QuadraticEquation y;
};

} // namespace eglyf
