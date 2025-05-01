#pragma once

namespace eglyf {

class Shape {
public:
  bool intersects(Rect<double> const &r) const {
    for (size_t i = 0; i < paths.size(); i++) {
      auto const &p = paths[i];
      if (p.intersects(r)) {
        return true;
      }
    }
    return false;
  }

  Shape transformed(Transform<double> const &txm) const {
    Shape s;
    for (auto const &p : paths) {
      s.paths.push_back(p.transformed(txm));
    }
    return s;
  }

  void toSvg(std::ostream &out) const {
    for (auto const &p : paths) {
      p.toSvg(out);
    }
  }

public:
  std::vector<Path> paths;
};

} // namespace eglyf
