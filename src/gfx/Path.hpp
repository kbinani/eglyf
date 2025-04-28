#pragma once

namespace eglyf {

class Path {
public:
  using Element = std::variant<Line<double>, QuadraticBezier<double>>;

  static Status FromPoints(std::vector<Point> const &points, Path &out) {
    using namespace std;
    if (points.empty()) {
      return Status::Ok();
    }
    Point prev = points[0];
    if (prev.control) {
      return EGLYF_ERROR;
    }
    vector<Element> elements;
    for (int i = 1; i < points.size();) {
      Point p = points[i];
      if (p.control) {
        if (i + 1 >= points.size()) {
          return EGLYF_ERROR;
        }
        Point next = points[i + 1];
        if (next.control) {
          return EGLYF_ERROR;
        }
        QuadraticBezier<double> q({prev.x, prev.y}, {p.x, p.y}, {next.x, next.y});
        out.elements.push_back(q);
        prev = next;
        i += 2;
      } else {
        Line<double> l(prev.x, prev.y, p.x, p.y);
        elements.push_back(l);
        prev = p;
        i++;
      }
    }
    out.elements.swap(elements);
    return Status::Ok();
  }

public:
  std::vector<Element> elements;
};

} // namespace eglyf
