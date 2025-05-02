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
        Point next;
        if (i + 1 >= points.size()) {
          next = points[0];
        } else {
          next = points[i + 1];
        }
        if (next.control) {
          Point mid((p.x + next.x) / 2, (p.y + next.y) / 2);
          QuadraticBezier<double> q(Vec<double>(prev.x, prev.y), Vec<double>(p.x, p.y), Vec<double>(mid.x, mid.y));
          elements.push_back(q);
          prev = mid;
          i++;
        } else {
          QuadraticBezier<double> q(Vec<double>(prev.x, prev.y), Vec<double>(p.x, p.y), Vec<double>(next.x, next.y));
          elements.push_back(q);
          prev = next;
          i += 2;
        }
      } else {
        Line<double> l(prev.x, prev.y, p.x, p.y);
        elements.push_back(l);
        prev = p;
        i++;
      }
    }
    Point first = points[0];
    if (prev.x != first.x || prev.y != first.y) {
      Line<double> l(prev.x, prev.y, first.x, first.y);
      elements.push_back(l);
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

  void toSvg(std::ostream &out) const {
    using namespace std;
    for (auto const &e : elements) {
      visit([&](auto const &v) { v.toSvg(out); }, e);
    }
  }

  std::optional<Rect<double>> boundingBox() const {
    using namespace std;
    optional<Rect<double>> ret;
    for (auto const &e : elements) {
      visit([&](auto const &v) {
        auto bounds = v.boundingBox();
        if (ret) {
          ret->updateBound(bounds.xMin, bounds.yMin, bounds.xMax, bounds.yMax);
        } else {
          ret = bounds;
        }
      },
            e);
    }
    return ret;
  }

public:
  std::vector<Element> elements;
};

} // namespace eglyf
