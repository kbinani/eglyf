#pragma once

namespace eglyf {

class Shape {
public:
  bool intersects(Shape const &o, double toleranceLength) const {
    for (size_t i = 0; i < paths.size(); i++) {
      auto const &p = paths[i];
      for (size_t j = 0; j < o.paths.size(); j++) {
        auto const &op = o.paths[j];
        if (p.intersects(op, toleranceLength)) {
          return true;
        }
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

public:
  std::vector<Path> paths;
};

} // namespace eglyf
