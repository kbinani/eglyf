#pragma once

namespace eglyf {

struct SizeVariants {
  int hGrids;
  int vGrids;
  std::shared_ptr<Glyph> base;
  std::map<WxH, std::shared_ptr<Glyph>> variants;
};

} // namespace eglyf
