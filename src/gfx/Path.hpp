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
          Point mid((p.x + next.x) / 2, (p.y + next.y) / 2);
          QuadraticBezier<double> q(Vec<double>(prev.x, prev.y), Vec<double>(p.x, p.y), Vec<double>(mid.x, mid.y));
          out.elements.push_back(q);
          prev = mid;
        } else {
          QuadraticBezier<double> q(Vec<double>(prev.x, prev.y), Vec<double>(p.x, p.y), Vec<double>(next.x, next.y));
          out.elements.push_back(q);
          prev = next;
        }
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

  bool intersects(Rect<double> const &r) const {
    for (size_t i = 0; i < elements.size(); i++) {
      auto const &e = elements[i];
      if (QuadraticBezier<double>::Intersects(e, r)) {
        return true;
      }
    }
    return false;
  }

  Path transformed(Transform<double> const &txm) const {
    using namespace std;
    Path p;
    for (auto const &e : elements) {
      if (holds_alternative<Line<double>>(e)) {
        auto const &l = get<Line<double>>(e);
        p.elements.push_back(l.transformed(txm));
      } else if (holds_alternative<QuadraticBezier<double>>(e)) {
        auto const &b = get<QuadraticBezier<double>>(e);
        p.elements.push_back(b.transformed(txm));
      } else [[unlikely]] {
        assert(false);
      }
    }
    return p;
  }

public:
  std::vector<Element> elements;
};

} // namespace eglyf
