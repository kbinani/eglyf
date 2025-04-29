#pragma once

namespace eglyf {

template <class T>
class Transform {
public:
  Transform(T xscale, T scale10, T scale01, T yscale, T dx, T dy) : xscale(xscale), scale10(scale10), scale01(scale01), yscale(yscale), dx(dx), dy(dy) {}

public:
  T xscale;
  T scale10;
  T scale01;
  T yscale;
  T dx;
  T dy;
};

} // namespace eglyf
