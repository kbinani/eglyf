#pragma once

namespace eglyf {

class Editor {
public:
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
  }

  Optional<uint16_t> defineGlyph(std::string const &name, std::optional<uint32_t> unicode, GlyphDefinitionTable::Class classDef) {
    // DEF_GLYPH "braceclose" ID 383 TYPE MARK END_GLYPH
    // DEF_GLYPH "tcrb" ID 384 UNICODE 10214 TYPE MARK END_GLYPH
    uint16_t glyphId = 0;
    if (unicode) {
      auto gid = font->getGlyphID(*unicode);
      if (gid) {
        glyphId = *gid;
        auto n = font->post->getName(*gid);
        if (!n || *n != name) {
          if (auto st = font->post->setName(*gid, name); !st.ok()) {
            return EGLYF_NULLOPT_PUSH(st);
          }
        }
      }
    } else {
      if (auto gid = font->post->getGlyphId(name); gid) {
        glyphId = *gid;
      } else if (auto gid = font->addEmptyGlyph(name, 0, 0); gid) {
        glyphId = *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    }
    // TODO:
    return glyphId;
  }

  Status run();

public:
  std::shared_ptr<FontFile> font;
};

} // namespace eglyf
