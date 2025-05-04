#pragma once

namespace eglyf {

template <class T>
struct Rect {
  T xMin;
  T yMin;
  T xMax;
  T yMax;

  Rect() : xMin(0), yMin(0), xMax(0), yMax(0) {}
  Rect(T xMin, T yMin, T xMax, T yMax) : xMin(xMin), yMin(yMin), xMax(xMax), yMax(yMax) {}

  void updateBound(Vec<T> const &v) {
    xMin = std::min(xMin, v.x);
    xMax = std::max(xMax, v.x);
    yMin = std::min(yMin, v.y);
    yMax = std::max(yMax, v.y);
  }

  void updateBound(T xMin, T yMin, T xMax, T yMax) {
    this->xMin = std::min(this->xMin, xMin);
    this->xMax = std::max(this->xMax, xMax);
    this->yMin = std::min(this->yMin, yMin);
    this->yMax = std::max(this->yMax, yMax);
  }

  bool intersects(Rect<T> const &o) const {
    return !(
        o.xMin > xMax ||
        o.yMin > yMax ||
        xMin > o.xMax ||
        yMin > o.yMax);
  }

  bool contains(Vec<T> const &p) const {
    return xMin <= p.x && p.x <= xMax && yMin <= p.y && p.y <= yMax;
  }

  bool contains(T x, T y) const {
    return xMin <= x && x <= xMax && yMin <= y && y <= yMax;
  }

  Rect<T> transformed(Transform<T> const &txm) const {
    using namespace std;
    Vec<T> p0 = Vec<T>(xMin, yMin).transformed(txm);
    Vec<T> p1 = Vec<T>(xMax, yMax).transformed(txm);
    return Rect(min(p0.x, p1.x), min(p0.y, p1.y), max(p0.x, p1.x), max(p0.y, p1.y));
  }

  T width() const { return xMax - xMin; }
  T height() const { return yMax - yMin; }
};

} // namespace eglyf
