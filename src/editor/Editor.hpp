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

public:
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
  }

  std::shared_ptr<Glyph> getGlyphByName(std::string const &name) {
    using namespace std;
    if (auto found = glyphs.find(name); found == glyphs.end()) {
      auto g = make_shared<Glyph>();
      g->name = name;
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

  Status compile(std::optional<std::string> onlyLookupWithName = std::nullopt) {
    using namespace std;

    if (onlyLookupWithName) {
      auto found = ranges::find_if(lookups, [&onlyLookupWithName](auto const &it) { return it.first == *onlyLookupWithName; });
      if (found == lookups.end()) {
        lookups.clear();
      } else {
        auto lookup = found->second;
        lookups.clear();
        lookups.push_back(*found);
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
        gdef->markAttachClassDef = make_shared<ClassDef1>();
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
  std::shared_ptr<FontFile> font;
  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<uint16_t, std::shared_ptr<Glyph>> glyphsLut;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::deque<std::pair<std::string, std::shared_ptr<Lookup>>> lookups;
  std::unordered_map<std::string, std::shared_ptr<Script>> scripts;

  std::shared_ptr<std::map<std::set<uint16_t>, std::pair<std::shared_ptr<Coverage>, size_t>>> markFilteringSets;
  std::shared_ptr<std::unordered_map<uint16_t, uint16_t>> markAttachClasses;
};

} // namespace eglyf
