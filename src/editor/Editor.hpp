#pragma once

namespace eglyf {

class Editor {
public:
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
  }

  Optional<uint16_t> defineGlyph(std::string const &name, std::optional<uint32_t> unicode, GlyphDefinitionTable::Class classDef) {
    using namespace std;
    uint16_t glyphId = 0;
    if (auto gid = font->post->getGlyphId(name); gid) {
      glyphId = *gid;
    } else {
      if (auto gid = font->addEmptyGlyph(name, 0, 0); gid) {
        glyphId = *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    }
    if (unicode) {
      if (auto st = font->cmap->map(*unicode, glyphId); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
      }
    }
    if (!font->gdef) {
      font->gdef = make_shared<GlyphDefinitionTable>();
      font->gdef->majorVersion = 1;
      font->gdef->minorVersion = 2;
    }
    if (!font->gdef->glyphClassDef) {
      font->gdef->glyphClassDef = make_shared<ClassDef2>();
    }
    if (auto st = font->gdef->glyphClassDef->add(glyphId, static_cast<uint16_t>(classDef)); !st.ok()) {
      return EGLYF_NULLOPT_PUSH(st);
    }
    // TODO:
    return glyphId;
  }

  Status run() {
    using namespace std;
#include "editor/DEF_GLYPH.hpp"
    return Status::Ok();
  }

public:
  std::shared_ptr<FontFile> font;
};

} // namespace eglyf
