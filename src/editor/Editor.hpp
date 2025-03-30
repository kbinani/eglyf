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
      struct MarkGlyphs {
        std::vector<std::shared_ptr<Glyph>> glyphs;
      };
      struct MarkGroup {
        std::shared_ptr<Group> group;
      };

      ProcessMarks() {}
      explicit ProcessMarks(std::variant<ProcessMarks::All, ProcessMarks::MarkGlyphs, ProcessMarks::MarkGroup> what) : what(what) {}

      std::variant<ProcessMarks::All, ProcessMarks::MarkGlyphs, ProcessMarks::MarkGroup> what;
    };
    std::variant<SkipMarks, ProcessMarks> marks;

    struct Context {
      Context(std::initializer_list<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> left, std::initializer_list<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> right) : left(left), right(right) {
      }

      std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> left;
      std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> right;
    };
    std::shared_ptr<Context> exceptContext;
    std::shared_ptr<Context> inContext;

    struct AttachTarget {
      AttachTarget(std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> target, std::shared_ptr<Anchor> const &anchor) : target(target), anchor(anchor) {}

      std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> target;
      std::shared_ptr<Anchor> anchor;
    };
    struct Attach {
      Attach(std::initializer_list<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> input, std::initializer_list<AttachTarget> output) : input(input), output(output) {}

      std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> input;
      std::vector<AttachTarget> output;
    };
    std::shared_ptr<Attach> attach;

    struct AdjustGlyph {
      AdjustGlyph(std::string const &name, std::optional<int16_t> dx, std::optional<int16_t> dy) : name(name), dx(dx), dy(dy) {}

      std::string name;
      std::optional<int16_t> dx;
      std::optional<int16_t> dy;
    };
    struct AdjustSingle {
      explicit AdjustSingle(std::initializer_list<AdjustGlyph> glyphs) : glyphs(glyphs) {}

      std::vector<AdjustGlyph> glyphs;
    };
    std::shared_ptr<AdjustSingle> adjustSingle;

    struct Substitution {
      std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> input;
      std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> output;
    };
    std::vector<std::shared_ptr<Substitution>> substitutions;

    Lookup() {}

    Lookup(std::variant<SkipBase, ProcessBase> base,
           std::variant<SkipMarks, ProcessMarks> marks,
           std::shared_ptr<Context> exceptContext,
           std::shared_ptr<Context> inContext,
           std::shared_ptr<Attach> attach,
           std::shared_ptr<AdjustSingle> adjustSingle,
           std::vector<std::shared_ptr<Substitution>> &substitutions) : base(base), marks(marks), exceptContext(exceptContext), inContext(inContext), attach(attach), adjustSingle(adjustSingle) {
      this->substitutions.swap(substitutions);
    }
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

public:
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::unordered_map<std::string, std::shared_ptr<Lookup>> lookups;
};

} // namespace eglyf
