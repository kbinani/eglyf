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

public:
  std::vector<Path> paths;
};

} // namespace eglyf
