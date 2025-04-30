#pragma once

namespace eglyf {

struct Glyph {
  std::string name;
  std::optional<uint16_t> id;
  gdef::GlyphDefinitionTable::Class classDef = gdef::GlyphDefinitionTable::Class::Mark;
};

} // namespace eglyf
