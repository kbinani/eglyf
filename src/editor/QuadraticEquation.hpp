#pragma once

namespace eglyf {

class QuadraticEquation {
public:
  QuadraticEquation(float a2, float a1, float a0) : a2(a2), a1(a1), a0(a0) {}

  void roots(std::vector<float> &out) const {
    using namespace std;
    if (fabs(a2) <= numeric_limits<float>::epsilon()) {
      if (fabs(a1) <= numeric_limits<float>::epsilon()) {
        return;
      }
      out.push_back(-a0 / a1);
      return;
    }
    float D = a1 * a1 - 4 * a2 * a0;
    if (D < 0) {
      return;
    } else if (D == 0) {
      out.push_back(-a1 / (2 * a2));
      return;
    }
    float x0 = (-a1 + sqrt(D)) / (2 * a2);
    float x1 = (-a1 - sqrt(D)) / (2 * a2);
    if (fabs(x0 - x1) <= numeric_limits<float>::epsilon()) {
      out.push_back((x0 + x1) / 2);
    } else {
      out.push_back(x0);
      out.push_back(x1);
    }
  }

  float get(float x) const {
    return (a2 * x + a1) * x + a0;
  }

  std::pair<float, float> minmax(float x0, float x1) const {
    using namespace std;
    float y0 = get(x0);
    float y1 = get(x1);
    if (a2 <= numeric_limits<float>::epsilon()) {
      if (a1 <= numeric_limits<float>::epsilon()) {
        return make_pair(a0, a0);
      }
      return make_pair(min(y0, y1), max(y0, y1));
    }
    float x = (-0.5f * a1) / a2;
    if (x0 <= x && x <= x1) {
      float y = get(x);
      return make_pair(min({y0, y, y1}), max({y0, y, y1}));
    } else {
      return make_pair(min(y0, y1), max(y0, y1));
    }
  }

public:
  float a2;
  float a1;
  float a0;
};

} // namespace eglyf
