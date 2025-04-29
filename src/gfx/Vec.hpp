#pragma once

namespace eglyf {

template <class T>
struct Vec {
  T x;
  T y;

  Vec() : x(T()), y(T()) {}
  Vec(T x, T y) : x(x), y(y) {}

  Vec<T> transform(T xscale, T scale10, T scale01, T yscale, T dx, T dy) const {
    T x = xscale * this->x + scale10 * this->y + dx;
    T y = scale01 * this->x + yscale * this->y + dy;
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
