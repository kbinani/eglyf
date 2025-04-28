#pragma once

namespace eglyf {

struct Point : public Vec<int16_t> {
  Point() : Vec<int16_t>(0, 0), control(false) {}
  Point(int16_t x, int16_t y, bool control = false) : Vec<int16_t>(x, y), control(control) {}

  bool control;
};

} // namespace eglyf
