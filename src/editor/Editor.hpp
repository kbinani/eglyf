#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
  struct Group {
    std::vector<std::variant<std::shared_ptr<Group>, uint16_t>> members;
  };

  struct GroupBuilder {
    GroupBuilder(std::shared_ptr<Editor> const &editor, std::shared_ptr<Group> const &group) : editor(editor), group(group) {
    }

    std::shared_ptr<Group> endGroup() {
      return group;
    }

    GroupBuilder &beginEnum() {
      return *this;
    }

    GroupBuilder &endEnum() {
      return *this;
    }

    GroupBuilder &addGroup(std::string const &name) {
      auto e = editor.lock();
      if (e) {
        auto g = e->getGroupByName(name);
        group->members.push_back(g);
      }
      return *this;
    }

    GroupBuilder &addGlyph(std::string const &name) {
      auto e = editor.lock();
      if (e) {
        if (auto glyphId = e->getGlyphByName(name); glyphId) {
          group->members.push_back(*glyphId);
        }
      }
      return *this;
    }

    std::weak_ptr<Editor> editor;
    std::shared_ptr<Group> group;
  };

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
    glyphs[name] = glyphId;
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
    return glyphId;
  }

  GroupBuilder defineGroup(std::string const &name) {
    auto g = getGroupByName(name);
    GroupBuilder builder(shared_from_this(), g);
    return builder;
  }

  std::optional<uint16_t> getGlyphByName(std::string const &name) {
    using namespace std;
    if (auto found = glyphs.find(name); found == glyphs.end()) {
      return nullopt;
    } else {
      return found->second;
    }
  }

  std::shared_ptr<Group> getGroupByName(std::string const &name) {
    using namespace std;
    if (auto found = groups.find(name); found == groups.end()) {
      auto g = make_shared<Group>();
      groups[name] = g;
      return g;
    } else {
      return found->second;
    }
  }

  Status run() {
    using namespace std;
#include "editor/DEF_GLYPH.hpp"
#include "editor/DEF_GROUP.hpp"
    return Status::Ok();
  }

public:
  std::shared_ptr<FontFile> font;
  std::map<std::string, uint16_t> glyphs;
  std::map<std::string, std::shared_ptr<Group>> groups;
};

} // namespace eglyf
