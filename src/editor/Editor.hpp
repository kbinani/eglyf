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
      AdjustGlyph(std::shared_ptr<Glyph> const &glyph, std::optional<int16_t> dx, std::optional<int16_t> dy) : glyph(glyph), dx(dx), dy(dy) {}

      std::shared_ptr<Glyph> glyph;
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

  struct Feature {
    std::string name;
    Tag tag;
    std::vector<std::shared_ptr<Lookup>> lookups;

    Feature(std::string const &name, Tag const &tag) : name(name), tag(tag) {}
  };

  struct LangSys {
    std::string name;
    Tag tag;
    std::vector<std::shared_ptr<Feature>> features;

    LangSys(std::string const &name, Tag const &tag) : name(name), tag(tag) {}
  };

  struct Script {
    std::string name;
    Tag tag;
    std::vector<std::shared_ptr<LangSys>> langSysList;

    Script(std::string const &name, Tag const &tag) : name(name), tag(tag) {}
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

  std::shared_ptr<Script> getScriptByName(std::string const &name) {
    using namespace std;
    if (auto found = scripts.find(name); found == scripts.end()) {
      // Script not found, create a new one with empty tag
      auto s = make_shared<Script>(name, FCC("\0\0\0\0"));
      scripts[name] = s;
      return s;
    } else {
      return found->second;
    }
  }

  std::shared_ptr<LangSys> getLangSysByName(std::shared_ptr<Script> script, std::string const &name) {
    using namespace std;
    for (auto &ls : script->langSysList) {
      if (ls->name == name) {
        return ls;
      }
    }
    // LangSys not found, create a new one with empty tag
    auto ls = make_shared<LangSys>(name, FCC("\0\0\0\0"));
    script->langSysList.push_back(ls);
    return ls;
  }

  std::shared_ptr<Feature> getFeatureByName(std::shared_ptr<LangSys> langsys, std::string const &name) {
    using namespace std;
    for (auto &f : langsys->features) {
      if (f->name == name) {
        return f;
      }
    }
    // Feature not found, create a new one with empty tag
    auto f = make_shared<Feature>(name, FCC("\0\0\0\0"));
    langsys->features.push_back(f);
    return f;
  }

  Status compile() {
    using namespace std;

    // Create GlyphPositioningTable and GlyphSubstitutionTable
    auto gpos = make_shared<GlyphPositioningTable>();
    auto gsub = make_shared<GlyphSubstitutionTable>();

    // Set basic properties
    gpos->majorVersion = 1;
    gpos->minorVersion = 0;

    gsub->majorVersion = 1;
    gsub->minorVersion = 0;

    // Convert Editor::Lookup to GlyphPositioningTable and GlyphSubstitutionTable
    for (auto const &[name, lookup] : lookups) {
      if (lookup->adjustSingle) {
        // Convert AdjustSingle to GlyphPositioningTable
        auto subtable = createAdjustSingleSubtable(lookup->adjustSingle);
        if (subtable) {
          auto lookupData = std::make_shared<SubtableCollection<Subtable>::LookupData>();
          lookupData->lookupType = 1; // SingleAdjustment
          lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
          lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
          lookupData->subtables.push_back(subtable);

          auto gposLookup = std::make_shared<SubtableCollection<Subtable>::Lookup>();
          gposLookup->data = lookupData;
          gpos->lookups.push_back(gposLookup);
        }
      }
    }

    // Create default script, langsys, and feature structure if there are lookups
    if (!gpos->lookups.empty()) {
      // Create default feature
      auto feature = make_shared<SubtableCollection<Subtable>::Feature>();
      feature->tag = FCC("kern"); // "kern" for kerning
      auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();

      // Add lookups to feature
      for (auto const &lookup : gpos->lookups) {
        featureData->lookups.push_back(lookup);
      }
      feature->data = featureData;
      gpos->features.push_back(feature);

      // Create default langsys
      auto langsys = make_shared<SubtableCollection<Subtable>::LangSys>();
      langsys->features.push_back(feature);

      // Create default script
      SubtableCollection<Subtable>::Script script;
      script.tag = FCC("DFLT"); // Default script
      script.defaultLangSys = langsys;

      // Add script to GPOS table
      gpos->scripts.push_back(script);
    }

    // Add tables to FontFile
    font->gpos = gpos;
    font->gsub = gsub;

    return Status::Ok();
  }

private:
  // Convert Editor::Lookup base and marks to OpenType lookupFlag
  uint16_t convertLookupFlag(std::variant<Lookup::SkipBase, Lookup::ProcessBase> const &base,
                             std::variant<Lookup::SkipMarks, Lookup::ProcessMarks> const &marks) const {
    uint16_t flag = 0;

    // Process base flag
    if (std::holds_alternative<Lookup::SkipBase>(base)) {
      flag |= 0x0002; // Ignore base glyphs
    }

    // Process marks flag
    if (std::holds_alternative<Lookup::SkipMarks>(marks)) {
      flag |= 0x0008; // Ignore marks
    } else if (std::holds_alternative<Lookup::ProcessMarks>(marks)) {
      auto const &processMarks = std::get<Lookup::ProcessMarks>(marks);

      // Set Use mark filtering set flag for ProcessMarks::MarkGlyphs or ProcessMarks::MarkGroup
      if (std::holds_alternative<Lookup::ProcessMarks::MarkGlyphs>(processMarks.what) ||
          std::holds_alternative<Lookup::ProcessMarks::MarkGroup>(processMarks.what)) {
        flag |= 0x0010; // Use mark filtering set
      }
    }

    return flag;
  }

  // Create SingleAdjustment subtable from Editor::Lookup::AdjustSingle
  std::shared_ptr<Subtable> createAdjustSingleSubtable(std::shared_ptr<Lookup::AdjustSingle> const &adjustSingle) const {
    using namespace std;

    // Create a map of glyph IDs to ValueRecords
    map<uint16_t, gpos::ValueRecord> glyphValueRecords;

    // Process each glyph in AdjustSingle
    for (auto const &adjustGlyph : adjustSingle->glyphs) {
      // Get glyph ID
      if (!adjustGlyph.glyph->id) {
        // Skip if glyph ID is not set
        continue;
      }

      // Create ValueRecord
      gpos::ValueRecord valueRecord;
      if (adjustGlyph.dx) {
        valueRecord.xPlacement = adjustGlyph.dx;
      }
      if (adjustGlyph.dy) {
        valueRecord.yPlacement = adjustGlyph.dy;
      }

      // Add glyph ID and ValueRecord to map
      glyphValueRecords[*adjustGlyph.glyph->id] = valueRecord;
    }

    // Return nullptr if no glyphs found
    if (glyphValueRecords.empty()) {
      return nullptr;
    }

    // Check if all ValueRecords are the same
    bool allSameValueRecord = true;
    auto firstValueRecord = glyphValueRecords.begin()->second;
    for (auto const &[glyphId, valueRecord] : glyphValueRecords) {
      if (valueRecord.xPlacement != firstValueRecord.xPlacement ||
          valueRecord.yPlacement != firstValueRecord.yPlacement) {
        allSameValueRecord = false;
        break;
      }
    }

    // Create Coverage
    vector<uint16_t> glyphIds;
    for (auto const &[glyphId, valueRecord] : glyphValueRecords) {
      glyphIds.push_back(glyphId);
    }
    // Sort glyph IDs in ascending order (OpenType specification requirement)
    sort(glyphIds.begin(), glyphIds.end());

    auto coverage = make_shared<Coverage1>();
    coverage->glyphArray = glyphIds;

    // Create SingleAdjustment
    if (allSameValueRecord) {
      // Use SingleAdjustment1 if all glyphs have the same ValueRecord
      auto subtable = make_shared<gpos::SingleAdjustment1>();
      subtable->coverage = coverage;
      subtable->valueRecord = firstValueRecord;
      return subtable;
    } else {
      // Use SingleAdjustment2 if glyphs have different ValueRecords
      auto subtable = make_shared<gpos::SingleAdjustment2>();
      subtable->coverage = coverage;

      // Determine valueFormat
      uint16_t valueFormat = 0;
      for (auto const &[glyphId, valueRecord] : glyphValueRecords) {
        if (valueRecord.xPlacement) {
          valueFormat |= gpos::ValueRecord::X_PLACEMENT;
        }
        if (valueRecord.yPlacement) {
          valueFormat |= gpos::ValueRecord::Y_PLACEMENT;
        }
      }
      subtable->valueFormat = valueFormat;

      // Create valueRecords
      for (auto glyphId : glyphIds) {
        subtable->valueRecords.push_back(glyphValueRecords[glyphId]);
      }

      return subtable;
    }
  }

  // Determine markFilteringSet index from Editor::Lookup marks
  uint16_t determineMarkFilteringSet(std::variant<Lookup::SkipMarks, Lookup::ProcessMarks> const &marks,
                                     std::shared_ptr<GlyphDefinitionTable> const &gdef) const {
    if (!gdef || !std::holds_alternative<Lookup::ProcessMarks>(marks)) {
      return 0;
    }

    auto const &processMarks = std::get<Lookup::ProcessMarks>(marks);

    if (std::holds_alternative<Lookup::ProcessMarks::MarkGlyphs>(processMarks.what)) {
      auto const &markGlyphs = std::get<Lookup::ProcessMarks::MarkGlyphs>(processMarks.what);
      // TODO: Add MarkGlyphs to GlyphDefinitionTable's MarkGlyphSetsDef table and return its index
      return 0; // Placeholder
    } else if (std::holds_alternative<Lookup::ProcessMarks::MarkGroup>(processMarks.what)) {
      auto const &markGroup = std::get<Lookup::ProcessMarks::MarkGroup>(processMarks.what);
      // TODO: Add MarkGroup to GlyphDefinitionTable's MarkGlyphSetsDef table and return its index
      return 0; // Placeholder
    }

    return 0;
  }

public:
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::unordered_map<std::string, std::shared_ptr<Lookup>> lookups;
  std::unordered_map<std::string, std::shared_ptr<Script>> scripts;
};

} // namespace eglyf
