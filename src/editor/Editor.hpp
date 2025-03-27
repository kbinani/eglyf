#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
  struct Glyph {
    std::optional<uint16_t> id;
    GlyphDefinitionTable::Class classDef;
  };

  struct Group {
    std::vector<std::variant<std::shared_ptr<Group>, std::shared_ptr<Glyph>>> members;
  };

  struct GroupBuilder {
    GroupBuilder(std::shared_ptr<Editor> const &editor, std::shared_ptr<Group> const &group) : editor(editor), group(group) {
    }

    std::shared_ptr<Group> endGroup() {
      return group;
    }

    GroupBuilder *beginEnum() {
      return this;
    }

    GroupBuilder *endEnum() {
      return this;
    }

    GroupBuilder *addGroup(std::string const &name) {
      auto e = editor.lock();
      if (e) {
        auto g = e->getGroupByName(name);
        group->members.push_back(g);
      }
      return this;
    }

    GroupBuilder *addGlyph(std::string const &name) {
      auto e = editor.lock();
      if (e) {
        auto g = e->getGlyphByName(name);
        group->members.push_back(g);
      }
      return this;
    }

    std::weak_ptr<Editor> editor;
    std::shared_ptr<Group> group;
  };

public:
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
  }

  std::shared_ptr<Glyph> getGlyphByName(std::string const &name) {
    using namespace std;
    if (auto found = glyphs.find(name); found == glyphs.end()) {
      auto g = make_shared<Glyph>();
      glyphs[name] = g;
      return g;
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

private:
  Optional<uint16_t> defineGlyph(std::string const &name, std::optional<uint32_t> unicode, GlyphDefinitionTable::Class classDef) {
    using namespace std;
    auto g = getGlyphByName(name);
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
    g->id = glyphId;
    g->classDef = classDef;
    return glyphId;
  }

  std::shared_ptr<GroupBuilder> defineGroup(std::string const &name) {
    using namespace std;
    auto g = getGroupByName(name);
    return make_shared<GroupBuilder>(shared_from_this(), g);
  }

public:
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
};

} // namespace eglyf
