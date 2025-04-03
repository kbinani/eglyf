#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
  struct Glyph {
    std::optional<uint16_t> id;
    GlyphDefinitionTable::Class classDef;
  };

  struct Group {
    std::vector<std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>> members;
  };

  using GG = std::variant<std::shared_ptr<Glyph>, std::shared_ptr<Group>>;

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
      Context(std::initializer_list<GG> left, std::initializer_list<GG> right) : left(left), right(right) {
      }

      std::vector<GG> left;
      std::vector<GG> right;
    };
    std::shared_ptr<Context> exceptContext;
    std::shared_ptr<Context> inContext;

    struct AttachTarget {
      AttachTarget(GG target, std::shared_ptr<Anchor> const &anchor) : target(target), anchor(anchor) {}

      GG target;
      std::shared_ptr<Anchor> anchor;
    };
    struct Attach {
      Attach(std::initializer_list<GG> input, std::initializer_list<AttachTarget> output) : input(input), output(output) {}

      std::vector<GG> input;
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
      std::vector<GG> input;
      std::vector<GG> output;
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

  Status convertLookup(std::shared_ptr<Lookup> const &lookup, std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &result) {
    using namespace std;

    if (!lookup->substitutions.empty()) {
      return EGLYF_STATUS_PUSH(convertGsubLookup(lookup, result));
    } else if (lookup->adjustSingle || lookup->attach) {
      shared_ptr<SubtableCollection<Subtable>::Lookup> ret;
      if (auto st = convertGposLookup(lookup, ret); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      result.push_back(ret);
    }

    return Status::Ok();
  }

  Status convertGposLookup(std::shared_ptr<Lookup> const &lookup, std::shared_ptr<SubtableCollection<Subtable>::Lookup> &result) {
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

    return Status::Ok();
  }

  Status convertSingleGsubLookup(std::vector<std::pair<GG, GG>> const &substitutions,
                                 std::shared_ptr<Subtable> &subtable) {
    using namespace std;

    if (substitutions.empty()) {
      return Status::Ok();
    }

    // Map to store the mapping between input glyph IDs and output glyph IDs
    map<uint16_t, uint16_t> glyphMap;

    // Vector to preserve the order of glyph IDs included in the Coverage
    vector<uint16_t> coverageGlyphIds;

    // Process each substitution pair
    for (auto const &[input, output] : substitutions) {
      // Extract glyph IDs from input (preserving order)
      vector<shared_ptr<Glyph>> inputGlyphs;
      collectGlyphVector(input, inputGlyphs);

      // Extract glyph IDs from output (preserving order)
      vector<shared_ptr<Glyph>> outputGlyphs;
      collectGlyphVector(output, outputGlyphs);

      // Error if input is a single glyph and output is a group (this is a multiple substitution)
      if (inputGlyphs.size() == 1 && outputGlyphs.size() > 1) {
        return EGLYF_ERROR_WHAT("Single to multiple substitution is not supported in convertSingleGsubLookup");
      }

      // Check if both input and output are groups, their glyph counts must match
      if (inputGlyphs.size() > 1 && outputGlyphs.size() > 1 && inputGlyphs.size() != outputGlyphs.size()) {
        return EGLYF_ERROR_WHAT("Group to group substitution requires equal number of glyphs");
      }

      // Check if all input/output glyphs have its id
      vector<uint16_t> inputGlyphIds;
      vector<uint16_t> outputGlyphIds;
      bool ok = true;
      for (auto const &g : inputGlyphs) {
        if (!g->id) {
          ok = false;
          break;
        }
        inputGlyphIds.push_back(*g->id);
      }
      if (!ok) {
        continue;
      }
      for (auto const &g : outputGlyphs) {
        if (!g->id) {
          ok = false;
          break;
        }
        outputGlyphIds.push_back(*g->id);
      }
      if (!ok) {
        continue;
      }

      // Create glyph ID mapping
      if (inputGlyphIds.size() == 1 && outputGlyphIds.size() == 1) {
        // Single glyph to single glyph substitution
        uint16_t inputGlyphId = inputGlyphIds[0];
        uint16_t outputGlyphId = outputGlyphIds[0];

        // Add to Coverage if not already processed
        if (glyphMap.find(inputGlyphId) == glyphMap.end()) {
          coverageGlyphIds.push_back(inputGlyphId);
        }

        glyphMap[inputGlyphId] = outputGlyphId;
      } else if (inputGlyphIds.size() > 1 && outputGlyphIds.size() == 1) {
        // Group to single glyph substitution
        uint16_t outputGlyphId = outputGlyphIds[0];

        for (auto inputGlyphId : inputGlyphIds) {
          // Add to Coverage if not already processed
          if (glyphMap.find(inputGlyphId) == glyphMap.end()) {
            coverageGlyphIds.push_back(inputGlyphId);
          }

          glyphMap[inputGlyphId] = outputGlyphId;
        }
      } else if (inputGlyphIds.size() > 1 && outputGlyphIds.size() > 1) {
        // Group to group substitution (when glyph counts match)
        for (size_t i = 0; i < inputGlyphIds.size(); ++i) {
          uint16_t inputGlyphId = inputGlyphIds[i];
          uint16_t outputGlyphId = outputGlyphIds[i];

          // Add to Coverage if not already processed
          if (glyphMap.find(inputGlyphId) == glyphMap.end()) {
            coverageGlyphIds.push_back(inputGlyphId);
          }

          glyphMap[inputGlyphId] = outputGlyphId;
        }
      }
    }

    // Do nothing if the glyph map is empty
    if (glyphMap.empty()) {
      return Status::Ok();
    }

    // Create Coverage
    auto coverage = make_shared<Coverage1>();
    for (auto glyphId : coverageGlyphIds) {
      coverage->glyphArray.insert(glyphId);
    }

    // Check if all substitutions have the same deltaGlyphID
    bool allSameDelta = true;
    int16_t firstDelta = static_cast<int16_t>(glyphMap[coverageGlyphIds[0]]) - static_cast<int16_t>(coverageGlyphIds[0]);

    for (auto glyphId : coverageGlyphIds) {
      int16_t delta = static_cast<int16_t>(glyphMap[glyphId]) - static_cast<int16_t>(glyphId);
      if (delta != firstDelta) {
        allSameDelta = false;
        break;
      }
    }

    // Create Subtable
    if (allSameDelta) {
      // Use SingleFormat1 if all substitutions have the same deltaGlyphID
      auto format1 = make_shared<gsub::SingleFormat1>();
      format1->coverage = coverage;
      format1->deltaGlyphID = firstDelta;
      subtable = format1;
    } else {
      // Use SingleFormat2 if substitutions have different deltaGlyphIDs
      auto format2 = make_shared<gsub::SingleFormat2>();
      format2->coverage = coverage;

      // Create substituteGlyphIDs (in the same order as Coverage)
      for (auto glyphId : coverage->glyphArray) {
        format2->substituteGlyphIDs.push_back(glyphMap[glyphId]);
      }

      subtable = format2;
    }

    return Status::Ok();
  }

  Status convertLigatureGsubLookup(std::vector<std::pair<std::vector<GG>, GG>> const &substitutions,
                                   std::shared_ptr<Subtable> &subtable) {
    using namespace std;

    vector<pair<vector<uint16_t>, uint16_t>> mapping;
    for (auto const &[input, output] : substitutions) {
      if (input.size() < 2) {
        return EGLYF_ERROR;
      }
      shared_ptr<Group> inGroup;
      for (auto const &it : input) {
        if (holds_alternative<shared_ptr<Group>>(it)) {
          auto g = get<shared_ptr<Group>>(it);
          if (inGroup) {
            if (g != inGroup) {
              return EGLYF_ERROR;
            }
          } else {
            inGroup = g;
          }
        }
      }
      vector<shared_ptr<Glyph>> inGroupGlyphs;
      if (inGroup) {
        collectGlyphVectorFromGroup(inGroup, inGroupGlyphs);
      }

      if (holds_alternative<shared_ptr<Group>>(output)) {
        auto outGroup = get<shared_ptr<Group>>(output);
        if (!inGroup) {
          return EGLYF_ERROR;
        }
        vector<shared_ptr<Glyph>> outGroupGlyphs;
        collectGlyphVectorFromGroup(outGroup, outGroupGlyphs);
        if (outGroupGlyphs.size() != inGroupGlyphs.size()) {
          return EGLYF_ERROR;
        }
        for (size_t i = 0; i < inGroupGlyphs.size(); i++) {
          auto const &inGlyph = inGroupGlyphs[i];
          if (!inGlyph->id) {
            continue;
          }
          auto const &outGlyph = outGroupGlyphs[i];
          if (!outGlyph->id) {
            continue;
          }
          vector<uint16_t> in;
          bool ok = true;
          for (auto const &it : input) {
            if (holds_alternative<shared_ptr<Glyph>>(it)) {
              auto glyph = get<shared_ptr<Glyph>>(it);
              if (!glyph->id) {
                ok = false;
                break;
              }
              in.push_back(*glyph->id);
            } else if (holds_alternative<shared_ptr<Group>>(it)) {
              auto group = get<shared_ptr<Group>>(it);
              assert(group == inGroup);
              in.push_back(*inGlyph->id);
            } else {
              return EGLYF_ERROR;
            }
          }
          mapping.push_back(make_pair(in, *outGlyph->id));
        }
      } else {
        auto outGlyph = get<shared_ptr<Glyph>>(output);
        if (!outGlyph->id) {
          continue;
        }
        if (inGroup) {
          for (auto const &inGlyph : inGroupGlyphs) {
            if (!inGlyph->id) {
              continue;
            }
            vector<uint16_t> in;
            bool ok = true;
            for (auto const &it : input) {
              if (holds_alternative<shared_ptr<Glyph>>(it)) {
                auto glyph = get<shared_ptr<Glyph>>(it);
                if (!glyph->id) {
                  ok = false;
                  break;
                }
                in.push_back(*glyph->id);
              } else if (holds_alternative<shared_ptr<Group>>(it)) {
                auto group = get<shared_ptr<Group>>(it);
                assert(group == inGroup);
                in.push_back(*inGlyph->id);
              } else {
                return EGLYF_ERROR;
              }
            }
            if (ok) {
              mapping.push_back(make_pair(in, *outGlyph->id));
            }
          }
        } else {
          vector<uint16_t> in;
          bool ok = true;
          for (auto const &it : input) {
            if (holds_alternative<shared_ptr<Glyph>>(it)) {
              auto glyph = get<shared_ptr<Glyph>>(it);
              if (!glyph->id) {
                ok = false;
                break;
              }
              in.push_back(*glyph->id);
            } else {
              return EGLYF_ERROR;
            }
          }
          if (ok) {
            mapping.push_back(make_pair(in, *outGlyph->id));
          }
        }
      }
    }

    // Return if mapping is empty
    if (mapping.empty()) {
      return Status::Ok();
    }

    // Group mappings by first glyph ID (for Coverage table)
    map<uint16_t, vector<pair<vector<uint16_t>, uint16_t>>> groupedMapping;
    for (auto const &[components, ligatureGlyph] : mapping) {
      if (components.empty()) {
        continue; // Skip empty input sequences
      }
      uint16_t firstGlyphId = components[0];
      groupedMapping[firstGlyphId].push_back(make_pair(components, ligatureGlyph));
    }

    // Create Coverage table with first glyph IDs
    auto coverage = make_shared<Coverage1>();
    for (auto const &[firstGlyphId, _] : groupedMapping) {
      coverage->glyphArray.insert(firstGlyphId);
    }

    // Create Ligature object and set coverage
    auto ligature = make_shared<gsub::Ligature>();
    ligature->coverage = coverage;

    // Create LigatureSet for each first glyph ID
    for (auto const &[firstGlyphId, mappings] : groupedMapping) {
      gsub::Ligature::LigatureSet ligatureSet;

      // Create LigatureTable for each mapping
      for (auto const &[components, ligatureGlyph] : mappings) {
        gsub::Ligature::LigatureTable ligatureTable;
        ligatureTable.ligatureGlyph = ligatureGlyph;

        // Add component glyphs (excluding the first one which is in Coverage)
        for (size_t i = 1; i < components.size(); i++) {
          ligatureTable.componentGlyphIDs.push_back(components[i]);
        }

        ligatureSet.ligatures.push_back(ligatureTable);
      }

      ligature->ligatureSets.push_back(ligatureSet);
    }

    subtable = ligature;
    return Status::Ok();
  }

  Status convertGsubLookup(std::shared_ptr<Lookup> const &lookup, std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &result) {
    using namespace std;

    if (lookup->substitutions.empty()) {
      return Status::Ok();
    }

    vector<pair<GG, GG>> single;
    vector<pair<vector<GG>, GG>> ligature;

    for (auto const &subst : lookup->substitutions) {
      if (subst->input.size() == 1 && subst->output.size()) {
        single.push_back(make_pair(subst->input[0], subst->output[0]));
      } else if (subst->input.size() == 1 && subst->output.size() > 1) {
        return EGLYF_ERROR_WHAT("Unsupported substitution (1 -> N multiple substitution)");
      } else if (subst->input.size() > 1 && subst->output.size() == 1) {
        ligature.push_back(make_pair(subst->input, subst->output[0]));
      } else {
        return EGLYF_ERROR;
      }
    }

    std::shared_ptr<SubtableCollection<Subtable>::Lookup> singleLookup;
    std::shared_ptr<SubtableCollection<Subtable>::Lookup> ligatureLookup;

    if (!single.empty()) {
      shared_ptr<Subtable> subtable;
      if (auto st = convertSingleGsubLookup(single, subtable); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (subtable) {
        auto extensionSubtable = make_shared<gsub::SubstitutionExtension>();
        extensionSubtable->extensionLookupType = 1; // Single
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->lookupType = 7; // Extension Substitution
        lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        singleLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        singleLookup->data = lookupData;
      }
    }

    if (!ligature.empty()) {
      shared_ptr<Subtable> subtable;
      if (auto st = convertLigatureGsubLookup(ligature, subtable); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (subtable) {
        auto extensionSubtable = make_shared<gsub::SubstitutionExtension>();
        extensionSubtable->extensionLookupType = 4; // Ligature
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->lookupType = 7; // Extension Substitution
        lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        ligatureLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        ligatureLookup->data = lookupData;
      }
    }

    if (!singleLookup && !ligatureLookup) {
      return Status::Ok();
    }

    if (!lookup->inContext && !lookup->exceptContext) {
      if (singleLookup) {
        result.push_back(singleLookup);
      }
      if (ligatureLookup) {
        result.push_back(ligatureLookup);
      }
      return Status::Ok();
    }

    if (lookup->inContext && lookup->exceptContext) {
      // Create a single lookup with two extension subtables
      auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
      lookupData->lookupType = 7; // Extension Substitution
      lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
      lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);

      // First rule: do nothing if the exceptContext matches
      auto chainedContexts1 = make_shared<ChainedContexts3>();

      // Set up input coverage
      set<uint16_t> inputGlyphIds;
      for (auto const &subst : lookup->substitutions) {
        for (auto const &input : subst->input) {
          collectGIDSet(input, inputGlyphIds);
        }
      }

      if (inputGlyphIds.empty()) {
        return Status::Ok(); // Do nothing if there are no input glyphs
      }

      auto inputCoverage1 = make_shared<Coverage1>();
      inputCoverage1->glyphArray = inputGlyphIds;
      chainedContexts1->inputCoverage.push_back(inputCoverage1);

      // Set up backtrack coverage for exceptContext (left context)
      for (auto const &item : lookup->exceptContext->left) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts1->backtrackCoverage.push_back(coverage);
        }
      }

      // Set up lookahead coverage for exceptContext (right context)
      for (auto const &item : lookup->exceptContext->right) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts1->lookaheadCoverage.push_back(coverage);
        }
      }

      // Don't add SubstLookupRecord (do nothing)

      // Second rule: apply substitution if inContext matches
      auto chainedContexts2 = make_shared<ChainedContexts3>();

      // Set up input coverage (same as the first rule)
      auto inputCoverage2 = make_shared<Coverage1>();
      inputCoverage2->glyphArray = inputGlyphIds;
      chainedContexts2->inputCoverage.push_back(inputCoverage2);

      // Set up backtrack coverage for inContext (left context)
      set<uint16_t> backtrackGlyphIds;
      for (auto const &item : lookup->inContext->left) {
        collectGIDSet(item, backtrackGlyphIds);
      }

      if (!backtrackGlyphIds.empty()) {
        auto coverage = make_shared<Coverage1>();
        coverage->glyphArray = backtrackGlyphIds;
        chainedContexts2->backtrackCoverage.push_back(coverage);
      }

      // Set up lookahead coverage for inContext (right context)
      set<uint16_t> lookaheadGlyphIds;
      for (auto const &item : lookup->inContext->right) {
        collectGIDSet(item, lookaheadGlyphIds);
      }

      if (!lookaheadGlyphIds.empty()) {
        auto coverage = make_shared<Coverage1>();
        coverage->glyphArray = lookaheadGlyphIds;
        chainedContexts2->lookaheadCoverage.push_back(coverage);
      }

      // Set up reference to substitution rule
      if (singleLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = singleLookup;
        chainedContexts2->seqLookups.push_back(seqLookup);

        result.push_back(singleLookup);
      }
      if (ligatureLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = ligatureLookup;
        chainedContexts2->seqLookups.push_back(seqLookup);

        result.push_back(ligatureLookup);
      }

      // Wrap both rules with extension subtables and add to a single lookup
      auto extensionSubtable1 = make_shared<gsub::SubstitutionExtension>();
      extensionSubtable1->extensionLookupType = 6; // ChainedContexts
      extensionSubtable1->extension = chainedContexts1;

      auto extensionSubtable2 = make_shared<gsub::SubstitutionExtension>();
      extensionSubtable2->extensionLookupType = 6; // ChainedContexts
      extensionSubtable2->extension = chainedContexts2;

      lookupData->subtables.push_back(extensionSubtable1);
      lookupData->subtables.push_back(extensionSubtable2);

      auto gsubLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
      gsubLookup->data = lookupData;

      result.push_back(gsubLookup);

      return Status::Ok();
    } else if (lookup->inContext) {
      auto chainedContexts = make_shared<ChainedContexts3>();

      // Set up input coverage
      set<uint16_t> inputGlyphIds;
      for (auto const &subst : lookup->substitutions) {
        for (auto const &input : subst->input) {
          collectGIDSet(input, inputGlyphIds);
        }
      }

      if (inputGlyphIds.empty()) {
        return Status::Ok(); // Do nothing if there are no input glyphs
      }

      auto inputCoverage = make_shared<Coverage1>();
      inputCoverage->glyphArray = inputGlyphIds;
      chainedContexts->inputCoverage.push_back(inputCoverage);

      // Set up backtrack coverage (left context)
      for (auto const &item : lookup->inContext->left) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts->backtrackCoverage.push_back(coverage);
        }
      }

      // Set up lookahead coverage (right context)
      for (auto const &item : lookup->inContext->right) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts->lookaheadCoverage.push_back(coverage);
        }
      }

      // Set up reference to substitution rule
      if (singleLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = singleLookup;
        chainedContexts->seqLookups.push_back(seqLookup);

        result.push_back(singleLookup);
      }
      if (ligatureLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = ligatureLookup;
        chainedContexts->seqLookups.push_back(seqLookup);

        result.push_back(ligatureLookup);
      }

      // Wrap with extension subtable
      auto extensionSubtable = make_shared<gsub::SubstitutionExtension>();
      extensionSubtable->extensionLookupType = 6; // ChainedContexts
      extensionSubtable->extension = chainedContexts;

      auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
      lookupData->lookupType = 7; // Extension Substitution
      lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
      lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
      lookupData->subtables.push_back(extensionSubtable);

      auto gsubLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
      gsubLookup->data = lookupData;

      result.push_back(gsubLookup);
    } else /* if (lookup->exceptContext)*/ {
      auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
      lookupData->lookupType = 7; // Extension Substitution
      lookupData->lookupFlag = convertLookupFlag(lookup->base, lookup->marks);
      lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);

      // First rule: do nothing if the context matches
      auto chainedContexts1 = make_shared<ChainedContexts3>();

      // Set up input coverage
      set<uint16_t> inputGlyphIds;
      for (auto const &subst : lookup->substitutions) {
        for (auto const &input : subst->input) {
          collectGIDSet(input, inputGlyphIds);
        }
      }

      if (inputGlyphIds.empty()) {
        return Status::Ok(); // Do nothing if there are no input glyphs
      }

      auto inputCoverage1 = make_shared<Coverage1>();
      inputCoverage1->glyphArray = inputGlyphIds;
      chainedContexts1->inputCoverage.push_back(inputCoverage1);

      // Set up backtrack coverage (left context)
      for (auto const &item : lookup->exceptContext->left) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts1->backtrackCoverage.push_back(coverage);
        }
      }

      // Set up lookahead coverage (right context)
      for (auto const &item : lookup->exceptContext->right) {
        set<uint16_t> glyphIds;
        collectGIDSet(item, glyphIds);

        if (!glyphIds.empty()) {
          auto coverage = make_shared<Coverage1>();
          coverage->glyphArray = glyphIds;
          chainedContexts1->lookaheadCoverage.push_back(coverage);
        }
      }

      // Don't add SubstLookupRecord (do nothing)

      // Second rule: apply substitution in all other cases
      auto chainedContexts2 = make_shared<ChainedContexts3>();

      // Set up input coverage (same as the first rule)
      auto inputCoverage2 = make_shared<Coverage1>();
      inputCoverage2->glyphArray = inputGlyphIds;
      chainedContexts2->inputCoverage.push_back(inputCoverage2);

      // Don't specify backtrack and lookahead

      // Set up reference to substitution rule
      if (singleLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = singleLookup;
        chainedContexts2->seqLookups.push_back(seqLookup);

        result.push_back(singleLookup);
      }
      if (ligatureLookup) {
        SequenceLookup seqLookup;
        seqLookup.sequenceIndex = 0; // Replace the first input glyph
        seqLookup.lookup = ligatureLookup;
        chainedContexts2->seqLookups.push_back(seqLookup);

        result.push_back(ligatureLookup);
      }

      // Wrap both rules with extension subtables and add to a single lookup
      auto extensionSubtable1 = make_shared<gsub::SubstitutionExtension>();
      extensionSubtable1->extensionLookupType = 6; // ChainedContexts
      extensionSubtable1->extension = chainedContexts1;

      auto extensionSubtable2 = make_shared<gsub::SubstitutionExtension>();
      extensionSubtable2->extensionLookupType = 6; // ChainedContexts
      extensionSubtable2->extension = chainedContexts2;

      lookupData->subtables.push_back(extensionSubtable1);
      lookupData->subtables.push_back(extensionSubtable2);

      auto gsubLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
      gsubLookup->data = lookupData;

      result.push_back(gsubLookup);
    }

    return Status::Ok();
  }

  Status compile(std::optional<std::string> onlyLookupWithName = std::nullopt) {
    using namespace std;

    if (onlyLookupWithName) {
      if (auto found = lookups.find(*onlyLookupWithName); found == lookups.end()) {
        lookups.clear();
      } else {
        auto lookup = found->second;
        lookups.clear();
        lookups[*onlyLookupWithName] = lookup;
      }
    }

    // Create GlyphPositioningTable and GlyphSubstitutionTable
    auto gpos = make_shared<GlyphPositioningTable>();
    auto gsub = make_shared<GlyphSubstitutionTable>();

    // Set basic properties
    gpos->majorVersion = 1;
    gpos->minorVersion = 0;

    gsub->majorVersion = 1;
    gsub->minorVersion = 0;

    // Convert each Lookup and store in maps for GPOS and GSUB
    map<shared_ptr<Lookup>, vector<shared_ptr<SubtableCollection<Subtable>::Lookup>>> convertedGposLookups;
    map<shared_ptr<Lookup>, vector<shared_ptr<SubtableCollection<Subtable>::Lookup>>> convertedGsubLookups;

    for (auto const &[name, lookup] : lookups) {
      // Determine if this is a GSUB or GPOS lookup
      bool isGsubLookup = !lookup->substitutions.empty();
      bool isGposLookup = lookup->adjustSingle || lookup->attach;

      if (!isGsubLookup && !isGposLookup) {
        continue; // Skip if neither GSUB nor GPOS
      }

      vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> converted;
      if (auto st = convertLookup(lookup, converted); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (converted.empty()) {
        continue; // Skip if conversion failed
      }

      // Store in appropriate map
      if (isGsubLookup) {
        ranges::copy(converted, back_inserter(convertedGsubLookups[lookup]));
      } else {
        ranges::copy(converted, back_inserter(convertedGposLookups[lookup]));
      }
    }

    // Map for GPOS table features (to avoid duplicates)
    map<Tag, shared_ptr<SubtableCollection<Subtable>::Feature>> gposFeatureMap;

    // Map for GSUB table features (to avoid duplicates)
    map<Tag, shared_ptr<SubtableCollection<Subtable>::Feature>> gsubFeatureMap;

    // Build tables from scripts
    for (auto const &[scriptName, script] : scripts) {
      // Create script for GPOS
      SubtableCollection<Subtable>::Script gposScript;
      gposScript.tag = script->tag;

      // Create script for GSUB
      SubtableCollection<Subtable>::Script gsubScript;
      gsubScript.tag = script->tag;

      // Process each LangSys
      for (auto const &langSys : script->langSysList) {
        auto gposLangSys = make_shared<SubtableCollection<Subtable>::LangSys>();
        auto gsubLangSys = make_shared<SubtableCollection<Subtable>::LangSys>();

        // Process each Feature
        for (auto const &feature : langSys->features) {
          // Process GPOS features
          bool hasGposLookup = false;
          shared_ptr<SubtableCollection<Subtable>::Feature> gposFeature;

          if (auto it = gposFeatureMap.find(feature->tag); it != gposFeatureMap.end()) {
            gposFeature = it->second;
          } else {
            gposFeature = make_shared<SubtableCollection<Subtable>::Feature>();
            gposFeature->tag = feature->tag;
            auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();
            gposFeature->data = featureData;
            gposFeatureMap[feature->tag] = gposFeature;
            gpos->features.push_back(gposFeature);
          }

          // Process GSUB features
          bool hasGsubLookup = false;
          shared_ptr<SubtableCollection<Subtable>::Feature> gsubFeature;

          if (auto it = gsubFeatureMap.find(feature->tag); it != gsubFeatureMap.end()) {
            gsubFeature = it->second;
          } else {
            gsubFeature = make_shared<SubtableCollection<Subtable>::Feature>();
            gsubFeature->tag = feature->tag;
            auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();
            gsubFeature->data = featureData;
            gsubFeatureMap[feature->tag] = gsubFeature;
            gsub->features.push_back(gsubFeature);
          }

          // Process each Lookup
          for (auto const &lookup : feature->lookups) {
            // Add to GPOS if it's a GPOS lookup
            if (auto it = convertedGposLookups.find(lookup); it != convertedGposLookups.end()) {
              hasGposLookup = true;

              auto converted = it->second;
              ranges::copy(converted, back_inserter(gposFeature->data->lookups));
              ranges::copy(converted, back_inserter(gpos->lookups));
            }

            // Add to GSUB if it's a GSUB lookup
            if (auto it = convertedGsubLookups.find(lookup); it != convertedGsubLookups.end()) {
              hasGsubLookup = true;

              auto converted = it->second;
              ranges::copy(converted, back_inserter(gsubFeature->data->lookups));
              ranges::copy(converted, back_inserter(gsub->lookups));
            }
          }

          // Add features to LangSys if they have lookups
          if (hasGposLookup) {
            gposLangSys->features.push_back(gposFeature);
          }

          if (hasGsubLookup) {
            for (auto &lookup : gsubFeature->data->lookups) {
              for (auto const &subtable : lookup->data->subtables) {
                auto sub = subtable;
                if (auto extension = dynamic_pointer_cast<gsub::SubstitutionExtension>(subtable); extension) {
                  sub = extension->extension;
                }
                if (auto chained = dynamic_pointer_cast<ChainedContexts>(sub); chained) {
                  chained->updateLookupToLookupListIndex(gsubFeature->data->lookups);
                }
              }
            }
            gsubLangSys->features.push_back(gsubFeature);
          }
        }

        // Add LangSys to Script if it has features
        if (!gposLangSys->features.empty()) {
          if (langSys->name == "dflt") {
            gposScript.defaultLangSys = gposLangSys;
          } else {
            gposScript.langSysTable.push_back(make_pair(langSys->tag, gposLangSys));
          }
        }

        if (!gsubLangSys->features.empty()) {
          if (langSys->name == "dflt") {
            gsubScript.defaultLangSys = gsubLangSys;
          } else {
            gsubScript.langSysTable.push_back(make_pair(langSys->tag, gsubLangSys));
          }
        }
      }

      // Add Script to table if it has LangSys
      if (gposScript.defaultLangSys || !gposScript.langSysTable.empty()) {
        gpos->scripts.push_back(gposScript);
      }

      if (gsubScript.defaultLangSys || !gsubScript.langSysTable.empty()) {
        gsub->scripts.push_back(gsubScript);
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
    collectGIDSetFromGroup(markGroup.group, glyphIds);

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
  void collectGIDSet(GG const &item,
                     std::set<uint16_t> &glyphIds) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (glyph->id) {
        glyphIds.insert(*glyph->id);
      }
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      collectGIDSetFromGroup(group, glyphIds);
    }
  }

  // Function to recursively collect glyphs from a group
  void collectGIDSetFromGroup(std::shared_ptr<Group> const &group,
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
        collectGIDSetFromGroup(subgroup, glyphIds);
      }
    }
  }

  // Function to extract glyph IDs from a variant (glyph or group) while preserving order
  void collectGlyphVector(GG const &item,
                          std::vector<std::shared_ptr<Glyph>> &glyphs) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      glyphs.push_back(glyph);
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      collectGlyphVectorFromGroup(group, glyphs);
    }
  }

  // Function to recursively extract glyph IDs from a group while preserving order
  void collectGlyphVectorFromGroup(std::shared_ptr<Group> const &group,
                                   std::vector<std::shared_ptr<Glyph>> &glyphs) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        glyphs.push_back(glyph);
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        collectGlyphVectorFromGroup(subgroup, glyphs);
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
  void countGlyphType(GG const &item, size_t &base, size_t &mark) const {
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
      collectGIDSet(item, markGlyphIds);
    }

    set<uint16_t> baseGlyphIds;
    map<uint16_t, vector<pair<ClassId, shared_ptr<Anchor>>>> baseGlyphAnchors; // glyphId -> [(classId, Anchor)]

    for (auto const &target : attach->output) {
      // Collect glyphs from target
      set<uint16_t> targetGlyphIds;
      collectGIDSet(target.target, targetGlyphIds);

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
