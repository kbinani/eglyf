#pragma once

namespace eglyf {

class QuadraticEquation {
public:
  QuadraticEquation(double a2, double a1, double a0) : a2(a2), a1(a1), a0(a0) {}

  void roots(std::vector<double> &out) const {
    using namespace std;
    if (fabs(a2) <= numeric_limits<double>::epsilon()) {
      if (fabs(a1) <= numeric_limits<double>::epsilon()) {
        return;
      }
      out.push_back(-a0 / a1);
      return;
    }
    double D = a1 * a1 - 4 * a2 * a0;
    if (D < 0) {
      return;
    } else if (D == 0) {
      out.push_back(-a1 / (2 * a2));
      return;
    }
    double x0 = (-a1 + sqrt(D)) / (2 * a2);
    double x1 = (-a1 - sqrt(D)) / (2 * a2);
    if (fabs(x0 - x1) <= numeric_limits<double>::epsilon()) {
      out.push_back((x0 + x1) / 2);
    } else {
      out.push_back(x0);
      out.push_back(x1);
    }
  }

  double get(double x) const {
    return (a2 * x + a1) * x + a0;
  }

  std::pair<double, double> minmax(double x0, double x1) const {
    using namespace std;
    double y0 = get(x0);
    double y1 = get(x1);
    if (a2 <= numeric_limits<double>::epsilon()) {
      if (a1 <= numeric_limits<double>::epsilon()) {
        return make_pair(a0, a0);
      }
      return make_pair(min(y0, y1), max(y0, y1));
    }
    double x = (-0.5f * a1) / a2;
    if (x0 <= x && x <= x1) {
      double y = get(x);
      return make_pair(min({y0, y, y1}), max({y0, y, y1}));
    } else {
      return make_pair(min(y0, y1), max(y0, y1));
    }
  }

public:
  double a2;
  double a1;
  double a0;
};

} // namespace eglyf
