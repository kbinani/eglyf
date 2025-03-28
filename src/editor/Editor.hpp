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

  struct Anchor {
    std::map<std::shared_ptr<Glyph>, Vec<std::optional<int16_t>>> glyphs;
  };

  struct Lookup {
    struct SkipBase {};
    struct ProcessBase {};
    std::variant<SkipBase, ProcessBase> base;

    struct SkipMarks {};
    struct ProcessMarks {
      struct All {};
      struct Glyphs {
        std::vector<std::shared_ptr<Editor::Glyph>> glyphs;
      };
      struct Group {
        std::shared_ptr<Editor::Group> group;
      };
      std::variant<ProcessMarks::All, ProcessMarks::Glyphs, ProcessMarks::Group> what;
    };
    std::variant<SkipMarks, ProcessMarks> marks;
  };

  struct LookupBuilder : public std::enable_shared_from_this<LookupBuilder> {
    struct ContextBuilder {
      explicit ContextBuilder(std::shared_ptr<LookupBuilder> const &builder) : lookupBuilder(builder) {}

      ContextBuilder *leftGlyph(std::string const &name) {
        return this;
      }

      ContextBuilder *leftGroup(std::string const &name) {
        return this;
      }

      ContextBuilder *rightGlyph(std::string const &name) {
        return this;
      }

      ContextBuilder *rightGroup(std::string const &name) {
        return this;
      }

      std::shared_ptr<LookupBuilder> endContext() {
        return lookupBuilder;
      }

      std::shared_ptr<LookupBuilder> lookupBuilder;
    };

    struct SubstitutionBuilder {
      explicit SubstitutionBuilder(std::shared_ptr<LookupBuilder> const &builder) : lookupBuilder(builder) {}

      SubstitutionBuilder *substitute() {
        return this;
      }

      SubstitutionBuilder *subGlyph(std::string const &name) {
        return this;
      }

      SubstitutionBuilder *subGroup(std::string const &name) {
        return this;
      }

      SubstitutionBuilder *withGlyph(std::string const &name) {
        return this;
      }

      SubstitutionBuilder *withGroup(std::string const &name) {
        return this;
      }

      SubstitutionBuilder *endSub() {
        return this;
      }

      std::shared_ptr<Lookup> endSubstitutionLookup() {
        return lookupBuilder->endLookup();
      }

      std::shared_ptr<LookupBuilder> lookupBuilder;
    };

    struct PositionBuilder {
      explicit PositionBuilder(std::shared_ptr<LookupBuilder> const &builder) : lookupBuilder(builder) {}

      PositionBuilder *attachGroup(std::string const &name) {
        return this;
      }

      PositionBuilder *attachGlyph(std::string const &name) {
        return this;
      }

      PositionBuilder *toGroup(std::string const &groupName, std::string const &anchorName) {
        return this;
      }

      PositionBuilder *toGlyph(std::string const &glyphName, std::string const &anchorName) {
        return this;
      }

      PositionBuilder *endAttach() {
        return this;
      }

      std::shared_ptr<Lookup> endPositionLookup() {
        return lookupBuilder->endLookup();
      }

      PositionBuilder *adjustSingle() {
        return this;
      }

      PositionBuilder *adjustGlyph(std::string const &name, std::optional<int16_t> dx, std::optional<int16_t> dy) {
        return this;
      }

      PositionBuilder *endAdjust() {
        return this;
      }

      std::shared_ptr<LookupBuilder> lookupBuilder;
    };

    LookupBuilder(std::shared_ptr<Editor> const &editor, std::shared_ptr<Lookup> const &lookup) : editor(editor), lookup(lookup) {}

    LookupBuilder *processBase() {
      lookup->base = Lookup::ProcessBase();
      return this;
    }

    LookupBuilder *skipBase() {
      lookup->base = Lookup::SkipBase();
      return this;
    }

    LookupBuilder *processMarksAll() {
      Lookup::ProcessMarks marks;
      marks.what = Lookup::ProcessMarks::All();
      lookup->marks = marks;
      return this;
    }

    LookupBuilder *processMarkGlyphs(std::string const &name) {
      auto e = editor.lock();
      if (!e) {
        return this;
      }
      Lookup::ProcessMarks marks;
      Lookup::ProcessMarks::Glyphs glyphs;
      auto glyph = e->getGlyphByName(name);
      glyphs.glyphs.push_back(glyph);
      marks.what = glyphs;
      lookup->marks = marks;
      return this;
    }

    LookupBuilder *processMarkGroup(std::string const &name) {
      auto e = editor.lock();
      if (!e) {
        return this;
      }
      Lookup::ProcessMarks marks;
      Lookup::ProcessMarks::Group group;
      auto g = e->getGroupByName(name);
      group.group = g;
      marks.what = group;
      lookup->marks = marks;
      return this;
    }

    LookupBuilder *skipMarks() {
      Lookup::SkipMarks marks;
      lookup->marks = marks;
      return this;
    }

    std::shared_ptr<ContextBuilder> exceptContext() {
      using namespace std;
      return make_shared<ContextBuilder>(shared_from_this());
    }

    std::shared_ptr<SubstitutionBuilder> asSubstitution() {
      using namespace std;
      return make_shared<SubstitutionBuilder>(shared_from_this());
    }

    std::shared_ptr<PositionBuilder> asPosition() {
      using namespace std;
      return make_shared<PositionBuilder>(shared_from_this());
    }

    std::shared_ptr<Lookup> endLookup() {
      return lookup;
    }

    std::weak_ptr<Editor> editor;
    std::shared_ptr<Lookup> lookup;
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

  std::shared_ptr<Anchor> getAnchorByName(std::string const &name) {
    using namespace std;
    if (auto found = anchors.find(name); found == anchors.end()) {
      auto a = make_shared<Anchor>();
      anchors[name] = a;
      return a;
    } else {
      return found->second;
    }
  }

  std::shared_ptr<Lookup> getLookupByName(std::string const &name) {
    using namespace std;
    if (auto found = lookups.find(name); found == lookups.end()) {
      auto l = make_shared<Lookup>();
      lookups[name] = l;
      return l;
    } else {
      return found->second;
    }
  }

  Status run() {
    using namespace std;
    // clang-format off
    #include "editor/DEF_GLYPH.hpp"
    #include "editor/DEF_GROUP.hpp"
    #include "editor/DEF_LOOKUP.hpp"
    #include "editor/DEF_ANCHOR.hpp"
    // clang-format on
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

  void defineAnchor(std::string const &name, std::string const &glyph, std::optional<int16_t> dx, std::optional<int16_t> dy) {
    using namespace std;
    auto a = getAnchorByName(name);
    if (auto g = getGlyphByName(glyph); g) {
      a->glyphs[g] = Vec<optional<int16_t>>(dx, dy);
    }
  }

  std::shared_ptr<LookupBuilder> defineLookup(std::string const &name) {
    using namespace std;
    auto l = getLookupByName(name);
    return make_shared<LookupBuilder>(shared_from_this(), l);
  }

public:
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::unordered_map<std::string, std::shared_ptr<Lookup>> lookups;
};

} // namespace eglyf
