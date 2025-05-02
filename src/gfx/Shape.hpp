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
    using namespace std;
    auto bb = boundingBox();
    if (bb) {
      out << format(R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="{} {} {} {}">)", bb->xMin, bb->yMin, bb->width(), bb->height()) << endl;
    } else {
      out << R"(<svg xmlns="http://www.w3.org/2000/svg">)" << endl;
    }
    for (auto const &p : paths) {
      p.toSvg(out);
    }
    out << "</svg>";
  }

  std::optional<Rect<double>> boundingBox() const {
    using namespace std;
    optional<Rect<double>> ret;
    for (auto const &p : paths) {
      auto bounds = p.boundingBox();
      if (!bounds) {
        continue;
      }
      if (ret) {
        ret->updateBound(bounds->xMin, bounds->yMin, bounds->xMax, bounds->yMax);
      } else {
        ret = *bounds;
      }
    }
    return ret;
  }

public:
  std::vector<Path> paths;
};

} // namespace eglyf
