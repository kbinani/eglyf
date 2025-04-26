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

public:
  float a2;
  float a1;
  float a0;
};

} // namespace eglyf
