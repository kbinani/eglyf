#pragma once

namespace eglyf {

struct Group {
  std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> members;
};

} // namespace eglyf
