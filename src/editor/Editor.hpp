#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
  struct Glyph {
    std::string name;
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
    std::string name;
    struct SkipBase {};
    struct ProcessBase {};
    std::variant<SkipBase, ProcessBase> base;

    struct SkipMarks {};
    struct ProcessMarks {
      struct All {};
      struct MarkGroup {
        std::shared_ptr<Group> group;
      };
      struct MarkFilteringSet {
        std::shared_ptr<Group> group;
      };

      ProcessMarks() {}
      explicit ProcessMarks(std::variant<ProcessMarks::All, ProcessMarks::MarkFilteringSet, ProcessMarks::MarkGroup> what) : what(what) {}

      std::variant<ProcessMarks::All, ProcessMarks::MarkFilteringSet, ProcessMarks::MarkGroup> what;
    };
    std::variant<SkipMarks, ProcessMarks> marks;

    struct Context {
      Context(std::initializer_list<GG> left, std::initializer_list<GG> right) : left(left), right(right) {
      }

      std::vector<GG> left;
      std::vector<GG> right;
    };
    std::vector<std::shared_ptr<Context>> exceptContexts;
    std::vector<std::shared_ptr<Context>> inContexts;

    struct AttachLigand {
      AttachLigand(GG ligand, std::shared_ptr<Anchor> const &anchor) : ligand(ligand), anchor(anchor) {}

      GG ligand;
      std::shared_ptr<Anchor> anchor;
    };
    struct Attach {
      std::vector<GG> receptors;
      std::vector<AttachLigand> ligands;
    };
    std::shared_ptr<Attach> attach;

    struct AdjustGlyph {
      AdjustGlyph(std::shared_ptr<Glyph> const &glyph, std::optional<int16_t> dx, std::optional<int16_t> dy) : glyph(glyph), dx(dx), dy(dy) {}

      std::shared_ptr<Glyph> glyph;
      std::optional<int16_t> dx;
      std::optional<int16_t> dy;
    };
    struct AdjustSingle {
      std::vector<AdjustGlyph> glyphs;
    };
    std::shared_ptr<AdjustSingle> adjustSingle;

    struct Substitution {
      std::vector<GG> input;
      std::vector<GG> output;
    };
    std::vector<std::shared_ptr<Substitution>> substitutions;
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

  struct SizeVariants {
    int hGrids;
    int vGrids;
    std::shared_ptr<Glyph> base;
    std::map<int, std::shared_ptr<Glyph>> variants;
  };

public:
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
  }

  std::shared_ptr<Glyph> getGlyphByName(std::string const &name) {
    using namespace std;
    if (auto found = glyphs.find(name); found == glyphs.end()) {
      auto g = make_shared<Glyph>();
      g->name = name;
      auto gid = font->post->getGlyphId(name);
      if (gid) {
        g->id = *gid;
      }
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
      g->id = gid;
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
    auto found = ranges::find_if(lookups, [&name](auto const &item) { return item.first == name; });
    if (found == lookups.end()) {
      auto l = make_shared<Lookup>();
      l->name = name;
      lookups.push_back(make_pair(name, l));
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

  Status convertLookup(std::shared_ptr<Lookup> const &lookup,
                       std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &result,
                       std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &indirect) {
    using namespace std;

    if (!lookup->substitutions.empty()) {
      return EGLYF_STATUS_PUSH(convertGsubLookup(lookup, result, indirect));
    } else if (lookup->adjustSingle || lookup->attach) {
      if (auto st = convertGposLookup(lookup, result, indirect); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }

    return Status::Ok();
  }

  Status convertGposLookup(std::shared_ptr<Lookup> const &lookup,
                           std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &result,
                           std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &indirect) {
    using namespace std;

    // clang-format off
    vector<
      pair<
        shared_ptr<SubtableCollection<Subtable>::Lookup>,
        map<
          size_t,
          vector<shared_ptr<Coverage>>
        >
      >
    > lookups;
    // clang-format on

    if (lookup->adjustSingle) {
      auto originalSubtable = createAdjustSingleSubtable(lookup->adjustSingle);
      if (originalSubtable) {
        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = 1; // SingleAdjustment
        extensionSubtable->extension = originalSubtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 9; // Extension Positioning
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        gposLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverage;
        set<uint16_t> coverage;
        for (auto const &adjust : lookup->adjustSingle->glyphs) {
          if (adjust.glyph->id) {
            coverage.insert(*adjust.glyph->id);
          }
        }
        inputCoverage[1].push_back(CoverageBuilder::Build(coverage));

        lookups.push_back(make_pair(gposLookup, inputCoverage));
      }
    }
    if (lookup->attach) {
      shared_ptr<Subtable> subtable;
      uint16_t lookupType = 0;
      if (auto st = createAttachmentSubtable(lookup, subtable, lookupType); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (subtable) {
        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = lookupType;
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 9; // Extension Positioning
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        gposLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        set<uint16_t> coverage;
        for (auto const &receptor : lookup->attach->receptors) {
          collectGIDSet(receptor, coverage);
        }
        inputCoverages[1].push_back(CoverageBuilder::Build(coverage));

        lookups.push_back(make_pair(gposLookup, inputCoverages));
      }
    }

    if (lookups.empty()) {
      return Status::Ok();
    }

    auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
    lookupData->name = lookup->name;
    lookupData->lookupType = 9; // Position extension
    auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
    if (!lookupFlag) {
      return EGLYF_STATUS_PUSH(lookupFlag.status());
    }
    lookupData->lookupFlag = *lookupFlag;
    lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);

    if (lookup->inContexts.empty() && lookup->exceptContexts.empty()) {
      for (auto const &[lookup, inputCoverages] : lookups) {
        result.push_back(lookup);
      }
      return Status::Ok();
    } else {
      for (auto const &[lookup, inputCoverages] : lookups) {
        indirect.push_back(lookup);
      }
    }

    addContextConditions<gpos::PositioningExtension, 8>(lookups,
                                                        lookup->inContexts,
                                                        lookup->exceptContexts,
                                                        lookupData->subtables);
    auto gposLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
    gposLookup->data = lookupData;

    result.push_back(gposLookup);
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
    set<uint16_t> coverage;
    for (auto glyphId : coverageGlyphIds) {
      coverage.insert(glyphId);
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
      format1->coverage = CoverageBuilder::Build(coverage);
      format1->deltaGlyphID = firstDelta;
      subtable = format1;
    } else {
      // Use SingleFormat2 if substitutions have different deltaGlyphIDs
      auto format2 = make_shared<gsub::SingleFormat2>();
      format2->coverage = CoverageBuilder::Build(coverage);

      // Create substituteGlyphIDs (in the same order as Coverage)
      for (auto glyphId : coverage) {
        format2->substituteGlyphIDs.push_back(glyphMap[glyphId]);
      }

      subtable = format2;
    }

    return Status::Ok();
  }

  Status convertMultipleGsubLookup(std::vector<std::pair<GG, std::vector<GG>>> const &substitutions,
                                   std::shared_ptr<Subtable> &subtable) {
    using namespace std;

    map<uint16_t, vector<uint16_t>> mapping;

    // Create mapping from input argument substitutions
    for (auto const &[input, outputs] : substitutions) {
      shared_ptr<Group> group;
      if (holds_alternative<shared_ptr<Group>>(input)) {
        auto g = get<shared_ptr<Group>>(input);
        if (group) {
          if (group != g) {
            return EGLYF_ERROR_WHAT("Inconsistent group in input substitution");
          }
        } else {
          group = g;
        }
      }
      for (auto const &output : outputs) {
        if (holds_alternative<shared_ptr<Group>>(output)) {
          auto g = get<shared_ptr<Group>>(output);
          if (group) {
            if (group != g) {
              return EGLYF_ERROR_WHAT("Inconsistent group in output substitution");
            }
          } else {
            return EGLYF_ERROR_WHAT("Output is a group but input is not a group");
          }
        }
      }
      vector<shared_ptr<Glyph>> groupGlyphs;
      if (group) {
        collectGlyphVector(group, groupGlyphs);
      }

      if (holds_alternative<shared_ptr<Group>>(input)) {
        auto inputGroup = get<shared_ptr<Group>>(input);
        if (inputGroup != group) {
          return EGLYF_ERROR_WHAT("Input group does not match the common group");
        }
        for (size_t i = 0; i < groupGlyphs.size(); i++) {
          auto glyph = groupGlyphs[i];
          if (!glyph->id) {
            continue;
          }
          vector<uint16_t> outputGlyphs;
          for (auto const &output : outputs) {
            if (holds_alternative<shared_ptr<Group>>(output)) {
              auto outputGroup = get<shared_ptr<Group>>(output);
              if (outputGroup != group) {
                return EGLYF_ERROR_WHAT("Output group does not match the common group");
              }
              outputGlyphs.push_back(*glyph->id);
            } else if (holds_alternative<shared_ptr<Glyph>>(output)) {
              auto outputGlyph = get<shared_ptr<Glyph>>(output);
              if (!outputGlyph->id) {
                break;
              }
              outputGlyphs.push_back(*outputGlyph->id);
            } else {
              return EGLYF_ERROR_WHAT("Invalid variant type in output");
            }
          }
          if (outputGlyphs.size() == outputs.size()) {
            mapping[*glyph->id] = outputGlyphs;
          }
        }
      } else if (holds_alternative<shared_ptr<Glyph>>(input)) {
        auto inputGlyph = get<shared_ptr<Glyph>>(input);
        if (!inputGlyph->id) {
          continue;
        }
        uint16_t inputGlyphId = *inputGlyph->id;
        vector<uint16_t> outputGlyphIds;
        for (auto const &output : outputs) {
          if (holds_alternative<shared_ptr<Group>>(output)) {
            return EGLYF_ERROR_WHAT("Output contains a group which is not supported for single glyph input");
          }
          auto outputGlyph = get<shared_ptr<Glyph>>(output);
          if (outputGlyph->id) {
            outputGlyphIds.push_back(*outputGlyph->id);
          } else {
            break;
          }
        }
        if (outputGlyphIds.size() == outputs.size()) {
          mapping[inputGlyphId] = outputGlyphIds;
        }
      } else {
        return EGLYF_ERROR_WHAT("Invalid variant type in input");
      }
    }

    // Return if mapping is empty
    if (mapping.empty()) {
      return Status::Ok();
    }

    set<uint16_t> coverageGlyphIds;
    for (auto const &[inputGlyphId, outputGlyphIds] : mapping) {
      coverageGlyphIds.insert(inputGlyphId);
    }

    // Create Sequence objects for each input glyph ID
    vector<gsub::Multiple::Sequence> sequences;

    for (auto glyphId : coverageGlyphIds) {
      gsub::Multiple::Sequence sequence;
      sequence.substituteGlyphIDs = mapping[glyphId];
      sequences.push_back(sequence);
    }

    // Create gsub::Multiple object and set coverage and sequences
    auto multiple = make_shared<gsub::Multiple>();
    multiple->coverage = CoverageBuilder::Build(coverageGlyphIds);
    multiple->sequences = sequences;

    // Set result to subtable argument
    subtable = multiple;

    return Status::Ok();
  }

  Status convertLigatureGsubLookup(std::vector<std::pair<std::vector<GG>, GG>> const &substitutions,
                                   std::shared_ptr<Subtable> &subtable) {
    using namespace std;

    vector<pair<vector<uint16_t>, uint16_t>> mapping;
    for (auto const &[input, output] : substitutions) {
      shared_ptr<Group> inGroup;
      for (auto const &it : input) {
        if (holds_alternative<shared_ptr<Group>>(it)) {
          auto g = get<shared_ptr<Group>>(it);
          if (inGroup) {
            if (g != inGroup) {
              return EGLYF_ERROR_WHAT("Multiple different groups in input are not supported");
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
          return EGLYF_ERROR_WHAT("Group output requires group input");
        }
        vector<shared_ptr<Glyph>> outGroupGlyphs;
        collectGlyphVectorFromGroup(outGroup, outGroupGlyphs);
        if (outGroupGlyphs.size() != inGroupGlyphs.size()) {
          return EGLYF_ERROR_WHAT("Output group size must match input group size");
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
              return EGLYF_ERROR_WHAT("Unsupported variant type in input");
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
                return EGLYF_ERROR_WHAT("Unsupported variant type in input");
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
              return EGLYF_ERROR_WHAT("Unsupported variant type in input");
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
    set<uint16_t> coverage;
    for (auto const &[firstGlyphId, _] : groupedMapping) {
      coverage.insert(firstGlyphId);
    }

    // Create Ligature object and set coverage
    auto ligature = make_shared<gsub::Ligature>();
    ligature->coverage = CoverageBuilder::Build(coverage);

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

  Status convertGsubLookup(std::shared_ptr<Lookup> const &lookup,
                           std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &result,
                           std::vector<std::shared_ptr<SubtableCollection<Subtable>::Lookup>> &indirect) {
    using namespace std;

    if (lookup->substitutions.empty()) {
      return Status::Ok();
    }

    vector<pair<GG, GG>> single;
    vector<pair<GG, vector<GG>>> multiple;
    vector<pair<vector<GG>, GG>> ligature;

    for (auto const &subst : lookup->substitutions) {
      if (subst->input.size() == 1 && subst->output.size() == 1) {
        single.push_back(make_pair(subst->input[0], subst->output[0]));
      } else if (subst->input.size() == 1 && subst->output.size() > 1) {
        multiple.push_back(make_pair(subst->input[0], subst->output));
      } else if (subst->input.size() > 1 && subst->output.size() == 1) {
        ligature.push_back(make_pair(subst->input, subst->output[0]));
      } else {
        return EGLYF_ERROR_WHAT("Unsupported substitution pattern");
      }
    }
    if (ligature.size() > 0 && single.size() > 0) {
      for (auto [input, output] : single) {
        vector<GG> v;
        v.push_back(input);
        ligature.push_back(make_pair(v, output));
      }
      single.clear();
    }

    // clang-format off
    vector<
      pair<
        shared_ptr<SubtableCollection<Subtable>::Lookup>,
        map<
          size_t,
          vector<shared_ptr<Coverage>>
        >
      >
    > lookups;
    // clang-format on

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
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto singleLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        singleLookup->data = lookupData;

        set<uint16_t> inputGlyphIds;
        for (auto const &[input, _] : single) {
          collectGIDSet(input, inputGlyphIds);
        }
        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        inputCoverages[1].push_back(CoverageBuilder::Build(inputGlyphIds));

        lookups.push_back(make_pair(singleLookup, inputCoverages));
      }
    }

    if (!multiple.empty()) {
      shared_ptr<Subtable> subtable;
      if (auto st = convertMultipleGsubLookup(multiple, subtable); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (subtable) {
        auto extensionSubtable = make_shared<gsub::SubstitutionExtension>();
        extensionSubtable->extensionLookupType = 2; // Multiple
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto multipleLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        multipleLookup->data = lookupData;

        set<uint16_t> inputGlyphIds;
        for (auto const &[input, _] : multiple) {
          collectGIDSet(input, inputGlyphIds);
        }
        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        inputCoverages[1].push_back(CoverageBuilder::Build(inputGlyphIds));

        lookups.push_back(make_pair(multipleLookup, inputCoverages));
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
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto ligatureLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
        ligatureLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        map<size_t, vector<set<uint16_t>>> inputGlyphs;
        for (auto const &[inputs, _] : ligature) {
          auto &glyphs = inputGlyphs[inputs.size()];
          glyphs.resize(inputs.size());
          for (size_t i = 0; i < inputs.size(); i++) {
            auto const &input = inputs[i];
            collectGIDSet(input, glyphs[i]);
          }
        }
        for (auto const &[numInputs, glyphs] : inputGlyphs) {
          auto &target = inputCoverages[numInputs];
          for (auto const &v : glyphs) {
            target.push_back(CoverageBuilder::Build(v));
          }
        }

        lookups.push_back(make_pair(ligatureLookup, inputCoverages));
      }
    }

    if (lookups.empty()) {
      return Status::Ok();
    }

    auto lookupData = make_shared<SubtableCollection<Subtable>::LookupData>();
    lookupData->name = lookup->name;
    lookupData->lookupType = 7; // Extension Substitution
    auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
    if (!lookupFlag) {
      return EGLYF_STATUS_PUSH(lookupFlag.status());
    }
    lookupData->lookupFlag = *lookupFlag;
    lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);

    if (lookup->inContexts.empty() && lookup->exceptContexts.empty()) {
      for (auto const &[lookup, inputCoverages] : lookups) {
        result.push_back(lookup);
      }
      return Status::Ok();
    } else {
      for (auto const &[lookup, inputCoverages] : lookups) {
        indirect.push_back(lookup);
      }
    }

    addContextConditions<gsub::SubstitutionExtension, 6>(lookups,
                                                         lookup->inContexts,
                                                         lookup->exceptContexts,
                                                         lookupData->subtables);
    auto gsubLookup = make_shared<SubtableCollection<Subtable>::Lookup>();
    gsubLookup->data = lookupData;

    result.push_back(gsubLookup);

    return Status::Ok();
  }

  Status createSizeVariants() {
    using namespace std;

    map<int, vector<int>> variationChain;
    variationChain[76] = {75, 56, 55, 45, 44, 33, 22, 11};
    variationChain[66] = {55, 44, 33, 22, 11};
    variationChain[65] = {55, 54, 45, 44, 32, 21, 11};
    variationChain[64] = {62, 54, 52, 43, 42, 32, 22, 11};
    variationChain[63] = {62, 61, 43, 33, 32, 31, 21, 11};
    variationChain[62] = {61, 52, 51, 42, 32, 31, 21, 11};
    variationChain[61] = {51, 41, 31, 21, 11};
    variationChain[56] = {45, 44, 35, 33, 22, 11};
    variationChain[55] = {55, 44, 33, 22, 11};
    variationChain[54] = {53, 43, 33, 22, 21, 11};
    variationChain[53] = {52, 42, 32, 31, 21, 11};
    variationChain[52] = {51, 42, 31, 21, 11};
    variationChain[46] = {35, 23, 11};
    variationChain[45] = {44, 34, 33, 22, 11};
    variationChain[44] = {33, 22, 11};
    variationChain[43] = {32, 31, 21, 11};
    variationChain[42] = {41, 32, 31, 21, 11};
    variationChain[36] = {34, 33, 26, 23, 22, 16, 12, 11};
    variationChain[35] = {25, 24, 23, 13, 12, 11};
    variationChain[34] = {24, 23, 13, 12, 11};
    variationChain[33] = {22, 11};
    variationChain[32] = {32, 31, 21, 11};
    variationChain[26] = {25, 24, 23, 16, 14, 13, 12, 11};
    variationChain[25] = {24, 15, 14, 11};
    variationChain[24] = {23, 14, 13, 12, 11};
    variationChain[23] = {13, 12, 11};
    variationChain[22] = {11};
    variationChain[21] = {11};
    variationChain[16] = {15, 14, 13, 12, 11};
    variationChain[12] = {11};

    deque<pair<int, float>> sizeList;
    for (auto const &[key, _] : variationChain) {
      int h = key / 10;
      int v = key % 10;
      sizeList.push_back(make_pair(h * 10 + v, h * v));
    }
    ranges::stable_sort(sizeList, [](auto const &a, auto const &b) { return a.second < b.second; });

    map<uint16_t, pair<uint32_t, Rect<int16_t>>> bounds;

    if (!holds_alternative<FontFile::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto &outline = get<FontFile::TrueTypeOutlines>(font->outlines);
    auto &glyf = outline.glyf;
    auto const process = [&, this](uint32_t cp) {
      auto gid = font->cmap->getGlyphId(cp);
      if (!gid) {
        return;
      }
      auto g = glyf->glyphs[*gid];
      if (holds_alternative<GlyphDataTable::ReadonlyGlyph>(g)) {
        auto const &r = get<GlyphDataTable::ReadonlyGlyph>(g);
        bounds[*gid] = make_pair(cp, Rect<int16_t>(r.header.xMin, r.header.yMin, r.header.xMax, r.header.yMax));
      } else if (holds_alternative<GlyphDataTable::CompositeGlyph>(g)) {
        auto const &c = get<GlyphDataTable::CompositeGlyph>(g);
        bounds[*gid] = make_pair(cp, Rect<int16_t>(c.header.xMin, c.header.yMin, c.header.xMax, c.header.yMax));
      }
    };
    Unicode::EnumerateHieroglyphUnicode([&](uint32_t cp) {
      process(cp);
    });

    if (bounds.empty()) {
      return EGLYF_ERROR;
    }

    int const presentationHeight = font->hhea->ascender - font->hhea->descender;
    int const fontHeight = font->head->unitsPerEm;
    float const presentationScale = fontHeight / (float)presentationHeight;
    int const topMargin = (int)round(fontHeight * 0.0322);
    int const bottomMargin = (int)round(fontHeight * 0.0615);
    int const maxHeight = fontHeight - topMargin - bottomMargin;
    int const bottom = (int)round(font->hhea->descender * presentationScale + bottomMargin);
    int maxWidth = 0;

    for (auto const &[gid, item] : bounds) {
      auto const &[cp, rect] = item;
      auto const w = rect.width();
      auto const h = rect.height();
      float const scale = h / (float)maxHeight;
      int scaledWidth;
      if (scale > 1) {
        scaledWidth = (int)round(w / scale);
      } else {
        scaledWidth = w;
      }
      maxWidth = max(maxWidth, scaledWidth);
    }

    hfu = (int16_t)std::ceilf(maxWidth * 3.0f / (2 + hhu * 3));
    hg0 = 2 * hfu / 3;
    vfu = (int16_t)std::ceilf(maxHeight * 3.0f / (2 + vhu * 3));
    vg0 = 2 * vfu / 3;
    sb = hfu / 3;

    map<string, pair<uint16_t, shared_ptr<Glyph>>> baseGlyphs;

    for (auto const &it : bounds) {
      auto const &gid = it.first;
      auto const &item = it.second;
      auto const &cp = item.first;
      auto const &rect = item.second;

      string name;
      auto found = GlyphNames::Get(cp);
      if (found) {
        name = *found;
      } else {
        name = format("u{0:x}", cp);
      }
      if (auto currentName = font->post->getName(gid); currentName) {
        if (name == *currentName) {
          if (auto st = font->post->setName(gid, "." + name); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
        }
      }
      auto w = rect.width();
      auto h = rect.height();
      float const scale = min(1.0f, maxHeight / (float)h);
      int16_t xMid = (rect.xMin + rect.xMax) / 2;
      int16_t dx;
      int16_t dy;
      int16_t lsb;
      if (scale < 1) {
        dx = (int16_t)round(-xMid * scale);
        dy = (int16_t)round((bottom - rect.yMin) * scale);
        lsb = (int16_t)round((rect.xMin - xMid) * scale);
      } else {
        dx = -xMid;
        dy = bottom - rect.yMin;
        lsb = rect.xMin - xMid;
      }
      GlyphDataTable::CompositeGlyph::GlyphRecord record;
      if (scale < 1) {
        record = GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, dx, dy, F2DOT14::FromFloat(scale));
      } else {
        record = GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, dx, dy);
      }
      auto newGid = font->addCompositeGlyph(name, record, 0, lsb, 0, 0);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
      if (auto st = font->cmap->map(cp, *newGid); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (found) {
        auto newGlyph = getGlyphByName(name);
        if (!newGlyph) {
          return EGLYF_ERROR;
        }
        baseGlyphs[name] = make_pair(gid, newGlyph);
      }
    }

    for (int h = 1; h <= hhu; h++) {
      string name = format("QB{0}", h);
      if (auto gid = font->post->getGlyphId(name); gid) {
        if (auto st = font->post->setName(*gid, "." + name); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      int16_t width = this->width(h);
      auto newGid = font->addEmptyGlyph(name, width, 0, fontHeight, fontHeight - topMargin);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
    }

    float constexpr kAspectDiffThreshold = 0.2f;
    for (auto const &it : baseGlyphs) {
      auto const &name = it.first;
      auto const &glyphs = it.second;
      auto const &originalGID = glyphs.first;
      auto const &newGlyph = glyphs.second;

      auto found = bounds.find(originalGID);
      if (found == bounds.end()) {
        continue;
      }
      auto const &[cp, rect] = found->second;

      float const baseScale = min(1.0f, maxHeight / (float)rect.height());

      int width = (int)ceilf((rect.width() * baseScale - (float)hg0) / hfu);
      int height = (int)ceilf((rect.height() * baseScale - (float)vg0) / vfu);
      width = min(max(width, 1), hhu);
      height = min(max(height, 1), vhu);
      int16_t xMid = (rect.xMin + rect.xMax) / 2;

      auto chain = variationChain.find(width * 10 + height);
      if (chain == variationChain.end()) {
        if (auto first = ranges::find_if(sizeList, [=](auto const &it) { return it.second >= width * height; }); first != sizeList.end()) {
          auto index = distance(sizeList.begin(), first);
          for (size_t i = index; i < sizeList.size(); i++) {
            auto const &it = sizeList[i];
            int key = it.first;
            int h = key / 10;
            int v = key % 10;
            if (h < width || v < height) {
              continue;
            }
            chain = variationChain.find(key);
            if (chain != variationChain.end()) {
              width = h;
              height = v;
              break;
            }
          }
        }
      }

      SizeVariants sv;
      sv.base = newGlyph;
      sv.hGrids = width;
      sv.vGrids = height;

      if (chain == variationChain.end()) {
        sizeVariants[name] = sv;
        continue;
      }

      for (auto key : chain->second) {
        int yLevel = key % 10;
        int xLevel = key / 10;

        string n = format("{0}_{1}{2}", name, xLevel, yLevel);
        float xScale = this->width(xLevel) / (float)this->width(width) * baseScale;
        float yScale = this->height(yLevel) / (float)this->height(height) * baseScale;
        float scale = min({1.0f, xScale, yScale});
        int16_t dx;
        int16_t dy;
        if (scale < 1) {
          dx = (int16_t)round(-xMid * scale);
          dy = (int16_t)round((bottom - rect.yMin) * scale);
        } else {
          dx = -xMid;
          dy = bottom - rect.yMin;
        }
        auto record = GlyphDataTable::CompositeGlyph::GlyphRecord::New(originalGID, dx, dy, F2DOT14::FromFloat(scale));
        auto newGid = font->addCompositeGlyph(n, record, 0, dx, 0, 0);
        if (!newGid) {
          return EGLYF_STATUS_PUSH(newGid.status());
        }
        auto variationGlyph = getGlyphById(*newGid);
        if (!variationGlyph) {
          return EGLYF_ERROR;
        }
        sv.variants[key] = variationGlyph;
      }

      sizeVariants[name] = sv;
    }

    return Status::Ok();
  }

  int width(int level) const {
    return (int)hg0 + (int)hfu * level;
  }

  int height(int level) const {
    return (int)vg0 + (int)vfu * level;
  }

  Status preprocess() {
    if (auto st = createSizeVariants(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status replaceLookups() {
    if (auto st = replaceLookup_pr021_tsg_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps045_targetglyphs_0_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps046_targetglyphs_1_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  void setupSubst(Lookup &lookup, std::initializer_list<std::string> input, std::initializer_list<std::string> output) {
    using namespace std;
    auto s = make_shared<Lookup::Substitution>();
    for (auto i : input) {
      auto g = getGlyphByName(i);
      s->input.push_back(g);
    }
    for (auto o : output) {
      auto g = getGlyphByName(o);
      s->output.push_back(g);
    }
    lookup.substitutions.push_back(s);
  }

  Status replaceLookup_pr021_tsg_A() {
    using namespace std;
    auto a = getLookupByName("pr021_tsg_A");
    if (!a) {
      return EGLYF_ERROR;
    }
    a->substitutions.clear();
    setupSubst(*a, {"BF1"}, {"et66", "tsh666564636261565554535251464544434241363534333231262524232221161514131211", "BF1", "Qf"});
    setupSubst(*a, {"BQ1"}, {"et33", "tsh33", "BQ1", "Qf"});
    setupSubst(*a, {"LF1"}, {"et66", "tsh666564636261565554535251464544434241363534333231262524232221161514131211", "LF1", "Qf"});
    setupSubst(*a, {"LQ1"}, {"et33", "tsh332211", "LQ1", "Qf"});
    setupSubst(*a, {"LT1"}, {"et36", "tsh363534262524231615141312", "LT1", "Qf"});
    setupSubst(*a, {"LW1"}, {"et63", "tsh636261535251434241323121", "LW1", "Qf"});
    setupSubst(*a, {"LF2"}, {"et66", "tsh666564636261565554535251464544434241363534333231262524232221161514131211", "LF2", "Qf"});
    setupSubst(*a, {"LQ2"}, {"et33", "tsh332211", "LQ2", "Qf"});
    setupSubst(*a, {"LT2"}, {"et36", "tsh363534262524231615141312", "LT2", "Qf"});
    setupSubst(*a, {"LW2"}, {"et63", "tsh636261535251434241323121", "LW2", "Qf"});
    auto Qf = getGlyphByName("Qf");
    for (auto const &it : sizeVariants) {
      // SUB GLYPH "A1"
      // WITH GLYPH "et56" GLYPH "tsh56454435332211" GLYPH "A1" GLYPH "Qf"

      auto const &name = it.first;
      auto const &sv = it.second;

      auto s = make_shared<Lookup::Substitution>();
      s->input.push_back(sv.base);
      auto et = getGlyphByName(format("et{0}{1}", sv.hGrids, sv.vGrids));
      s->output.push_back(et);
      string tsh = "";
      for (auto const &[variant, glyph] : sv.variants) {
        tsh = format("{0}", variant) + tsh;
      }
      tsh = format("{0}{1}", sv.hGrids, sv.vGrids) + tsh;
      tsh = "tsh" + tsh;
      s->output.push_back(getGlyphByName(tsh));
      s->output.push_back(sv.base);
      s->output.push_back(Qf);
      a->substitutions.push_back(s);
    }

    return Status::Ok();
  }

  Status replaceLookup_ps045_targetglyphs_0_A() {
    using namespace std;
    auto a = getLookupByName("ps045_targetglyphs_0_A");
    if (!a) {
      return EGLYF_ERROR;
    }
    a->substitutions.clear();
    setupSubst(*a, {"et66", "BF1"}, {"BF1"});
    setupSubst(*a, {"et33", "BQ1"}, {"BQ1"});
    setupSubst(*a, {"et66", "LF1"}, {"LF1"});
    setupSubst(*a, {"et33", "LQ1"}, {"LQ1"});
    setupSubst(*a, {"et36", "LT1"}, {"LT1"});
    setupSubst(*a, {"et63", "LW1"}, {"LW1"});
    setupSubst(*a, {"et54", "GB1"}, {"GB1"});
    setupSubst(*a, {"et66", "GB1"}, {"GB1_66"});
    setupSubst(*a, {"et65", "GB1"}, {"GB1_65"});
    setupSubst(*a, {"et64", "GB1"}, {"GB1_64"});
    setupSubst(*a, {"et63", "GB1"}, {"GB1_63"});
    setupSubst(*a, {"et62", "GB1"}, {"GB1_62"});
    setupSubst(*a, {"et61", "GB1"}, {"GB1_61"});
    setupSubst(*a, {"et56", "GB1"}, {"GB1_56"});
    setupSubst(*a, {"et55", "GB1"}, {"GB1_55"});
    setupSubst(*a, {"et54", "GB1"}, {"GB1_54"});
    setupSubst(*a, {"et53", "GB1"}, {"GB1_53"});
    setupSubst(*a, {"et52", "GB1"}, {"GB1_52"});
    setupSubst(*a, {"et51", "GB1"}, {"GB1_51"});
    setupSubst(*a, {"et46", "GB1"}, {"GB1_46"});
    setupSubst(*a, {"et45", "GB1"}, {"GB1_45"});
    setupSubst(*a, {"et44", "GB1"}, {"GB1_44"});
    setupSubst(*a, {"et43", "GB1"}, {"GB1_43"});
    setupSubst(*a, {"et42", "GB1"}, {"GB1_42"});
    setupSubst(*a, {"et41", "GB1"}, {"GB1_41"});
    setupSubst(*a, {"et36", "GB1"}, {"GB1_36"});
    setupSubst(*a, {"et35", "GB1"}, {"GB1_35"});
    setupSubst(*a, {"et34", "GB1"}, {"GB1_34"});
    setupSubst(*a, {"et33", "GB1"}, {"GB1_33"});
    setupSubst(*a, {"et32", "GB1"}, {"GB1_32"});
    setupSubst(*a, {"et31", "GB1"}, {"GB1_31"});
    setupSubst(*a, {"et26", "GB1"}, {"GB1_26"});
    setupSubst(*a, {"et25", "GB1"}, {"GB1_25"});
    setupSubst(*a, {"et24", "GB1"}, {"GB1_24"});
    setupSubst(*a, {"et23", "GB1"}, {"GB1_23"});
    setupSubst(*a, {"et22", "GB1"}, {"GB1_22"});
    setupSubst(*a, {"et21", "GB1"}, {"GB1_21"});
    setupSubst(*a, {"et16", "GB1"}, {"GB1_16"});
    setupSubst(*a, {"et15", "GB1"}, {"GB1_15"});
    setupSubst(*a, {"et14", "GB1"}, {"GB1_14"});
    setupSubst(*a, {"et13", "GB1"}, {"GB1_13"});
    setupSubst(*a, {"et12", "GB1"}, {"GB1_12"});
    setupSubst(*a, {"et11", "GB1"}, {"GB1_11"});
    setupSubst(*a, {"et65", "LF1"}, {"LF1_65"});
    setupSubst(*a, {"et64", "LF1"}, {"LF1_64"});
    setupSubst(*a, {"et63", "LF1"}, {"LF1_63"});
    setupSubst(*a, {"et62", "LF1"}, {"LF1_62"});
    setupSubst(*a, {"et61", "LF1"}, {"LF1_61"});
    setupSubst(*a, {"et56", "LF1"}, {"LF1_56"});
    setupSubst(*a, {"et55", "LF1"}, {"LF1_55"});
    setupSubst(*a, {"et54", "LF1"}, {"LF1_54"});
    setupSubst(*a, {"et53", "LF1"}, {"LF1_53"});
    setupSubst(*a, {"et52", "LF1"}, {"LF1_52"});
    setupSubst(*a, {"et51", "LF1"}, {"LF1_51"});
    setupSubst(*a, {"et46", "LF1"}, {"LF1_46"});
    setupSubst(*a, {"et45", "LF1"}, {"LF1_45"});
    setupSubst(*a, {"et44", "LF1"}, {"LF1_44"});
    setupSubst(*a, {"et43", "LF1"}, {"LF1_43"});
    setupSubst(*a, {"et42", "LF1"}, {"LF1_42"});
    setupSubst(*a, {"et41", "LF1"}, {"LF1_41"});
    setupSubst(*a, {"et36", "LF1"}, {"LF1_36"});
    setupSubst(*a, {"et35", "LF1"}, {"LF1_35"});
    setupSubst(*a, {"et34", "LF1"}, {"LF1_34"});
    setupSubst(*a, {"et33", "LF1"}, {"LF1_33"});
    setupSubst(*a, {"et32", "LF1"}, {"LF1_32"});
    setupSubst(*a, {"et31", "LF1"}, {"LF1_31"});
    setupSubst(*a, {"et26", "LF1"}, {"LF1_26"});
    setupSubst(*a, {"et25", "LF1"}, {"LF1_25"});
    setupSubst(*a, {"et24", "LF1"}, {"LF1_24"});
    setupSubst(*a, {"et23", "LF1"}, {"LF1_23"});
    setupSubst(*a, {"et22", "LF1"}, {"LF1_22"});
    setupSubst(*a, {"et21", "LF1"}, {"LF1_21"});
    setupSubst(*a, {"et16", "LF1"}, {"LF1_16"});
    setupSubst(*a, {"et15", "LF1"}, {"LF1_15"});
    setupSubst(*a, {"et14", "LF1"}, {"LF1_14"});
    setupSubst(*a, {"et13", "LF1"}, {"LF1_13"});
    setupSubst(*a, {"et12", "LF1"}, {"LF1_12"});
    setupSubst(*a, {"et11", "LF1"}, {"LF1_11"});
    setupSubst(*a, {"et66", "LF2"}, {"LF2"});
    setupSubst(*a, {"et65", "LF2"}, {"LF2_65"});
    setupSubst(*a, {"et64", "LF2"}, {"LF2_64"});
    setupSubst(*a, {"et63", "LF2"}, {"LF2_63"});
    setupSubst(*a, {"et62", "LF2"}, {"LF2_62"});
    setupSubst(*a, {"et61", "LF2"}, {"LF2_61"});
    setupSubst(*a, {"et56", "LF2"}, {"LF2_56"});
    setupSubst(*a, {"et55", "LF2"}, {"LF2_55"});
    setupSubst(*a, {"et54", "LF2"}, {"LF2_54"});
    setupSubst(*a, {"et53", "LF2"}, {"LF2_53"});
    setupSubst(*a, {"et52", "LF2"}, {"LF2_52"});
    setupSubst(*a, {"et51", "LF2"}, {"LF2_51"});
    setupSubst(*a, {"et46", "LF2"}, {"LF2_46"});
    setupSubst(*a, {"et45", "LF2"}, {"LF2_45"});
    setupSubst(*a, {"et44", "LF2"}, {"LF2_44"});
    setupSubst(*a, {"et43", "LF2"}, {"LF2_43"});
    setupSubst(*a, {"et42", "LF2"}, {"LF2_42"});
    setupSubst(*a, {"et41", "LF2"}, {"LF2_41"});
    setupSubst(*a, {"et36", "LF2"}, {"LF2_36"});
    setupSubst(*a, {"et35", "LF2"}, {"LF2_35"});
    setupSubst(*a, {"et34", "LF2"}, {"LF2_34"});
    setupSubst(*a, {"et33", "LF2"}, {"LF2_33"});
    setupSubst(*a, {"et32", "LF2"}, {"LF2_32"});
    setupSubst(*a, {"et31", "LF2"}, {"LF2_31"});
    setupSubst(*a, {"et26", "LF2"}, {"LF2_26"});
    setupSubst(*a, {"et25", "LF2"}, {"LF2_25"});
    setupSubst(*a, {"et24", "LF2"}, {"LF2_24"});
    setupSubst(*a, {"et23", "LF2"}, {"LF2_23"});
    setupSubst(*a, {"et22", "LF2"}, {"LF2_22"});
    setupSubst(*a, {"et21", "LF2"}, {"LF2_21"});
    setupSubst(*a, {"et16", "LF2"}, {"LF2_16"});
    setupSubst(*a, {"et15", "LF2"}, {"LF2_15"});
    setupSubst(*a, {"et14", "LF2"}, {"LF2_14"});
    setupSubst(*a, {"et13", "LF2"}, {"LF2_13"});
    setupSubst(*a, {"et12", "LF2"}, {"LF2_12"});
    setupSubst(*a, {"et11", "LF2"}, {"LF2_11"});
    setupSubst(*a, {"et22", "LQ1"}, {"LQ1_22"});
    setupSubst(*a, {"et11", "LQ1"}, {"LQ1_11"});
    setupSubst(*a, {"et33", "LQ2"}, {"LQ2"});
    setupSubst(*a, {"et22", "LQ2"}, {"LQ2_22"});
    setupSubst(*a, {"et11", "LQ2"}, {"LQ2_11"});
    setupSubst(*a, {"et35", "LT1"}, {"LT1_35"});
    setupSubst(*a, {"et34", "LT1"}, {"LT1_34"});
    setupSubst(*a, {"et26", "LT1"}, {"LT1_26"});
    setupSubst(*a, {"et25", "LT1"}, {"LT1_25"});
    setupSubst(*a, {"et24", "LT1"}, {"LT1_24"});
    setupSubst(*a, {"et23", "LT1"}, {"LT1_23"});
    setupSubst(*a, {"et16", "LT1"}, {"LT1_16"});
    setupSubst(*a, {"et15", "LT1"}, {"LT1_15"});
    setupSubst(*a, {"et14", "LT1"}, {"LT1_14"});
    setupSubst(*a, {"et13", "LT1"}, {"LT1_13"});
    setupSubst(*a, {"et12", "LT1"}, {"LT1_12"});
    setupSubst(*a, {"et36", "LT2"}, {"LT2"});
    setupSubst(*a, {"et35", "LT2"}, {"LT2_35"});
    setupSubst(*a, {"et34", "LT2"}, {"LT2_34"});
    setupSubst(*a, {"et26", "LT2"}, {"LT2_26"});
    setupSubst(*a, {"et25", "LT2"}, {"LT2_25"});
    setupSubst(*a, {"et24", "LT2"}, {"LT2_24"});
    setupSubst(*a, {"et23", "LT2"}, {"LT2_23"});
    setupSubst(*a, {"et16", "LT2"}, {"LT2_16"});
    setupSubst(*a, {"et15", "LT2"}, {"LT2_15"});
    setupSubst(*a, {"et14", "LT2"}, {"LT2_14"});
    setupSubst(*a, {"et13", "LT2"}, {"LT2_13"});
    setupSubst(*a, {"et12", "LT2"}, {"LT2_12"});
    setupSubst(*a, {"et62", "LW1"}, {"LW1_62"});
    setupSubst(*a, {"et61", "LW1"}, {"LW1_61"});
    setupSubst(*a, {"et53", "LW1"}, {"LW1_53"});
    setupSubst(*a, {"et52", "LW1"}, {"LW1_52"});
    setupSubst(*a, {"et51", "LW1"}, {"LW1_51"});
    setupSubst(*a, {"et43", "LW1"}, {"LW1_43"});
    setupSubst(*a, {"et42", "LW1"}, {"LW1_42"});
    setupSubst(*a, {"et41", "LW1"}, {"LW1_41"});
    setupSubst(*a, {"et32", "LW1"}, {"LW1_32"});
    setupSubst(*a, {"et31", "LW1"}, {"LW1_31"});
    setupSubst(*a, {"et21", "LW1"}, {"LW1_21"});
    setupSubst(*a, {"et63", "LW2"}, {"LW2"});
    setupSubst(*a, {"et62", "LW2"}, {"LW2_62"});
    setupSubst(*a, {"et61", "LW2"}, {"LW2_61"});
    setupSubst(*a, {"et53", "LW2"}, {"LW2_53"});
    setupSubst(*a, {"et52", "LW2"}, {"LW2_52"});
    setupSubst(*a, {"et51", "LW2"}, {"LW2_51"});
    setupSubst(*a, {"et43", "LW2"}, {"LW2_43"});
    setupSubst(*a, {"et42", "LW2"}, {"LW2_42"});
    setupSubst(*a, {"et41", "LW2"}, {"LW2_41"});
    setupSubst(*a, {"et32", "LW2"}, {"LW2_32"});
    setupSubst(*a, {"et31", "LW2"}, {"LW2_31"});
    setupSubst(*a, {"et21", "LW2"}, {"LW2_21"});
    for (auto const &it : sizeVariants) {
      // SUB GLYPH "et56" GLYPH "A1"
      // WITH GLYPH "A1"

      auto const &name = it.first;
      auto const &sv = it.second;

      auto s = make_shared<Lookup::Substitution>();
      auto et = format("et{0}{1}", sv.hGrids, sv.vGrids);
      s->input.push_back(getGlyphByName(et));
      s->input.push_back(sv.base);
      s->output.push_back(sv.base);
      a->substitutions.push_back(s);
    }

    return Status::Ok();
  }

  Status replaceLookup_ps046_targetglyphs_1_A() {
    using namespace std;
    using namespace std;
    auto a = getLookupByName("ps046_targetglyphs_1_A");
    if (!a) {
      return EGLYF_ERROR;
    }
    a->substitutions.clear();
    for (auto const &it : sizeVariants) {
      auto const &name = it.first;
      auto const &sv = it.second;
      {
        // SUB GLYPH "et45" GLYPH "A1"
        // WITH GLYPH "A1_45"

        auto s = make_shared<Lookup::Substitution>();
        auto et = format("et{0}{1}", sv.hGrids, sv.vGrids);
        s->input.push_back(getGlyphByName(et));
        s->input.push_back(sv.base);
        auto sized = name + format("{0}{1}", sv.hGrids, sv.vGrids);
        s->output.push_back(getGlyphByName(sized));
        a->substitutions.push_back(s);
      }
      for (auto const [key, glyph] : sv.variants) {
        // SUB GLYPH "et44" GLYPH "A1"
        // WITH GLYPH "A1_44"

        auto s = make_shared<Lookup::Substitution>();
        auto et = format("et{0}", key);
        s->input.push_back(getGlyphByName(et));
        s->input.push_back(sv.base);
        auto v = format("{0}_{1}", name, key);
        s->output.push_back(getGlyphByName(v));
        a->substitutions.push_back(s);
      }
    }
    setupSubst(*a, {"et11", "BF1"}, {"BF1_11"});
    setupSubst(*a, {"et12", "BF1"}, {"BF1_12"});
    setupSubst(*a, {"et13", "BF1"}, {"BF1_13"});
    setupSubst(*a, {"et14", "BF1"}, {"BF1_14"});
    setupSubst(*a, {"et15", "BF1"}, {"BF1_15"});
    setupSubst(*a, {"et16", "BF1"}, {"BF1_16"});
    setupSubst(*a, {"et21", "BF1"}, {"BF1_21"});
    setupSubst(*a, {"et22", "BF1"}, {"BF1_22"});
    setupSubst(*a, {"et23", "BF1"}, {"BF1_23"});
    setupSubst(*a, {"et24", "BF1"}, {"BF1_24"});
    setupSubst(*a, {"et25", "BF1"}, {"BF1_25"});
    setupSubst(*a, {"et26", "BF1"}, {"BF1_26"});
    setupSubst(*a, {"et31", "BF1"}, {"BF1_31"});
    setupSubst(*a, {"et32", "BF1"}, {"BF1_32"});
    setupSubst(*a, {"et33", "BF1"}, {"BF1_33"});
    setupSubst(*a, {"et34", "BF1"}, {"BF1_34"});
    setupSubst(*a, {"et35", "BF1"}, {"BF1_35"});
    setupSubst(*a, {"et36", "BF1"}, {"BF1_36"});
    setupSubst(*a, {"et41", "BF1"}, {"BF1_41"});
    setupSubst(*a, {"et42", "BF1"}, {"BF1_42"});
    setupSubst(*a, {"et43", "BF1"}, {"BF1_43"});
    setupSubst(*a, {"et44", "BF1"}, {"BF1_44"});
    setupSubst(*a, {"et45", "BF1"}, {"BF1_45"});
    setupSubst(*a, {"et46", "BF1"}, {"BF1_46"});
    setupSubst(*a, {"et51", "BF1"}, {"BF1_51"});
    setupSubst(*a, {"et52", "BF1"}, {"BF1_52"});
    setupSubst(*a, {"et53", "BF1"}, {"BF1_53"});
    setupSubst(*a, {"et54", "BF1"}, {"BF1_54"});
    setupSubst(*a, {"et55", "BF1"}, {"BF1_55"});
    setupSubst(*a, {"et56", "BF1"}, {"BF1_56"});
    setupSubst(*a, {"et61", "BF1"}, {"BF1_61"});
    setupSubst(*a, {"et62", "BF1"}, {"BF1_62"});
    setupSubst(*a, {"et63", "BF1"}, {"BF1_63"});
    setupSubst(*a, {"et64", "BF1"}, {"BF1_64"});
    setupSubst(*a, {"et65", "BF1"}, {"BF1_65"});

    return Status::Ok();
  }

  Status postprocess() {
    if (auto st = replaceLookups(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = relocateMarks(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status relocateMarks() {
    using namespace std;
    if (auto a1 = anchors.find("a1"); a1 != anchors.end()) {
      // DEF_ANCHOR "a1" ON 469 GLYPH QB1 COMPONENT 1 AT  POS DX 105 DY 1860 END_POS END_ANCHOR
      int16_t const dy = vfu * vhu;
      for (auto &it : a1->second->glyphs) {
        it.second = Vec<optional<int16_t>>(sb, dy);
      }
    }
    if (auto r1 = anchors.find("r1"); r1 != anchors.end()) {
      int16_t const dy = vfu * vhu;
      for (auto const &[prefix, suffix] : initializer_list<pair<string, string>>{{"QB", ""}, {"QD", ""}, {"QD", "V"}, {"QF", ""}, {"QF", "V"}, {"QO", ""}, {"QO", "V"}, {"QC", ""}, {"QC", "V"}, {"QW", ""}, {"QW", "V"}}) {
        for (int i = 1; i <= hhu; i++) {
          // DEF_ANCHOR "r1" ON 469 GLYPH QB1 COMPONENT 1 AT  POS DX 420 DY 1860 END_POS END_ANCHOR
          auto name = format("{}{}{}", prefix, i, suffix);
          auto glyph = getGlyphByName(name);
          auto found = r1->second->glyphs.find(glyph);
          if (found == r1->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = sb + hfu * i;
          found->second = Vec<optional<int16_t>>(dx, dy);
        }
      }
    }
    if (auto bottom = anchors.find("bottom"); bottom != anchors.end()) {
      for (auto const &prefix : {"r0v", "r1v", "r2v"}) {
        for (int v = 1; v <= vhu; v++) {
          for (auto const &suffix : {"", "R"}) {
            auto name = format("{}{}{}", prefix, v, suffix);
            auto glyph = getGlyphByName(name);
            auto found = bottom->second->glyphs.find(glyph);
            if (found == bottom->second->glyphs.end()) {
              continue;
            }
            int16_t const dy = -v * vfu;
            found->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
#if 0
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p25 COMPONENT 1 AT  POS DY -77 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p33 COMPONENT 1 AT  POS DY -102 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p5 COMPONENT 1 AT  POS DY -155 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p66 COMPONENT 1 AT  POS DY -204 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s1p0 COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s1p5 COMPONENT 1 AT  POS DY -465 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s2p0 COMPONENT 1 AT  POS DY -620 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s3p0 COMPONENT 1 AT  POS DY -930 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s4p0 COMPONENT 1 AT  POS DY -1240 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p25R COMPONENT 1 AT  POS DY -77 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p33R COMPONENT 1 AT  POS DY -102 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p5R COMPONENT 1 AT  POS DY -155 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s0p66R COMPONENT 1 AT  POS DY -204 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s1p0R COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s1p5R COMPONENT 1 AT  POS DY -465 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s2p0R COMPONENT 1 AT  POS DY -620 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s3p0R COMPONENT 1 AT  POS DY -930 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r0s4p0R COMPONENT 1 AT  POS DY -1240 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s0p33 COMPONENT 1 AT  POS DY -102 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s0p5 COMPONENT 1 AT  POS DY -155 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s1p0 COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s2p0 COMPONENT 1 AT  POS DY -620 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s3p0 COMPONENT 1 AT  POS DY -930 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s4p0 COMPONENT 1 AT  POS DY -1240 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s0p33R COMPONENT 1 AT  POS DY -102 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s0p5R COMPONENT 1 AT  POS DY -155 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s1p0R COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s2p0R COMPONENT 1 AT  POS DY -620 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s3p0R COMPONENT 1 AT  POS DY -930 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r1s4p0R COMPONENT 1 AT  POS DY -1240 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r2s1p0 COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
    DEF_ANCHOR "bottom" ON None GLYPH r2s1p0R COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
#endif
    if (auto MARK_top = anchors.find("MARK_top"); MARK_top != anchors.end()) {
      for (auto const &key : {"a", "b", "l", "p", "r", "u"}) {
        for (int i = 0; i <= 2; i++) {
          for (int v = 1; v <= vhu; v++) {
            auto name = format("tc{}b{}_{}", key, i, v);
            auto glyph = getGlyphByName(name);
            auto found = MARK_top->second->glyphs.find(glyph);
            if (found == MARK_top->second->glyphs.end()) {
              continue;
            }
            int16_t const dy = v * vfu;
            found->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    if (auto MARK_right = anchors.find("MARK_right"); MARK_right != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          auto found = MARK_right->second->glyphs.find(glyph);
          if (found == MARK_right->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = h * hfu;
          int16_t const dy = v * vfu;
          found->second = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (auto const &key : {"a", "b", "l", "p", "r", "u"}) {
        for (int i = 0; i <= 2; i++) {
          for (int v = 1; v <= vhu; v++) {
            auto name = format("tc{}e{}_{}", key, i, v);
            auto glyph = getGlyphByName(name);
            auto found = MARK_right->second->glyphs.find(glyph);
            if (found == MARK_right->second->glyphs.end()) {
              continue;
            }
            int16_t const dy = v * vfu;
            found->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    if (auto right = anchors.find("right"); right != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        // DEF_ANCHOR "right" ON None GLYPH c0h1 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        auto name = format("c0h{}", h);
        auto glyph = getGlyphByName(name);
        auto found = right->second->glyphs.find(glyph);
        if (found == right->second->glyphs.end()) {
          continue;
        }
        int16_t const dx = h * hfu;
        found->second = Vec<optional<int16_t>>(dx, nullopt);
      }
      for (int i = 1; i <= 2; i++) {
        for (int h = 1; h <= chu; h++) {
          // DEF_ANCHOR "right" ON None GLYPH c1h1 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("c{}h{}", i, h);
          auto glyph = getGlyphByName(name);
          auto found = right->second->glyphs.find(glyph);
          if (found == right->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = h * hfu;
          found->second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
#if 0
      DEF_ANCHOR "right" ON None GLYPH c0s0p25 COMPONENT 1 AT  POS DX 78 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s0p33 COMPONENT 1 AT  POS DX 103 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s0p5 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s0p66 COMPONENT 1 AT  POS DX 207 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s1p0 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s1p5 COMPONENT 1 AT  POS DX 472 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s2p0 COMPONENT 1 AT  POS DX 630 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s3p0 COMPONENT 1 AT  POS DX 945 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c0s4p0 COMPONENT 1 AT  POS DX 1260 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c1s0p33 COMPONENT 1 AT  POS DX 103 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c1s0p5 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c1s1p0 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c1s2p0 COMPONENT 1 AT  POS DX 630 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c1s3p0 COMPONENT 1 AT  POS DX 945 END_POS END_ANCHOR
      DEF_ANCHOR "right" ON None GLYPH c2s1p0 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
#endif
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "right" ON None GLYPH o11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          auto found = right->second->glyphs.find(glyph);
          if (found == right->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = h * hfu;
          found->second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (auto const &key : {"s", "i", "es", "om", "om2"}) {
        // DEF_ANCHOR "right" ON None GLYPH s11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH i11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH es11 COMPONENT 1 AT  POS DX 315 DY 310 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH om11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH om211 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            auto name = format("{}{}{}", key, h, v);
            auto glyph = getGlyphByName(name);
            auto found = right->second->glyphs.find(glyph);
            if (found == right->second->glyphs.end()) {
              continue;
            }
            int16_t const dx = h * hfu;
            found->second = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "right" ON None GLYPH it11R COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("it{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          auto found = right->second->glyphs.find(glyph);
          if (found == right->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = h * hfu;
          found->second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
    }
    if (auto left = anchors.find("left"); left != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        // DEF_ANCHOR "left" ON None GLYPH c0h1R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
        auto name = format("c0h{}R", h);
        auto glyph = getGlyphByName(name);
        auto found = left->second->glyphs.find(glyph);
        if (found == left->second->glyphs.end()) {
          continue;
        }
        int16_t const dx = -h * hfu;
        found->second = Vec<optional<int16_t>>(dx, nullopt);
      }
      for (int i = 1; i <= 2; i++) {
        for (int h = 1; h <= chu; h++) {
          // DEF_ANCHOR "left" ON None GLYPH c1h1R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
          auto name = format("c{}h{}R", i, h);
          auto glyph = getGlyphByName(name);
          auto found = left->second->glyphs.find(glyph);
          if (found == left->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = -h * hfu;
          found->second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
#if 0
      DEF_ANCHOR "left" ON None GLYPH c0s0p25R COMPONENT 1 AT  POS DX -78 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s0p33R COMPONENT 1 AT  POS DX -103 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s0p5R COMPONENT 1 AT  POS DX -157 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s0p66R COMPONENT 1 AT  POS DX -207 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s1p0R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s1p5R COMPONENT 1 AT  POS DX -472 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s2p0R COMPONENT 1 AT  POS DX -630 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s3p0R COMPONENT 1 AT  POS DX -945 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c0s4p0R COMPONENT 1 AT  POS DX -1260 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c1s0p33R COMPONENT 1 AT  POS DX -103 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c1s0p5R COMPONENT 1 AT  POS DX -157 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c1s1p0R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c1s2p0R COMPONENT 1 AT  POS DX -630 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c1s3p0R COMPONENT 1 AT  POS DX -945 END_POS END_ANCHOR
      DEF_ANCHOR "left" ON None GLYPH c2s1p0R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
#endif
      for (int i = 1; i <= chu; i++) {
        for (int h = 1; h <= chu; h++) {
          // DEF_ANCHOR "left" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, h);
          auto glyph = getGlyphByName(name);
          auto found = left->second->glyphs.find(glyph);
          if (found == left->second->glyphs.end()) {
            continue;
          }
          int16_t const dx = h * hfu;
          found->second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
    }
    if (auto MARK_left = anchors.find("MARK_left"); MARK_left != anchors.end()) {
      for (int i = 1; i <= chu; i++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_left" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, v);
          auto glyph = getGlyphByName(name);
          auto found = MARK_left->second->glyphs.find(glyph);
          if (found == MARK_left->second->glyphs.end()) {
            continue;
          }
          int16_t const dy = v * vfu;
          found->second = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    if (auto MARK_ts = anchors.find("MARK_ts"); MARK_ts != anchors.end()) {
      for (int i = 1; i <= chu; i++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_ts" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, v);
          auto glyph = getGlyphByName(name);
          auto found = MARK_ts->second->glyphs.find(glyph);
          if (found == MARK_ts->second->glyphs.end()) {
            continue;
          }
          int16_t const dy = v * vfu;
          found->second = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    auto bs = anchors.find("bs");
    auto te = anchors.find("te");
    auto be = anchors.find("be");
    if (bs != anchors.end() && bs != anchors.end() && be != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "bs" ON None GLYPH o8(6) COMPONENT 1 AT  POS DY -(1860) END_POS END_ANCHOR
          // DEF_ANCHOR "te" ON None GLYPH o[8]6 COMPONENT 1 AT  POS DX [2520] END_POS END_ANCHOR
          // DEF_ANCHOR "be" ON None GLYPH o[8](6) COMPONENT 1 AT  POS DX [2520] DY -(1860) END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbs = bs->second->glyphs.find(glyph); fbs != bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
          if (auto fte = te->second->glyphs.find(glyph); fte != te->second->glyphs.end()) {
            fte->second = Vec<optional<int16_t>>(dx, nullopt);
          }
          if (auto fbe = be->second->glyphs.find(glyph); fbe != be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "bs" ON None GLYPH s6(5) COMPONENT 1 AT  POS DY -(1550) END_POS END_ANCHOR
          // DEF_ANCHOR "te" ON None GLYPH s[6]5 COMPONENT 1 AT  POS DX [1890] END_POS END_ANCHOR
          // DEF_ANCHOR "be" ON None GLYPH s[6](5) COMPONENT 1 AT  POS DX [1890] DY -(1550) END_POS END_ANCHOR
          auto name = format("s{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbs = bs->second->glyphs.find(glyph); fbs != bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
          if (auto fte = te->second->glyphs.find(glyph); fte != te->second->glyphs.end()) {
            fte->second = Vec<optional<int16_t>>(dx, nullopt);
          }
          if (auto fbe = be->second->glyphs.find(glyph); fbe != be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
    }
    auto MARK_bs = anchors.find("MARK_bs");
    auto MARK_be = anchors.find("MARK_be");
    if (MARK_bs != anchors.end() && MARK_be != anchors.end()) {
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_bs" ON None GLYPH bs6(5) COMPONENT 1 AT  POS DY -(1550) END_POS END_ANCHOR
          // DEF_ANCHOR "MARK_be" ON None GLYPH bs[6](5) COMPONENT 1 AT  POS DX [1890] DY -(1550) END_POS END_ANCHOR
          auto name = format("bs{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbs = MARK_bs->second->glyphs.find(glyph); fbs != MARK_bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
          if (auto fbe = MARK_be->second->glyphs.find(glyph); fbe != MARK_be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_bs" ON None GLYPH bs265 COMPONENT 1 AT  POS DY -1550 END_POS END_ANCHOR
          // DEF_ANCHOR "MARK_be" ON None GLYPH bs265 COMPONENT 1 AT  POS DX 1890 DY -1550 END_POS END_ANCHOR
          auto name = format("bs2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbs = MARK_bs->second->glyphs.find(glyph); fbs != MARK_bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
          if (auto fbe = MARK_be->second->glyphs.find(glyph); fbe != MARK_be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
    }
    if (auto MARK_te = anchors.find("MARK_te"); MARK_te != anchors.end()) {
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_te" ON None GLYPH te65 COMPONENT 1 AT  POS DX 1890 END_POS END_ANCHOR
          auto name = format("te{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          if (auto found = MARK_te->second->glyphs.find(glyph); found != MARK_te->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_te" ON None GLYPH te266 COMPONENT 1 AT  POS DX 1890 END_POS END_ANCHOR
          auto name = format("te2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          if (auto found = MARK_te->second->glyphs.find(glyph); found != MARK_te->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
    }
    if (MARK_bs != anchors.end() && MARK_be != anchors.end()) {
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_be" ON None GLYPH be66 COMPONENT 1 AT  POS DX 1890 DY -1860 END_POS END_ANCHOR
          // DEF_ANCHOR "MARK_bs" ON None GLYPH be66 COMPONENT 1 AT  POS DY -1860 END_POS END_ANCHOR
          auto name = format("be{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbe = MARK_be->second->glyphs.find(glyph); fbe != MARK_be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
          if (auto fbs = MARK_bs->second->glyphs.find(glyph); fbs != MARK_bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_be" ON None GLYPH be266 COMPONENT 1 AT  POS DX 1890 DY -1860 END_POS END_ANCHOR
          // DEF_ANCHOR "MARK_bs" ON None GLYPH be266 COMPONENT 1 AT  POS DY -1860 END_POS END_ANCHOR
          auto name = format("be2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          if (auto fbe = MARK_be->second->glyphs.find(glyph); fbe != MARK_be->second->glyphs.end()) {
            fbe->second = Vec<optional<int16_t>>(dx, dy);
          }
          if (auto fbs = MARK_bs->second->glyphs.find(glyph); fbs != MARK_bs->second->glyphs.end()) {
            fbs->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    // TODO: MARK_ti
    // TODO: ti
    // TODO: MARK_bi
    // TODO: bi
    if (auto MARK_center = anchors.find("MARK_center"); MARK_center != anchors.end()) {
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it12 COMPONENT 1 AT  POS DX 157 DY -310 END_POS END_ANCHOR
          auto name = format("it{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it11R COMPONENT 1 AT  POS DX -157 DY -155 END_POS END_ANCHOR
          auto name = format("it{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = -h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("it2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it211R COMPONENT 1 AT  POS DX -157 DY -155 END_POS END_ANCHOR
          auto name = format("it2{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = -h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH om11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("om{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH om211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("om2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH ti11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("ti{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH ti211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("ti2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH mi11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("mi{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH mi211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("mi2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH bi11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("bi{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH bi211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("bi2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH es11 COMPONENT 1 AT  POS DX 157 DY 155 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH df11 COMPONENT 1 AT  POS DX 157 DY 155 END_POS END_ANCHOR
          auto name = format("df{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (auto const &prefix : {"dq1234", "dq123", "dq124", "dq12", "dq134", "dq13", "dq14", "dq1", "dq234", "dq23", "dq24", "dq2", "dq34", "dq3", "dq4"}) {
        for (int h = 1; h <= hhu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_center" ON 1472 GLYPH dq1234_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1136 GLYPH dq123_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1280 GLYPH dq124_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 992 GLYPH dq12_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1376 GLYPH dq134_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1040 GLYPH dq13_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1184 GLYPH dq14_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 800 GLYPH dq1_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1424 GLYPH dq234_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1088 GLYPH dq23_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1232 GLYPH dq24_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 848 GLYPH dq2_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 1328 GLYPH dq34_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 896 GLYPH dq3_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_center" ON 944 GLYPH dq4_11 COMPONENT 1 AT  POS DY 155 END_POS END_ANCHOR
            auto name = format("{}_{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dy = v * vfu / 2;
            if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
              found->second = Vec<optional<int16_t>>(nullopt, dy);
            }
          }
        }
      }
      for (auto const &[n, sv] : sizeVariants) {
        // A1 = 5x6
        // DEF_ANCHOR "MARK_center" ON 1811 GLYPH A1 COMPONENT 1 AT  POS DY 930 END_POS END_ANCHOR
        {
          auto glyph = getGlyphByName(n);
          int16_t dy = sv.vGrids * vfu / 2;
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
        for (auto const &[key, glyph] : sv.variants) {
          // DEF_ANCHOR "MARK_center" ON 2888 GLYPH A1_11 COMPONENT 1 AT  POS DY 144 END_POS END_ANCHOR
          int v = key % 10;
          int16_t dy = v * vfu / 2; // TODO: this equation is not sure
          if (auto found = MARK_center->second->glyphs.find(glyph); found != MARK_center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    if (auto center = anchors.find("center"); center != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "center" ON None GLYPH o11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          if (auto found = center->second->glyphs.find(glyph); found != center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (auto const &prefix : {"s", "i"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "center" ON None GLYPH s11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH i11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            int16_t dy = -v * vfu / 2;
            if (auto found = center->second->glyphs.find(glyph); found != center->second->glyphs.end()) {
              found->second = Vec<optional<int16_t>>(dx, dy);
            }
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "center" ON None GLYPH es11 COMPONENT 1 AT  POS DX 157 DY 155 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu / 2;
          if (auto found = center->second->glyphs.find(glyph); found != center->second->glyphs.end()) {
            found->second = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (auto const &prefix : {"ts", "bs", "te", "be", "om", "mi", "ts2", "bs2", "te2", "be2", "om2", "mi2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "center" ON None GLYPH ts11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH bs11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH te11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH be11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH om11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH mi11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH ts211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH bs211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH te211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH be211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH om211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            // DEF_ANCHOR "center" ON None GLYPH mi211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            int16_t dy = -v * vfu / 2;
            if (auto found = center->second->glyphs.find(glyph); found != center->second->glyphs.end()) {
              found->second = Vec<optional<int16_t>>(dx, dy);
            }
          }
        }
      }
    }
    return Status::Ok();
  }

  Status compile(std::optional<size_t> maxNumLookup = std::nullopt) {
    using namespace std;

    if (maxNumLookup) {
      while (lookups.size() > *maxNumLookup) {
        lookups.pop_back();
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
    struct ConvertedLookups {
      vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> direct;
      vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> indirect;
    };
    map<shared_ptr<Lookup>, ConvertedLookups> convertedGposLookups;
    map<shared_ptr<Lookup>, ConvertedLookups> convertedGsubLookups;

    for (auto const &[name, lookup] : lookups) {
      // Determine if this is a GSUB or GPOS lookup
      bool isGsubLookup = !lookup->substitutions.empty();
      bool isGposLookup = lookup->adjustSingle || lookup->attach;

      if (!isGsubLookup && !isGposLookup) {
        continue; // Skip if neither GSUB nor GPOS
      }

      vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> converted;
      vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> indirect;
      if (auto st = convertLookup(lookup, converted, indirect); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }

      if (converted.empty()) {
        continue; // Skip if conversion failed
      }

      // Store in appropriate map
      if (isGsubLookup) {
        ranges::copy(converted, back_inserter(convertedGsubLookups[lookup].direct));
        ranges::copy(indirect, back_inserter(convertedGsubLookups[lookup].indirect));

        ranges::copy(converted, back_inserter(gsub->lookups));
        ranges::copy(indirect, back_inserter(gsub->lookups));
      } else {
        ranges::copy(converted, back_inserter(convertedGposLookups[lookup].direct));
        ranges::copy(indirect, back_inserter(convertedGposLookups[lookup].indirect));

        ranges::copy(converted, back_inserter(gpos->lookups));
        ranges::copy(indirect, back_inserter(gpos->lookups));
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
          vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> gposLookups;
          vector<shared_ptr<SubtableCollection<Subtable>::Lookup>> gsubLookups;

          // Process each Lookup
          for (auto const &lookup : feature->lookups) {
            // Add to GPOS if it's a GPOS lookup
            if (auto it = convertedGposLookups.find(lookup); it != convertedGposLookups.end()) {
              auto converted = it->second;
              ranges::copy(converted.direct, back_inserter(gposLookups));
            }

            // Add to GSUB if it's a GSUB lookup
            if (auto it = convertedGsubLookups.find(lookup); it != convertedGsubLookups.end()) {
              auto converted = it->second;
              ranges::copy(converted.direct, back_inserter(gsubLookups));
            }
          }

          // Add features to LangSys if they have lookups
          if (!gposLookups.empty()) {
            shared_ptr<SubtableCollection<Subtable>::Feature> gposFeature;

            if (auto it = gposFeatureMap.find(feature->tag); it != gposFeatureMap.end()) {
              gposFeature = it->second;
            } else {
              gposFeature = make_shared<SubtableCollection<Subtable>::Feature>();
              gposFeature->tag = feature->tag;
              auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();
              ranges::copy(gposLookups, back_inserter(featureData->lookups));
              gposFeature->data = featureData;
              gposFeatureMap[feature->tag] = gposFeature;
              gpos->features.push_back(gposFeature);
            }

            for (auto &lookup : gposFeature->data->lookups) {
              for (auto const &subtable : lookup->data->subtables) {
                auto sub = subtable;
                if (auto extension = dynamic_pointer_cast<gpos::PositioningExtension>(subtable); extension) {
                  sub = extension->extension;
                }
                if (auto chained = dynamic_pointer_cast<ChainedContexts>(sub); chained) {
                  if (auto st = chained->updateLookupToLookupListIndex(gpos->lookups); !st.ok()) {
                    return EGLYF_STATUS_PUSH(st);
                  }
                }
              }
            }

            gposLangSys->features.push_back(gposFeature);
          }

          if (!gsubLookups.empty()) {
            shared_ptr<SubtableCollection<Subtable>::Feature> gsubFeature;

            if (auto it = gsubFeatureMap.find(feature->tag); it != gsubFeatureMap.end()) {
              gsubFeature = it->second;
            } else {
              gsubFeature = make_shared<SubtableCollection<Subtable>::Feature>();
              gsubFeature->tag = feature->tag;
              auto featureData = make_shared<SubtableCollection<Subtable>::FeatureData>();
              ranges::copy(gsubLookups, back_inserter(featureData->lookups));
              gsubFeature->data = featureData;
              gsubFeatureMap[feature->tag] = gsubFeature;
              gsub->features.push_back(gsubFeature);
            }

            for (auto &lookup : gsubFeature->data->lookups) {
              for (auto const &subtable : lookup->data->subtables) {
                auto sub = subtable;
                if (auto extension = dynamic_pointer_cast<gsub::SubstitutionExtension>(subtable); extension) {
                  sub = extension->extension;
                }
                if (auto chained = dynamic_pointer_cast<ChainedContexts>(sub); chained) {
                  if (auto st = chained->updateLookupToLookupListIndex(gsub->lookups); !st.ok()) {
                    return EGLYF_STATUS_PUSH(st);
                  }
                }
              }
            }
            gsubLangSys->features.push_back(gsubFeature);
          }
        }

        // Add LangSys to Script if it has features
        if (!gposLangSys->features.empty()) {
          if (langSys->tag == FCC("dflt")) {
            gposScript.defaultLangSys = gposLangSys;
          } else {
            gposScript.langSysTable.push_back(make_pair(langSys->tag, gposLangSys));
          }
        }

        if (!gsubLangSys->features.empty()) {
          if (langSys->tag == FCC("dflt")) {
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
  // clang-format off
  template <class Extension, uint16_t ChainedContextLookupType /* 6 for GSUB, 8 for GPOS */>
  void addContextConditions(std::vector<
                              std::pair<
                                std::shared_ptr<SubtableCollection<Subtable>::Lookup>,
                                std::map<
                                  size_t,
                                  std::vector<std::shared_ptr<Coverage>>
                                >
                              >
                            > const &lookups,
                            std::vector<std::shared_ptr<Lookup::Context>> const &inContexts,
                            std::vector<std::shared_ptr<Lookup::Context>> const &exceptContexts,
                            std::vector<std::shared_ptr<Subtable>> &subtables) const {
    // clang-format on
    using namespace std;

    assert(!inContexts.empty() || !exceptContexts.empty());

    for (auto const &context : exceptContexts) {
      size_t const left = context->left.size();
      size_t const right = context->right.size();

      vector<std::shared_ptr<Coverage>> backtrackCoverage;
      for (size_t i = 0; i < left; i++) {
        size_t index = left - i - 1;
        set<uint16_t> glyphIds;
        auto const &item = context->left[index];
        collectGIDSet(item, glyphIds);
        if (!glyphIds.empty()) {
          backtrackCoverage.push_back(CoverageBuilder::Build(glyphIds));
        }
      }

      vector<shared_ptr<Coverage>> lookaheadCoverage;
      for (size_t i = 0; i < right; i++) {
        size_t index = right - i - 1;
        set<uint16_t> glyphIds;
        auto const &item = context->right[index];
        collectGIDSet(item, glyphIds);
        if (!glyphIds.empty()) {
          lookaheadCoverage.push_back(CoverageBuilder::Build(glyphIds));
        }
      }
      for (auto const &[lookup, inputCoverages] : lookups) {
        for (auto const &[numInput, inputCoverage] : inputCoverages) {
          auto negative = make_shared<ChainedContexts3>();
          negative->inputCoverage = inputCoverage;
          negative->backtrackCoverage = backtrackCoverage;
          negative->lookaheadCoverage = lookaheadCoverage;

          // Wrap with extension subtable
          auto extensionSubtable = make_shared<Extension>();
          extensionSubtable->extensionLookupType = ChainedContextLookupType;
          extensionSubtable->extension = negative;

          subtables.push_back(extensionSubtable);
        }
      }
    }

    if (inContexts.empty()) {
      for (auto const &[lookup, inputCoverages] : lookups) {
        for (auto const &[numInput, inputCoverage] : inputCoverages) {
          auto positive = make_shared<ChainedContexts3>();
          positive->inputCoverage = inputCoverage;

          SequenceLookup seqLookup;
          seqLookup.sequenceIndex = 0; // Replace the first input glyph
          seqLookup.lookup = lookup;
          positive->seqLookups.push_back(seqLookup);

          // Wrap with extension subtable
          auto extensionSubtable = make_shared<Extension>();
          extensionSubtable->extensionLookupType = ChainedContextLookupType;
          extensionSubtable->extension = positive;

          subtables.push_back(extensionSubtable);
        }
      }
    } else {
      for (auto const &context : inContexts) {
        size_t const left = context->left.size();
        size_t const right = context->right.size();

        vector<shared_ptr<Coverage>> backtrackCoverage;
        for (size_t i = 0; i < left; i++) {
          size_t index = left - i - 1;
          set<uint16_t> glyphIds;
          auto const &item = context->left[index];
          collectGIDSet(item, glyphIds);
          if (!glyphIds.empty()) {
            backtrackCoverage.push_back(CoverageBuilder::Build(glyphIds));
          }
        }

        vector<shared_ptr<Coverage>> lookaheadCoverage;
        for (size_t i = 0; i < right; i++) {
          set<uint16_t> glyphIds;
          auto const &item = context->right[i];
          collectGIDSet(item, glyphIds);
          if (!glyphIds.empty()) {
            lookaheadCoverage.push_back(CoverageBuilder::Build(glyphIds));
          }
        }

        for (auto const &[lookup, inputCoverages] : lookups) {
          for (auto const &[numInput, inputCoverage] : inputCoverages) {
            auto positive = make_shared<ChainedContexts3>();
            positive->inputCoverage = inputCoverage;
            positive->backtrackCoverage = backtrackCoverage;
            positive->lookaheadCoverage = lookaheadCoverage;

            SequenceLookup seqLookup;
            seqLookup.sequenceIndex = 0; // Replace the first input glyph
            seqLookup.lookup = lookup;
            positive->seqLookups.push_back(seqLookup);

            // Wrap with extension subtable
            auto extensionSubtable = make_shared<Extension>();
            extensionSubtable->extensionLookupType = ChainedContextLookupType;
            extensionSubtable->extension = positive;

            subtables.push_back(extensionSubtable);
          }
        }
      }
    }
  }

  // Convert Editor::Lookup base and marks to OpenType lookupFlag
  Optional<uint16_t> convertLookupFlag(std::variant<Lookup::SkipBase, Lookup::ProcessBase> const &base,
                                       std::variant<Lookup::SkipMarks, Lookup::ProcessMarks> const &marks,
                                       std::shared_ptr<GlyphDefinitionTable> const &gdef) {
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
      if (holds_alternative<Lookup::ProcessMarks::MarkFilteringSet>(processMarks.what)) {
        flag |= 0x0010; // Use mark filtering set
      } else if (holds_alternative<Lookup::ProcessMarks::MarkGroup>(processMarks.what)) {
        auto markGroup = get<Lookup::ProcessMarks::MarkGroup>(processMarks.what);
        auto classValue = determineMarkAttachmentClass(markGroup.group, gdef);
        if (!classValue) {
          return EGLYF_NULLOPT_PUSH(classValue.status());
        }
        flag |= (0xff00 & (*classValue << 8));
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

    auto coverage = CoverageBuilder::Build(glyphIds);

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
                                     std::shared_ptr<GlyphDefinitionTable> const &gdef) {
    using namespace std;
    if (!gdef || !holds_alternative<Lookup::ProcessMarks>(marks)) {
      return 0;
    }

    auto const &processMarks = get<Lookup::ProcessMarks>(marks);

    if (!holds_alternative<Lookup::ProcessMarks::MarkFilteringSet>(processMarks.what)) {
      return 0;
    }

    auto const &what = get<Lookup::ProcessMarks::MarkFilteringSet>(processMarks.what);

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
    collectGIDSetFromGroup(what.group, glyphIds);

    // Return 0 if no glyph IDs
    if (glyphIds.empty()) {
      return 0;
    }

    if (!markFilteringSets) {
      markFilteringSets = make_shared<map<set<uint16_t>, pair<shared_ptr<Coverage>, size_t>>>();

      for (size_t i = 0; i < gdef->markGlyphSets->coverages.size(); i++) {
        if (auto coverage1 = dynamic_pointer_cast<Coverage1>(gdef->markGlyphSets->coverages[i]); coverage1) {
          (*markFilteringSets)[coverage1->glyphArray] = make_pair(coverage1, i);
        } else if (auto coverage2 = dynamic_pointer_cast<Coverage2>(gdef->markGlyphSets->coverages[i]); coverage2) {
          set<uint16_t> coverage2GlyphIds;
          for (auto const &rangeRecord : coverage2->rangeRecords) {
            for (uint16_t glyphId = rangeRecord.startGlyphID; glyphId <= rangeRecord.endGlyphID; glyphId++) {
              coverage2GlyphIds.insert(glyphId);
            }
          }
          (*markFilteringSets)[coverage2GlyphIds] = make_pair(coverage2, i);
        }
      }
    }

    if (auto found = markFilteringSets->find(glyphIds); found == markFilteringSets->end()) {
      auto coverage = CoverageBuilder::Build(glyphIds);
      auto index = gdef->markGlyphSets->coverages.size();
      gdef->markGlyphSets->coverages.push_back(coverage);
      (*markFilteringSets)[glyphIds] = make_pair(coverage, index);
      return index;
    } else {
      return found->second.second;
    }
  }

  Optional<uint16_t> determineMarkAttachmentClass(std::shared_ptr<Group> const &group, std::shared_ptr<GlyphDefinitionTable> const &gdef) {
    using namespace std;
    if (!markAttachClasses) {
      markAttachClasses = make_shared<unordered_map<uint16_t, uint16_t>>();
      if (gdef->markAttachClassDef) {
        gdef->markAttachClassDef->enumerateClassValues([this](uint16_t gid, uint16_t classValue) {
          (*markAttachClasses)[gid] = classValue;
        });
      } else {
        gdef->markAttachClassDef = make_shared<ClassDef2>();
      }
    }
    set<uint16_t> gids;
    collectGIDSet(group, gids);
    if (gids.empty()) {
      return EGLYF_NULLOPT_WHAT("No glyph IDs found in the group");
    }
    uint16_t first = *gids.begin();
    auto found = markAttachClasses->find(first);
    if (found == markAttachClasses->end()) {
      uint16_t maxClassValue = 0;
      for (auto const &[gid, cv] : *markAttachClasses) {
        if (gids.find(gid) != gids.end()) {
          return EGLYF_NULLOPT_WHAT("Failed to determine mark attachment class");
        }
        maxClassValue = (std::max)(maxClassValue, cv);
      }
      uint16_t classValue = maxClassValue + 1;
      if (classValue > (uint16_t)numeric_limits<uint8_t>::max()) {
        return EGLYF_NULLOPT_WHAT("Class value exceeds maximum allowed value");
      }
      for (auto gid : gids) {
        (*markAttachClasses)[gid] = classValue;
        if (auto st = gdef->markAttachClassDef->add(gid, classValue); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      }
      return classValue;
    } else {
      uint16_t classValue = found->second;
      if (classValue > (uint16_t)numeric_limits<uint8_t>::max()) {
        return EGLYF_NULLOPT_WHAT("Class value exceeds maximum allowed value");
      }
      for (auto const &[gid, cv] : *markAttachClasses) {
        if (cv != classValue && gids.find(gid) != gids.end()) {
          return EGLYF_NULLOPT_WHAT("Inconsistent class values for glyphs in the same group");
        }
      }
      return classValue;
    }
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
  template <class C>
    requires requires(C &container, std::shared_ptr<Glyph> const &g) {
      container.push_back(g);
    }
  void collectGlyphVector(GG const &item,
                          C &glyphs) const {
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
  template <class C>
    requires requires(C &container, std::shared_ptr<Glyph> const &g) {
      container.push_back(g);
    }
  void collectGlyphVectorFromGroup(std::shared_ptr<Group> const &group,
                                   C &glyphs) const {
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

  // Determine if a glyph is a mark glyph
  bool isMarkGlyph(std::shared_ptr<Glyph> const &glyph) const {
    return glyph->classDef == GlyphDefinitionTable::Class::Mark;
  }

  // Determine if all glyphs in a variant are mark glyphs
  void countGlyphType(GG const &item, size_t &base, size_t &mark) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (glyph->id) {
        if (isMarkGlyph(glyph)) {
          mark++;
        } else {
          base++;
        }
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
        if (glyph->id) {
          if (isMarkGlyph(glyph)) {
            mark++;
          } else {
            base++;
          }
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        countGlyphTypeInGroup(subgroup, base, mark);
      }
    }
  }

  // Create attachment subtable (MarkToBase or MarkToMark) from Editor::Lookup::Attach
  Status createAttachmentSubtable(std::shared_ptr<Lookup> const &lookup, std::shared_ptr<Subtable> &result, uint16_t &lookupType) {
    using namespace std;
    using ClassId = uint16_t;

    if (!lookup->attach || lookup->attach->ligands.empty()) {
      return Status::Ok();
    }

    size_t receptorBase = 0;
    size_t receptorMark = 0;
    for (auto const &receptor : lookup->attach->receptors) {
      countGlyphType(receptor, receptorBase, receptorMark);
    }

    size_t ligandBase = 0;
    size_t ligandMark = 0;
    for (auto const &item : lookup->attach->ligands) {
      countGlyphType(item.ligand, ligandBase, ligandMark);
    }

    if (receptorBase * ligandBase != 0) {
      return EGLYF_ERROR_WHAT("Both receptor and ligand cannot be base glyphs simultaneously");
    }
    if (ligandBase != 0) {
      return EGLYF_ERROR_WHAT("Ligand must be a mark glyph, not a base glyph");
    }
    if (receptorBase * receptorMark != 0) {
      return EGLYF_ERROR_WHAT("Receptor cannot be both base and mark glyphs simultaneously");
    }
    if (ligandBase * ligandMark != 0) {
      return EGLYF_ERROR_WHAT("Ligand cannot be both base and mark glyphs simultaneously");
    }

    if (receptorBase > 0 && ligandMark > 0) {
      // mark
      lookupType = 4;

      vector<shared_ptr<Glyph>> receptorGlyphs;
      set<uint16_t> receptorGlyphIds;
      for (auto const &receptor : lookup->attach->receptors) {
        collectGlyphVector(receptor, receptorGlyphs);
      }
      for (auto const &glyph : receptorGlyphs) {
        if (glyph->id) {
          receptorGlyphIds.insert(*glyph->id);
        }
      }
      if (receptorGlyphs.empty()) {
        return Status::Ok();
      }

      set<uint16_t> markCoverage;

      ClassId nextMarkClass = 0;
      map<shared_ptr<Anchor>, ClassId> anchors;
      deque<pair<shared_ptr<Glyph>, shared_ptr<Anchor>>> ligandGlyphs;
      for (auto const &item : lookup->attach->ligands) {
        if (auto found = anchors.find(item.anchor); found == anchors.end()) {
          anchors[item.anchor] = nextMarkClass;
          nextMarkClass++;
        }
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(item.ligand, glyphs);
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            ligandGlyphs.push_back(make_pair(glyph, item.anchor));
            markCoverage.insert(*glyph->id);
          }
        }
      }
      ranges::sort(ligandGlyphs, [](auto const &a, auto const &b) { return a.first->id < b.first->id; });

      auto mark = make_unique<gpos::MarkToBaseAttachment>();
      mark->markCoverage = CoverageBuilder::Build(markCoverage);
      mark->baseCoverage = CoverageBuilder::Build(receptorGlyphIds);

      for (auto const &[ligand, anchor] : ligandGlyphs) {
        auto found = anchors.find(anchor);
        if (found == anchors.end()) [[unlikely]] {
          return EGLYF_ERROR_WHAT("Anchor not found in anchors map");
        }
        gpos::MarkRecord record;
        record.markClass = found->second;
        auto gposAnchor = make_shared<gpos::Anchor1>();
        gposAnchor->xCoordinate = 0;
        gposAnchor->yCoordinate = 0;
        record.markAnchor = gposAnchor;
        mark->markArray.markRecords.push_back(record);
      }

      for (auto const &receptor : receptorGlyphs) {
        gpos::MarkToBaseAttachment::BaseRecord record;
        record.baseAnchors.resize(nextMarkClass, nullptr);

        for (auto const &[anchor, classId] : anchors) {
          auto found = anchor->glyphs.find(receptor);
          if (found == anchor->glyphs.end()) [[unlikely]] {
            return EGLYF_ERROR_WHAT("Receptor glyph not found in anchor glyphs map");
          }
          auto gposAnchor = make_shared<gpos::Anchor1>();
          gposAnchor->xCoordinate = found->second.x.value_or(0);
          gposAnchor->yCoordinate = found->second.y.value_or(0);
          record.baseAnchors[classId] = gposAnchor;
        }
        mark->baseArray.baseRecords.push_back(record);
      }
      mark->baseArray.markClassCount = nextMarkClass;

      result.reset(mark.release());
      return Status::Ok();
    } else if (receptorMark > 0 && ligandMark > 0) {
      // mkmk
      lookupType = 6;

      // mark2: receptor
      // mark: ligand

      deque<shared_ptr<Glyph>> receptorGlyphs;
      set<uint16_t> receptorGlyphIds;
      for (auto const &receptor : lookup->attach->receptors) {
        collectGlyphVector(receptor, receptorGlyphs);
      }
      for (auto const &glyph : receptorGlyphs) {
        if (glyph->id) {
          receptorGlyphIds.insert(*glyph->id);
        }
      }
      if (receptorGlyphs.empty()) {
        return Status::Ok();
      }
      ranges::sort(receptorGlyphs, [](auto const &a, auto const &b) { return *a->id < *b->id; });

      set<uint16_t> markCoverage;

      ClassId nextMarkClass = 0;
      map<shared_ptr<Anchor>, ClassId> anchors;
      deque<pair<shared_ptr<Glyph>, shared_ptr<Anchor>>> ligandGlyphs;
      for (auto const &item : lookup->attach->ligands) {
        if (auto found = anchors.find(item.anchor); found == anchors.end()) {
          anchors[item.anchor] = nextMarkClass;
          nextMarkClass++;
        }
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(item.ligand, glyphs);
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            ligandGlyphs.push_back(make_pair(glyph, item.anchor));
            markCoverage.insert(*glyph->id);
          }
        }
      }
      ranges::sort(ligandGlyphs, [](auto const &a, auto const &b) { return a.first->id < b.first->id; });

      auto mark = make_unique<gpos::MarkToMarkAttachmentPositioning>();
      mark->mark1Coverage = CoverageBuilder::Build(markCoverage);
      mark->mark2Coverage = CoverageBuilder::Build(receptorGlyphIds);

      for (auto const &[ligand, anchor] : ligandGlyphs) {
        auto found = anchors.find(anchor);
        if (found == anchors.end()) [[unlikely]] {
          return EGLYF_ERROR_WHAT("Anchor not found in anchors map");
        }
        gpos::MarkRecord record;
        record.markClass = found->second;
        auto gposAnchor = make_shared<gpos::Anchor1>();
        auto markAnchor = getAnchorByName("MARK_" + anchor->name);
        if (auto f = markAnchor->glyphs.find(ligand); f == markAnchor->glyphs.end()) {
          gposAnchor->xCoordinate = 0;
          gposAnchor->yCoordinate = 0;
        } else {
          gposAnchor->xCoordinate = f->second.x.value_or(0);
          gposAnchor->yCoordinate = f->second.y.value_or(0);
        }
        record.markAnchor = gposAnchor;
        mark->mark1Array.markRecords.push_back(record);
      }

      for (auto const &receptor : receptorGlyphs) {
        gpos::MarkToMarkAttachmentPositioning::Mark2 record;
        record.mark2Anchors.resize(nextMarkClass, nullptr);

        for (auto const &[anchor, classId] : anchors) {
          auto found = anchor->glyphs.find(receptor);
          auto gposAnchor = make_shared<gpos::Anchor1>();
          if (found == anchor->glyphs.end()) [[unlikely]] {
            gposAnchor->xCoordinate = 0;
            gposAnchor->yCoordinate = 0;
          } else {
            gposAnchor->xCoordinate = found->second.x.value_or(0);
            gposAnchor->yCoordinate = found->second.y.value_or(0);
          }
          record.mark2Anchors[classId] = gposAnchor;
        }
        mark->mark2Array.mark2Records.push_back(record);
      }
      mark->mark2Array.markClassCount = nextMarkClass;

      result.reset(mark.release());
      return Status::Ok();
    } else {
      return EGLYF_ERROR_WHAT("Invalid combination of receptor and ligand glyph types");
    }
  }

public:
  static int constexpr hhu = 8;
  static int constexpr vhu = 6;
  static int constexpr chu = 6;

  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<uint16_t, std::shared_ptr<Glyph>> glyphsLut;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::deque<std::pair<std::string, std::shared_ptr<Lookup>>> lookups;
  std::unordered_map<std::string, std::shared_ptr<Script>> scripts;

  std::shared_ptr<std::map<std::set<uint16_t>, std::pair<std::shared_ptr<Coverage>, size_t>>> markFilteringSets;
  std::shared_ptr<std::unordered_map<uint16_t, uint16_t>> markAttachClasses;

  // unit per horizontal grid
  int16_t hfu;
  // grid width = hg0 + n * hg; where 1 <= n <= kHGrids
  int16_t hg0;
  // unit per vertical grid
  int16_t vfu;
  // grid height = vg0 + n * vg; where 1 <= n <= kVGrids
  int16_t vg0;
  std::unordered_map<std::string, SizeVariants> sizeVariants;
  // font side bearings: hfu / 3
  int16_t sb;
};

} // namespace eglyf
