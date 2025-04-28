#pragma once

namespace eglyf {

class QuadraticBezier {
public:
  QuadraticBezier(Vec<int16_t> p0, Vec<int16_t> p1, Vec<int16_t> p2) : p0(p0), p1(p1), p2(p2), x(p0.x - 2 * p1.x + p2.x, -2 * p0.x + 2 * p1.x, p0.x), y(p0.y - 2 * p1.y + p2.y, -2 * p0.y + 2 * p1.y, p0.y) {}

  QuadraticBezier rotatedCCW90() const {
    return QuadraticBezier(p0.rotatedCCW90(), p1.rotatedCCW90(), p2.rotatedCCW90());
  }

  QuadraticBezier rotatedCW90() const {
    return QuadraticBezier(p0.rotatedCW90(), p1.rotatedCW90(), p2.rotatedCW90());
  }

  QuadraticBezier translated(int16_t dx, int16_t dy) const {
    return QuadraticBezier(p0.translated(dx, dy), p1.translated(dx, dy), p2.translated(dx, dy));
  }

  void getTWhenX(float x, std::vector<float> &t) const {
    using namespace std;
    QuadraticEquation qe(p0.x - 2 * p1.x + p2.x,
                         -2 * p0.x + 2 * p1.x,
                         p0.x - x);
    qe.roots(t);
    t.erase(ranges::remove_if(t, [](float t) { return t < 0 || 1 < t; }).begin(), t.end());
    ranges::sort(t);
  }

  Vec<float> get(float t) const {
    return Vec<float>(x.get(t), y.get(t));
  }

  std::pair<QuadraticBezier, QuadraticBezier> cut(float t) const {
    using namespace std;
    auto m = get(t);
    auto mi = NewVec(m.x, m.y);
    QuadraticBezier c1(p0, NewVec((1 - t) * p0.x + t * p1.x, (1 - t) * p0.y + t * p1.y), mi);
    QuadraticBezier c2(mi, NewVec((1 - t) * p1.x + t * p2.x, (1 - t) * p1.y + t * p2.y), p2);
    return make_pair(c1, c2);
  }

  float length() const {
    // https://stackoverflow.com/questions/11854907/calculate-the-length-of-a-segment-of-a-quadratic-bezier
    float x0 = p0.x;
    float x1 = p1.x;
    float x2 = p2.x;
    float y0 = p0.y;
    float y1 = p1.y;
    float y2 = p2.y;
    float A = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    float B = (x1 - x0) * (x0 - 2 * x1 + x2) + (y1 - y0) * (y0 - 2 * y1 + y2);
    float C = (x0 - 2 * x1 + x2) * (x0 - 2 * x1 + x2) + (y0 - 2 * y1 + y2) * (y0 - 2 * y1 + y2);
    return (C + B) / C * sqrt(A + 2 * B + C) - B / C * sqrt(A) + (A * C - B * B) / pow(C, 1.5f) * log((C + B + sqrt(C * (A + 2 * B + C))) / (B + sqrt(C * A)));
  }

  bool intersects(QuadraticBezier const &a, float toleranceLength) const {
    using namespace std;

    QuadraticBezier const &b = *this;
    Rect<float> boundsA = a.boundingBox();
    Rect<float> boundsB = b.boundingBox();
    if (!boundsA.intersects(boundsB)) {
      return false;
    }

    float la = a.length();
    float lb = b.length();
    int na = max(1, (int)ceil(la / toleranceLength));
    int nb = max(1, (int)ceil(lb / toleranceLength));

    for (int ia = 0; ia < na; ia++) {
      float ta0 = ia / (float)na;
      float ta1 = (ia + 1) / (float)na;
      Line sa(a.x.get(ta0), a.y.get(ta0), a.x.get(ta1), a.y.get(ta1));
      for (int ib = 0; ib < nb; ib++) {
        float tb0 = ib / (float)nb;
        float tb1 = (ib + 1) / (float)nb;
        Line sb(b.x.get(tb0), b.y.get(tb0), b.x.get(tb1), b.y.get(tb1));
        if (sa.intersects(sb)) {
          return true;
        }
      }
    }
    return false;
  }

  Rect<float> boundingBox() const {
    auto [xMin, xMax] = x.minmax(0, 1);
    auto [yMin, yMax] = y.minmax(0, 1);
    return Rect<float>(xMin, yMin, xMax, yMax);
  }

  static std::array<QuadraticBezier, 4> LeftCartouche(Vec<int16_t> center, int16_t height, int16_t width) {
    using namespace std;
    float a = height / 2.0f;
    float b = width;
    float c = a * (2 - sqrt(3.0f));
    float d = b / sqrt(3.0f);
    QuadraticBezier upper1(NewVec(a, 0),
                           NewVec(a, d),
                           NewVec(a / 2, sqrt(3.0f) / 2 * b));
    QuadraticBezier upper2(NewVec(a / 2, sqrt(3.0f) / 2 * b),
                           NewVec(c, b),
                           NewVec(0, b));
    QuadraticBezier lower1(NewVec(0, b),
                           NewVec(-c, b),
                           NewVec(-a / 2, sqrt(3.0f) / 2 * b));
    QuadraticBezier lower2(NewVec(-a / 2, sqrt(3.0f) / 2 * b),
                           NewVec(-a, d),
                           NewVec(-a, 0));
    return {
        upper1.rotatedCCW90().translated(center.x, center.y),
        upper2.rotatedCCW90().translated(center.x, center.y),
        lower1.rotatedCCW90().translated(center.x, center.y),
        lower2.rotatedCCW90().translated(center.x, center.y)};
  }

  static std::array<QuadraticBezier, 4> RightCartouche(Vec<int16_t> center, int16_t height, int16_t width) {
    using namespace std;
    float a = height / 2.0f;
    float b = width;
    float c = a * (2 - sqrt(3.0f));
    float d = b / sqrt(3.0f);
    QuadraticBezier upper1(NewVec(-a, 0),
                           NewVec(-a, d),
                           NewVec(-a / 2, sqrt(3.0f) / 2 * b));
    QuadraticBezier upper2(NewVec(-a / 2, sqrt(3.0f) / 2 * b),
                           NewVec(-c, b),
                           NewVec(0, b));
    QuadraticBezier lower1(NewVec(0, b),
                           NewVec(c, b),
                           NewVec(a / 2, sqrt(3.0f) / 2 * b));
    QuadraticBezier lower2(NewVec(a / 2, sqrt(3.0f) / 2 * b),
                           NewVec(a, d),
                           NewVec(a, 0));
    return {
        upper1.rotatedCW90().translated(center.x, center.y),
        upper2.rotatedCW90().translated(center.x, center.y),
        lower1.rotatedCW90().translated(center.x, center.y),
        lower2.rotatedCW90().translated(center.x, center.y)};
  }

private:
  static Vec<int16_t> NewVec(float x, float y) {
    return Vec<int16_t>((int16_t)round(x), (int16_t)round(y));
  }

public:
  Vec<int16_t> const p0;
  Vec<int16_t> const p1;
  Vec<int16_t> const p2;
  QuadraticEquation const x;
  QuadraticEquation const y;
};

} // namespace eglyf
