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
      struct MarkGroup {
        std::shared_ptr<Group> group;
      };

      ProcessMarks() {}
      explicit ProcessMarks(std::variant<ProcessMarks::All, ProcessMarks::MarkGroup> what) : what(what) {}

      std::variant<ProcessMarks::All, ProcessMarks::MarkGroup> what;
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

  std::shared_ptr<Glyph> getGlyphById(uint16_t gid) {
    using namespace std;
    if (auto found = glyphsLut.find(gid); found != glyphsLut.end()) {
      return found->second;
    }
    if (auto found = ranges::find_if(glyphs, [=](pair<string, shared_ptr<Glyph>> const &it) { return it.second->id == gid; }); found != glyphs.end()) {
      glyphsLut[gid] = found->second;
      return found->second;
    }
    if (auto name = font->post->getName(gid); name) {
      auto g = getGlyphByName(*name);
      glyphsLut[gid] = g;
      return g;
    }
    return nullptr;
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

  Status convertLookup(std::shared_ptr<Lookup> const &lookup, std::shared_ptr<SubtableCollection<Subtable>::Lookup> &result) {
    using namespace std;

    if (lookup->adjustSingle) {
      auto originalSubtable = createAdjustSingleSubtable(lookup->adjustSingle);
      if (originalSubtable) {
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
        result.swap(gposLookup);
        return Status::Ok();
      }
    }

    if (lookup->attach) {
      shared_ptr<Subtable> subtable;
      uint16_t lookupType = 0;
      if (auto st = createAttachmentSubtable(lookup->attach, subtable, lookupType); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (subtable) {
        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = lookupType;
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->lookupType = 9; // Extension Positioning
        lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        gposLookup->data = lookupData;
        result.swap(gposLookup);
        return Status::Ok();
      }
    }

    // TODO:

    return Status::Ok();
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
      shared_ptr<SubtableCollection<Subtable>::Lookup> converted;
      if (auto st = convertLookup(lookup, converted); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
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
    using namespace std;
    uint16_t flag = 0;

    // Process base flag
    if (holds_alternative<Lookup::SkipBase>(base)) {
      flag |= 0x0002; // Ignore base glyphs
    }

    // Process marks flag
    if (holds_alternative<Lookup::SkipMarks>(marks)) {
      flag |= 0x0008; // Ignore marks
    } else if (holds_alternative<Lookup::ProcessMarks>(marks)) {
      auto const &processMarks = get<Lookup::ProcessMarks>(marks);

      // Set Use mark filtering set flag for ProcessMarks::MarkGlyphs or ProcessMarks::MarkGroup
      if (holds_alternative<Lookup::ProcessMarks::MarkGroup>(processMarks.what)) {
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
    set<uint16_t> glyphIds;
    for (auto const &[glyphId, valueRecord] : glyphValueRecords) {
      glyphIds.insert(glyphId);
    }

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
    using namespace std;
    if (!gdef || !holds_alternative<Lookup::ProcessMarks>(marks)) {
      return 0;
    }

    auto const &processMarks = get<Lookup::ProcessMarks>(marks);

    // Process only for MarkGlyphs or MarkGroup
    if (!holds_alternative<Lookup::ProcessMarks::MarkGroup>(processMarks.what)) {
      return 0;
    }
    auto const &markGroup = get<Lookup::ProcessMarks::MarkGroup>(processMarks.what);

    // Create markGlyphSets if it doesn't exist
    if (!gdef->markGlyphSets) {
      gdef->markGlyphSets = MarkGlyphSets();
      // Update minorVersion to 2 if it's 1 or less
      if (gdef->minorVersion <= 1) {
        gdef->minorVersion = 2;
      }
    }

    // Collect glyph IDs
    set<uint16_t> glyphIds;
    collectGlyphsFromGroup(markGroup.group, glyphIds);

    // Return 0 if no glyph IDs
    if (glyphIds.empty()) {
      return 0;
    }

    // Search for existing coverages with the same glyph ID set
    for (size_t i = 0; i < gdef->markGlyphSets->coverages.size(); i++) {
      auto existingCoverage1 = dynamic_pointer_cast<Coverage1>(gdef->markGlyphSets->coverages[i]);
      if (existingCoverage1) {
        // For Coverage1, compare glyphArray
        if (existingCoverage1->glyphArray == glyphIds) {
          // If a Coverage with the same glyph ID set is found, return its index
          return i;
        }
      } else {
        auto existingCoverage2 = dynamic_pointer_cast<Coverage2>(gdef->markGlyphSets->coverages[i]);
        if (existingCoverage2) {
          // For Coverage2, extract glyph ID set from rangeRecords and compare
          set<uint16_t> coverage2GlyphIds;
          for (auto const &rangeRecord : existingCoverage2->rangeRecords) {
            for (uint16_t glyphId = rangeRecord.startGlyphID; glyphId <= rangeRecord.endGlyphID; glyphId++) {
              coverage2GlyphIds.insert(glyphId);
            }
          }

          // Compare glyph ID sets
          if (coverage2GlyphIds == glyphIds) {
            // If a Coverage with the same glyph ID set is found, return its index
            return i;
          }
        }
      }
    }

    // If no Coverage with the same glyph ID set is found, create a new one
    auto coverage = make_shared<Coverage1>();
    coverage->glyphArray = glyphIds;

    // Add to markGlyphSets
    gdef->markGlyphSets->coverages.push_back(coverage);

    // Return the index (0-based)
    return gdef->markGlyphSets->coverages.size() - 1;
  }

  // Function to collect glyphs from a variant (glyph or group)
  void collectGlyphsFromVariant(std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> const &item,
                                std::set<uint16_t> &glyphIds) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (glyph->id) {
        glyphIds.insert(*glyph->id);
      }
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      collectGlyphsFromGroup(group, glyphIds);
    }
  }

  // Function to recursively collect glyphs from a group
  void collectGlyphsFromGroup(std::shared_ptr<Group> const &group,
                              std::set<uint16_t> &glyphIds) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        if (glyph->id) {
          glyphIds.insert(*glyph->id);
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        collectGlyphsFromGroup(subgroup, glyphIds);
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
  void countGlyphType(std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>> const &item, size_t &base, size_t &mark) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (isMarkGlyph(glyph)) {
        mark++;
      } else {
        base++;
      }
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      countGlyphTypeInGroup(group, base, mark);
    }
  }

  // Determine if all glyphs in a group are mark glyphs
  void countGlyphTypeInGroup(std::shared_ptr<Group> const &group, size_t &base, size_t &mark) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        if (isMarkGlyph(glyph)) {
          mark++;
        } else {
          base++;
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        countGlyphTypeInGroup(subgroup, base, mark);
      }
    }
  }

  // Create attachment subtable (MarkToBase or MarkToMark) from Editor::Lookup::Attach
  Status createAttachmentSubtable(std::shared_ptr<Lookup::Attach> const &attach, std::shared_ptr<Subtable> &result, uint16_t &lookupType) {
    using namespace std;
    using ClassId = uint16_t;

    if (!attach || attach->input.empty() || attach->output.empty()) {
      return Status::Ok();
    }

    // Check if input glyphs are all mark glyphs
    size_t inputBase = 0;
    size_t inputMark = 0;
    bool inputIsAllMarks = true;
    for (auto const &item : attach->input) {
      countGlyphType(item, inputBase, inputMark);
    }

    // Check if output glyphs are all mark glyphs
    size_t outputBase = 0;
    size_t outputMark = 0;
    bool outputIsAllMarks = true;
    for (auto const &target : attach->output) {
      countGlyphType(target.target, outputBase, outputMark);
    }

    // Determine lookup type based on output glyphs
    if (inputBase * outputBase != 0) {
      return EGLYF_ERROR;
    }
    if (outputBase != 0) {
      return EGLYF_ERROR;
    }
    if (outputBase == 0) {
      lookupType = 6; // MarkToMarkAttachmentPositioning
    } else {
      lookupType = 4; // MarkToBaseAttachment
    }

    // 1. Create mapping from anchor names to mark class IDs
    map<string, ClassId> anchorNameToClassId;
    ClassId nextClassId = 0;

    for (auto const &target : attach->output) {
      if (auto anchor = target.anchor) {
        // Get anchor name directly
        string anchorName = anchor->name;

        if (anchorNameToClassId.find(anchorName) == anchorNameToClassId.end()) {
          anchorNameToClassId[anchorName] = nextClassId++;
        }
      }
    }

    if (anchorNameToClassId.empty()) {
      return EGLYF_ERROR_WHAT("Cannot convert if no mark classes are found");
    }

    // 2. Collect mark and base glyph IDs
    set<uint16_t> markGlyphIds;

    for (auto const &item : attach->input) {
      collectGlyphsFromVariant(item, markGlyphIds);
    }

    set<uint16_t> baseGlyphIds;
    map<uint16_t, vector<pair<ClassId, shared_ptr<Anchor>>>> baseGlyphAnchors; // glyphId -> [(classId, Anchor)]

    for (auto const &target : attach->output) {
      // Collect glyphs from target
      set<uint16_t> targetGlyphIds;
      collectGlyphsFromVariant(target.target, targetGlyphIds);

      // Assign anchors to each glyph
      for (auto glyphId : targetGlyphIds) {
        baseGlyphIds.insert(glyphId);

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
      return Status::Ok();
    }

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

      auto subtable = make_unique<gpos::MarkToBaseAttachment>();
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
            auto glyph = getGlyphById(glyphId);
            record.baseAnchors[classId] = convertToGposAnchor(anchor, glyph);
          }
        }

        baseArray.baseRecords.push_back(record);
      }

      subtable->baseArray = baseArray;
      result.reset(subtable.release());
      return Status::Ok();
    } else {
      // MarkToMarkAttachmentPositioning
      auto subtable = make_unique<gpos::MarkToMarkAttachmentPositioning>();
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
            auto glyph = getGlyphById(glyphId);
            record.mark2Anchors[classId] = convertToGposAnchor(anchor, glyph);
          }
        }

        mark2Array.mark2Records.push_back(record);
      }

      subtable->mark2Array = mark2Array;
      result.reset(subtable.release());
      return Status::Ok();
    }
  }

public:
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<uint16_t, std::shared_ptr<Glyph>> glyphsLut;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::unordered_map<std::string, std::shared_ptr<Lookup>> lookups;
  std::unordered_map<std::string, std::shared_ptr<Script>> scripts;
};

} // namespace eglyf
