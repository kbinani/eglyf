#pragma once

namespace eglyf {

template <class T>
struct Vec {
  T x;
  T y;

  Vec() : x(T()), y(T()) {}
  Vec(T x, T y) : x(x), y(y) {}

  Vec<T> transformed(Transform<T> const &t) const {
    T x = t.xscale * this->x + t.scale10 * this->y + t.dx;
    T y = t.scale01 * this->x + t.yscale * this->y + t.dy;
    return Vec<T>(x, y);
  }

  Vec<T> rotatedCCW90() const {
    return Vec<T>(-y, x);
  }

  Vec<T> rotatedCW90() const {
    return Vec<T>(y, -x);
  }

  Vec<T> translated(T dx, T dy) const {
    return Vec<T>(x + dx, y + dy);
  }
};

} // namespace eglyf
