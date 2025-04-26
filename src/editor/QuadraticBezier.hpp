#pragma once

namespace eglyf {

class QuadraticBezier {
public:
  QuadraticBezier(Vec<int16_t> p0, Vec<int16_t> p1, Vec<int16_t> p2) : p0(p0), p1(p1), p2(p2) {}

  QuadraticBezier rotatedCCW90() const {
    return QuadraticBezier(p0.rotatedCCW90(), p1.rotatedCCW90(), p2.rotatedCCW90());
  }

  QuadraticBezier rotatedCW90() const {
    return QuadraticBezier(p0.rotatedCW90(), p1.rotatedCW90(), p2.rotatedCW90());
  }

  QuadraticBezier translated(int16_t dx, int16_t dy) const {
    return QuadraticBezier(p0.translated(dx, dy), p1.translated(dx, dy), p2.translated(dx, dy));
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

private:
  static Vec<int16_t> NewVec(float x, float y) {
    return Vec<int16_t>((int16_t)round(x), (int16_t)round(y));
  }

public:
  Vec<int16_t> p0;
  Vec<int16_t> p1;
  Vec<int16_t> p2;
};

} // namespace eglyf
