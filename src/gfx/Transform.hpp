#pragma once

namespace eglyf {

template <class T>
class Transform {
public:
  Transform() : xscale(1), scale10(0), scale01(0), yscale(1), dx(0), dy(0) {}
  Transform(T xscale, T scale10, T scale01, T yscale, T dx, T dy) : xscale(xscale), scale10(scale10), scale01(scale01), yscale(yscale), dx(dx), dy(dy) {}

  static Transform<T> CW90() {
    Transform<T> r;
    r.xscale = 0;
    r.scale10 = 1;
    r.scale01 = -1;
    r.yscale = 0;
    r.dx = 0;
    r.dy = 0;
    return r;
  }

  static Transform<T> CW180() {
    Transform<T> r;
    r.xscale = -1;
    r.scale10 = 0;
    r.scale01 = 0;
    r.yscale = -1;
    r.dx = 0;
    r.dy = 0;
    return r;
  }

  static Transform<T> CW270() {
    Transform<T> r;
    r.xscale = 0;
    r.scale10 = -1;
    r.scale01 = 1;
    r.yscale = 0;
    r.dx = 0;
    r.dy = 0;
    return r;
  }

  static Transform<T> Concat(Transform<T> const &left, Transform<T> const &right) {
    T a = left.xscale;
    T b = left.scale10;
    T c = left.scale01;
    T d = left.yscale;
    T e = left.dx;
    T f = left.dy;
    T A = right.xscale;
    T B = right.scale10;
    T C = right.scale01;
    T D = right.yscale;
    T E = right.dx;
    T F = right.dy;
    return Transform<T>(a * A + b * C,
                        a * B + b * D,
                        c * A + d * C,
                        c * B + d * D,
                        a * E + b * F + e,
                        c * E + d * F + f);
  }

  bool isIdentity() const {
    return xscale == 1 && scale10 == 0 && scale01 == 0 && yscale == 1 && dx == 0 && dy == 0;
  }

public:
  T xscale;
  T scale10;
  T scale01;
  T yscale;
  T dx;
  T dy;
};

} // namespace eglyf
