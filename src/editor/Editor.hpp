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
    std::string name;
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

  std::shared_ptr<SubtableCollection<Subtable>::Lookup> convertLookup(std::shared_ptr<Lookup> const &lookup) const {
    using namespace std;

    // For Lookups with adjustSingle
    if (lookup->adjustSingle) {
      auto originalSubtable = createAdjustSingleSubtable(lookup->adjustSingle);
      if (originalSubtable) {
        // Wrap the original subtable in an Extension Positioning subtable
        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = 1; // SingleAdjustment
        extensionSubtable->extension = originalSubtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->lookupType = 9; // Extension Positioning
        lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        gposLookup->data = lookupData;
        return gposLookup;
      }
    }

    // For Lookups with attach
    if (lookup->attach) {
      uint16_t originalLookupType = 4; // Default to MarkToBaseAttachment
      auto originalSubtable = createAttachmentSubtable(lookup->attach, originalLookupType);
      if (originalSubtable) {
        // Wrap the original subtable in an Extension Positioning subtable
        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = originalLookupType; // MarkToBaseAttachment or MarkToMarkAttachmentPositioning
        extensionSubtable->extension = originalSubtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->lookupType = 9; // Extension Positioning
        lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        gposLookup->data = lookupData;
        return gposLookup;
      }
    }

    // For other types of Lookups (to be implemented in the future)
    // ...

    // Return nullptr if conversion is not possible
    return nullptr;
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

    // Convert each Lookup and store in a map
    map<shared_ptr<Lookup>, shared_ptr<SubtableCollection<Subtable>::Lookup>> convertedLookups;
    for (auto const &[name, lookup] : lookups) {
      auto converted = convertLookup(lookup);
      convertedLookups[lookup] = converted; // Will be nullptr if conversion is not possible
    }

    // Map for GPOS table features (to avoid duplicates)
    map<Tag, shared_ptr<SubtableCollection<Subtable>::Feature>> featureMap;

    // Build GPOS table from scripts
    for (auto const &[scriptName, script] : scripts) {
      SubtableCollection<Subtable>::Script gposScript;
      gposScript.tag = script->tag;

      // Process each LangSys
      for (auto const &langSys : script->langSysList) {
        auto gposLangSys = make_shared<SubtableCollection<Subtable>::LangSys>();

        // Process each Feature
        for (auto const &feature : langSys->features) {
          // Check if Feature already exists
          shared_ptr<SubtableCollection<Subtable>::Feature> gposFeature;
          if (auto it = featureMap.find(feature->tag); it != featureMap.end()) {
            gposFeature = it->second;
          } else {
            gposFeature = make_shared<SubtableCollection<Subtable>::Feature>();
            gposFeature->tag = feature->tag;
            auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();
            gposFeature->data = featureData;
            featureMap[feature->tag] = gposFeature;
            gpos->features.push_back(gposFeature);
          }

          // Process each Lookup
          bool hasConvertibleLookup = false;
          for (auto const &lookup : feature->lookups) {
            auto converted = convertedLookups[lookup];
            if (converted) {
              gposFeature->data->lookups.push_back(converted);
              hasConvertibleLookup = true;

              // Add to GPOS lookups list if not already added
              if (find(gpos->lookups.begin(), gpos->lookups.end(), converted) == gpos->lookups.end()) {
                gpos->lookups.push_back(converted);
              }
            }
          }

          // Add only Features with convertible Lookups to LangSys
          if (hasConvertibleLookup) {
            gposLangSys->features.push_back(gposFeature);
          }
        }

        // Add LangSys only if it has features
        if (!gposLangSys->features.empty()) {
          if (langSys->name == "dflt") {
            gposScript.defaultLangSys = gposLangSys;
          } else {
            gposScript.langSysTable.push_back(make_pair(langSys->tag, gposLangSys));
          }
        }
      }

      // Add Script only if it has defaultLangSys or langSysTable
      if (gposScript.defaultLangSys || !gposScript.langSysTable.empty()) {
        gpos->scripts.push_back(gposScript);
      }
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

  // Function to collect glyphs from a variant (glyph or group)
  void collectGlyphsFromVariant(std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> const &item,
                                std::vector<uint16_t> &glyphIds,
                                std::map<uint16_t, std::shared_ptr<Glyph>> &glyphMap) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (glyph->id) {
        glyphIds.push_back(*glyph->id);
        glyphMap[*glyph->id] = glyph;
      }
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      collectGlyphsFromGroup(group, glyphIds, glyphMap);
    }
  }

  // Function to recursively collect glyphs from a group
  void collectGlyphsFromGroup(std::shared_ptr<Group> const &group,
                              std::vector<uint16_t> &glyphIds,
                              std::map<uint16_t, std::shared_ptr<Glyph>> &glyphMap) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        if (glyph->id) {
          glyphIds.push_back(*glyph->id);
          glyphMap[*glyph->id] = glyph;
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        collectGlyphsFromGroup(subgroup, glyphIds, glyphMap);
      }
    }
  }

  // Function to convert Editor::Anchor to GPOS::Anchor1
  std::shared_ptr<gpos::Anchor> convertToGposAnchor(std::shared_ptr<Anchor> const &editorAnchor,
                                                    std::shared_ptr<Glyph> const &glyph) const {
    using namespace std;

    auto it = editorAnchor->glyphs.find(glyph);
    if (it == editorAnchor->glyphs.end()) {
      return nullptr;
    }

    auto vec = it->second;
    auto anchor = make_shared<gpos::Anchor1>();
    anchor->xCoordinate = vec.x.value_or(0);
    anchor->yCoordinate = vec.y.value_or(0);

    return anchor;
  }

  // Determine if a glyph is a mark glyph
  bool isMarkGlyph(std::shared_ptr<Glyph> const &glyph) const {
    return glyph->classDef == GlyphDefinitionTable::Class::Mark;
  }

  // Determine if all glyphs in a variant are mark glyphs
  bool areAllMarkGlyphs(std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> const &item) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      return isMarkGlyph(glyph);
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      return areAllMarkGlyphsInGroup(group);
    }

    return false;
  }

  // Determine if all glyphs in a group are mark glyphs
  bool areAllMarkGlyphsInGroup(std::shared_ptr<Group> const &group) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        if (!isMarkGlyph(glyph)) {
          return false;
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        if (!areAllMarkGlyphsInGroup(subgroup)) {
          return false;
        }
      }
    }

    return true;
  }

  // Create attachment subtable (MarkToBase or MarkToMark) from Editor::Lookup::Attach
  std::shared_ptr<Subtable> createAttachmentSubtable(std::shared_ptr<Lookup::Attach> const &attach, uint16_t &lookupType) const {
    using namespace std;

    if (!attach || attach->input.empty() || attach->output.empty()) {
      return nullptr;
    }

    // Check if input glyphs are all mark glyphs
    bool inputIsAllMarks = true;
    for (auto const &item : attach->input) {
      if (!areAllMarkGlyphs(item)) {
        inputIsAllMarks = false;
        break;
      }
    }

    if (!inputIsAllMarks) {
      // Input must be mark glyphs for both MarkToBase and MarkToMark
      return nullptr;
    }

    // Check if output glyphs are all mark glyphs
    bool outputIsAllMarks = true;
    for (auto const &target : attach->output) {
      if (!areAllMarkGlyphs(target.target)) {
        outputIsAllMarks = false;
        break;
      }
    }

    // Determine lookup type based on output glyphs
    if (outputIsAllMarks) {
      lookupType = 6; // MarkToMarkAttachmentPositioning
    } else {
      lookupType = 4; // MarkToBaseAttachment
    }

    // 1. Create mapping from anchor names to mark class IDs
    map<string, uint16_t> anchorNameToClassId;
    uint16_t nextClassId = 0;

    for (auto const &target : attach->output) {
      if (auto anchor = target.anchor) {
        // Get anchor name directly
        string anchorName = anchor->name;

        if (anchorNameToClassId.find(anchorName) == anchorNameToClassId.end()) {
          anchorNameToClassId[anchorName] = nextClassId++;
        }
      }
    }

    // Cannot convert if no mark classes are found
    if (anchorNameToClassId.empty()) {
      return nullptr;
    }

    // 2. Collect mark and base glyph IDs
    vector<uint16_t> markGlyphIds;
    map<uint16_t, shared_ptr<Glyph>> markGlyphMap; // glyphId -> Glyph

    for (auto const &item : attach->input) {
      collectGlyphsFromVariant(item, markGlyphIds, markGlyphMap);
    }

    vector<uint16_t> baseGlyphIds;
    map<uint16_t, shared_ptr<Glyph>> baseGlyphMap;                              // glyphId -> Glyph
    map<uint16_t, vector<pair<uint16_t, shared_ptr<Anchor>>>> baseGlyphAnchors; // glyphId -> [(classId, Anchor)]

    for (auto const &target : attach->output) {
      // Collect glyphs from target
      vector<uint16_t> targetGlyphIds;
      map<uint16_t, shared_ptr<Glyph>> targetGlyphMap;
      collectGlyphsFromVariant(target.target, targetGlyphIds, targetGlyphMap);

      // Assign anchors to each glyph
      for (auto glyphId : targetGlyphIds) {
        baseGlyphIds.push_back(glyphId);
        baseGlyphMap[glyphId] = targetGlyphMap[glyphId];

        // Get class ID from anchor name
        string anchorName = target.anchor->name;
        if (anchorNameToClassId.find(anchorName) != anchorNameToClassId.end()) {
          uint16_t classId = anchorNameToClassId[anchorName];
          baseGlyphAnchors[glyphId].push_back(make_pair(classId, target.anchor));
        }
      }
    }

    // Return nullptr if no glyph IDs are found
    if (markGlyphIds.empty() || baseGlyphIds.empty()) {
      return nullptr;
    }

    // Remove duplicates and sort
    sort(markGlyphIds.begin(), markGlyphIds.end());
    markGlyphIds.erase(unique(markGlyphIds.begin(), markGlyphIds.end()), markGlyphIds.end());

    sort(baseGlyphIds.begin(), baseGlyphIds.end());
    baseGlyphIds.erase(unique(baseGlyphIds.begin(), baseGlyphIds.end()), baseGlyphIds.end());

    // 3. Create Coverage
    auto markCoverage = make_shared<Coverage1>();
    markCoverage->glyphArray = markGlyphIds;

    auto baseCoverage = make_shared<Coverage1>();
    baseCoverage->glyphArray = baseGlyphIds;

    // 4. Create MarkArray
    gpos::MarkArray markArray;

    // Find anchors corresponding to mark glyphs
    for (auto const &[anchorName, classId] : anchorNameToClassId) {
      for (auto const &[name, anchor] : anchors) {
        if (anchor->name == anchorName) {
          for (auto const &[glyph, coords] : anchor->glyphs) {
            if (glyph->id && find(markGlyphIds.begin(), markGlyphIds.end(), *glyph->id) != markGlyphIds.end()) {
              gpos::MarkRecord record;
              record.markClass = classId;
              record.markAnchor = convertToGposAnchor(anchor, glyph);
              markArray.markRecords.push_back(record);
            }
          }
        }
      }
    }

    // 5. Create BaseArray or Mark2Array
    if (lookupType == 4) {
      // MarkToBaseAttachment
      auto subtable = make_shared<gpos::MarkToBaseAttachment>();
      subtable->markCoverage = markCoverage;
      subtable->baseCoverage = baseCoverage;
      subtable->markArray = markArray;

      // Create BaseArray
      gpos::MarkToBaseAttachment::BaseArray baseArray;
      baseArray.markClassCount = nextClassId;

      for (auto glyphId : baseGlyphIds) {
        gpos::MarkToBaseAttachment::BaseRecord record;
        record.baseAnchors.resize(nextClassId, nullptr); // Initialize with nullptr for all mark classes

        // Set anchors corresponding to the glyph
        if (baseGlyphAnchors.find(glyphId) != baseGlyphAnchors.end()) {
          for (auto const &[classId, anchor] : baseGlyphAnchors[glyphId]) {
            auto glyph = baseGlyphMap[glyphId];
            record.baseAnchors[classId] = convertToGposAnchor(anchor, glyph);
          }
        }

        baseArray.baseRecords.push_back(record);
      }

      subtable->baseArray = baseArray;
      return subtable;
    } else {
      // MarkToMarkAttachmentPositioning
      auto subtable = make_shared<gpos::MarkToMarkAttachmentPositioning>();
      subtable->mark1Coverage = markCoverage;
      subtable->mark2Coverage = baseCoverage;
      subtable->mark1Array = markArray;

      // Create Mark2Array
      gpos::MarkToMarkAttachmentPositioning::Mark2Array mark2Array;
      mark2Array.markClassCount = nextClassId;

      for (auto glyphId : baseGlyphIds) {
        gpos::MarkToMarkAttachmentPositioning::Mark2 record;
        record.mark2Anchors.resize(nextClassId, nullptr); // Initialize with nullptr for all mark classes

        // Set anchors corresponding to the glyph
        if (baseGlyphAnchors.find(glyphId) != baseGlyphAnchors.end()) {
          for (auto const &[classId, anchor] : baseGlyphAnchors[glyphId]) {
            auto glyph = baseGlyphMap[glyphId];
            record.mark2Anchors[classId] = convertToGposAnchor(anchor, glyph);
          }
        }

        mark2Array.mark2Records.push_back(record);
      }

      subtable->mark2Array = mark2Array;
      return subtable;
    }
  }

  // Create MarkToBaseAttachment subtable from Editor::Lookup::Attach
  std::shared_ptr<Subtable> createMarkToBaseAttachmentSubtable(std::shared_ptr<Lookup::Attach> const &attach) const {
    uint16_t lookupType = 4; // Default to MarkToBaseAttachment
    return createAttachmentSubtable(attach, lookupType);
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
