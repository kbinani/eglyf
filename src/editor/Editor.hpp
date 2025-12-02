#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
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

    void clear() {
      substitutions.clear();
      adjustSingle = nullptr;
      attach = nullptr;
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
    std::shared_ptr<Feature> requiredFeature;

    LangSys(std::string const &name, Tag const &tag) : name(name), tag(tag) {}
  };

  struct Script {
    std::string name;
    Tag tag;
    std::vector<std::shared_ptr<LangSys>> langSysList;

    Script(std::string const &name, Tag const &tag) : name(name), tag(tag) {}
  };

public:
  Editor(std::shared_ptr<Font> const &font, Config cfg) : font(font), cfg(cfg) {
  }

  std::shared_ptr<Glyph> getGlyphByName(std::string const &name) {
    using namespace std;
    if (auto found = glyphs.find(name); found == glyphs.end()) {
      auto g = make_shared<Glyph>();
      g->name = name;
      auto gid = font->postGetGlyphID(name);
      if (gid) {
        g->id = *gid;
      }
      glyphs[name] = g;
      return g;
    } else {
      return found->second;
    }
  }

  std::shared_ptr<Glyph> getGlyphByID(uint16_t gid) {
    using namespace std;
    if (auto found = glyphsLut.find(gid); found != glyphsLut.end()) {
      return found->second;
    }
    if (auto name = font->postGetName(gid); name) {
      auto g = getGlyphByName(*name);
      g->id = gid;
      glyphsLut[gid] = g;
      return g;
    }
    if (auto found = ranges::find_if(glyphs, [=](pair<string, shared_ptr<Glyph>> const &it) { return it.second->id == gid; }); found != glyphs.end()) {
      glyphsLut[gid] = found->second;
      return found->second;
    }
    return nullptr;
  }

  std::shared_ptr<Group> getGroupByName(std::string const &name) {
    using namespace std;
    if (auto found = groups.find(name); found == groups.end()) {
      auto g = make_shared<Group>();
      g->name = name;
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
      a->name = name;
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
                       std::vector<std::shared_ptr<SubtableCollection::Lookup>> &result,
                       std::vector<std::shared_ptr<SubtableCollection::Lookup>> &indirect) {
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
                           std::vector<std::shared_ptr<SubtableCollection::Lookup>> &result,
                           std::vector<std::shared_ptr<SubtableCollection::Lookup>> &indirect) {
    using namespace std;

    // clang-format off
    vector<
      pair<
        shared_ptr<SubtableCollection::Lookup>,
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

        auto lookupData = make_shared<SubtableCollection::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 9; // Extension Positioning
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection::Lookup>();
        gposLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverage;
        auto coverage = make_shared<Coverage>();
        for (auto const &adjust : lookup->adjustSingle->glyphs) {
          if (adjust.glyph->id) {
            coverage->insert(*adjust.glyph->id);
          }
        }
        inputCoverage[1].push_back(coverage);

        lookups.push_back(make_pair(gposLookup, inputCoverage));
      }
    }
    if (lookup->attach) {
      vector<pair<shared_ptr<Subtable>, shared_ptr<Coverage>>> subtables;
      uint16_t lookupType = 0;
      if (auto st = createAttachmentSubtable(lookup, subtables, lookupType); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      for (auto const &it : subtables) {
        shared_ptr<Subtable> subtable = it.first;
        shared_ptr<Coverage> receptorCoverage = it.second;

        auto extensionSubtable = make_shared<gpos::PositioningExtension>();
        extensionSubtable->extensionLookupType = lookupType;
        extensionSubtable->extension = subtable;

        auto lookupData = make_shared<SubtableCollection::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 9; // Extension Positioning
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto gposLookup = make_shared<SubtableCollection::Lookup>();
        gposLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        inputCoverages[1].push_back(receptorCoverage);

        lookups.push_back(make_pair(gposLookup, inputCoverages));
      }
    }

    if (lookups.empty()) {
      return Status::Ok();
    }

    auto lookupData = make_shared<SubtableCollection::LookupData>();
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
    auto gposLookup = make_shared<SubtableCollection::Lookup>();
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
    vector<uint16_t> coverageGlyphIDs;

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
      vector<uint16_t> inputGlyphIDs;
      vector<uint16_t> outputGlyphIDs;
      bool ok = true;
      for (auto const &g : inputGlyphs) {
        if (!g->id) {
          ok = false;
          break;
        }
        inputGlyphIDs.push_back(*g->id);
      }
      if (!ok) {
        continue;
      }
      for (auto const &g : outputGlyphs) {
        if (!g->id) {
          ok = false;
          break;
        }
        outputGlyphIDs.push_back(*g->id);
      }
      if (!ok) {
        continue;
      }

      // Create glyph ID mapping
      if (inputGlyphIDs.size() == 1 && outputGlyphIDs.size() == 1) {
        // Single glyph to single glyph substitution
        uint16_t inputGlyphID = inputGlyphIDs[0];
        uint16_t outputGlyphID = outputGlyphIDs[0];

        // Add to Coverage if not already processed
        if (glyphMap.find(inputGlyphID) == glyphMap.end()) {
          coverageGlyphIDs.push_back(inputGlyphID);
        }

        glyphMap[inputGlyphID] = outputGlyphID;
      } else if (inputGlyphIDs.size() > 1 && outputGlyphIDs.size() == 1) {
        // Group to single glyph substitution
        uint16_t outputGlyphID = outputGlyphIDs[0];

        for (auto inputGlyphID : inputGlyphIDs) {
          // Add to Coverage if not already processed
          if (glyphMap.find(inputGlyphID) == glyphMap.end()) {
            coverageGlyphIDs.push_back(inputGlyphID);
          }

          glyphMap[inputGlyphID] = outputGlyphID;
        }
      } else if (inputGlyphIDs.size() > 1 && outputGlyphIDs.size() > 1) {
        // Group to group substitution (when glyph counts match)
        for (size_t i = 0; i < inputGlyphIDs.size(); ++i) {
          uint16_t inputGlyphID = inputGlyphIDs[i];
          uint16_t outputGlyphID = outputGlyphIDs[i];

          // Add to Coverage if not already processed
          if (glyphMap.find(inputGlyphID) == glyphMap.end()) {
            coverageGlyphIDs.push_back(inputGlyphID);
          }

          glyphMap[inputGlyphID] = outputGlyphID;
        }
      }
    }

    // Do nothing if the glyph map is empty
    if (glyphMap.empty()) {
      return Status::Ok();
    }

    auto coverage = make_shared<Coverage>();
    for (auto glyphID : coverageGlyphIDs) {
      coverage->insert(glyphID);
    }

    // Check if all substitutions have the same deltaGlyphID
    bool allSameDelta = true;
    int16_t firstDelta = static_cast<int16_t>(glyphMap[coverageGlyphIDs[0]]) - static_cast<int16_t>(coverageGlyphIDs[0]);

    for (auto glyphID : coverageGlyphIDs) {
      int16_t delta = static_cast<int16_t>(glyphMap[glyphID]) - static_cast<int16_t>(glyphID);
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
      for (auto glyphID : coverage->glyphIDs) {
        format2->substituteGlyphIDs.push_back(glyphMap[glyphID]);
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
        uint16_t inputGlyphID = *inputGlyph->id;
        vector<uint16_t> outputGlyphIDs;
        for (auto const &output : outputs) {
          if (holds_alternative<shared_ptr<Group>>(output)) {
            return EGLYF_ERROR_WHAT("Output contains a group which is not supported for single glyph input");
          }
          auto outputGlyph = get<shared_ptr<Glyph>>(output);
          if (outputGlyph->id) {
            outputGlyphIDs.push_back(*outputGlyph->id);
          } else {
            break;
          }
        }
        if (outputGlyphIDs.size() == outputs.size()) {
          mapping[inputGlyphID] = outputGlyphIDs;
        }
      } else {
        return EGLYF_ERROR_WHAT("Invalid variant type in input");
      }
    }

    // Return if mapping is empty
    if (mapping.empty()) {
      return Status::Ok();
    }

    auto coverageGlyphIDs = make_shared<Coverage>();
    for (auto const &[inputGlyphID, outputGlyphIDs] : mapping) {
      coverageGlyphIDs->insert(inputGlyphID);
    }

    // Create Sequence objects for each input glyph ID
    vector<gsub::Multiple::Sequence> sequences;

    for (auto glyphID : coverageGlyphIDs->glyphIDs) {
      gsub::Multiple::Sequence sequence;
      sequence.substituteGlyphIDs = mapping[glyphID];
      sequences.push_back(sequence);
    }

    // Create gsub::Multiple object and set coverage and sequences
    auto multiple = make_shared<gsub::Multiple>();
    multiple->coverage = coverageGlyphIDs;
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
          if (ok) {
            mapping.push_back(make_pair(in, *outGlyph->id));
          }
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
      uint16_t firstGlyphID = components[0];
      groupedMapping[firstGlyphID].push_back(make_pair(components, ligatureGlyph));
    }

    // Create Coverage table with first glyph IDs
    auto coverage = make_shared<Coverage>();
    for (auto const &[firstGlyphID, _] : groupedMapping) {
      coverage->insert(firstGlyphID);
    }

    // Create Ligature object and set coverage
    auto ligature = make_shared<gsub::Ligature>();
    ligature->coverage = coverage;

    // Create LigatureSet for each first glyph ID
    for (auto const &[firstGlyphID, mappings] : groupedMapping) {
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
                           std::vector<std::shared_ptr<SubtableCollection::Lookup>> &result,
                           std::vector<std::shared_ptr<SubtableCollection::Lookup>> &indirect) {
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
        shared_ptr<SubtableCollection::Lookup>,
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

        auto lookupData = make_shared<SubtableCollection::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto singleLookup = make_shared<SubtableCollection::Lookup>();
        singleLookup->data = lookupData;

        auto inputGlyphIDs = make_shared<Coverage>();
        for (auto const &[input, _] : single) {
          collectGIDSet(input, *inputGlyphIDs);
        }
        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        inputCoverages[1].push_back(inputGlyphIDs);

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

        auto lookupData = make_shared<SubtableCollection::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto multipleLookup = make_shared<SubtableCollection::Lookup>();
        multipleLookup->data = lookupData;

        auto inputGlyphIDs = make_shared<Coverage>();
        for (auto const &[input, _] : multiple) {
          collectGIDSet(input, *inputGlyphIDs);
        }
        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        inputCoverages[1].push_back(inputGlyphIDs);

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

        auto lookupData = make_shared<SubtableCollection::LookupData>();
        lookupData->name = lookup->name;
        lookupData->lookupType = 7; // Extension Substitution
        auto lookupFlag = convertLookupFlag(lookup->base, lookup->marks, font->gdef);
        if (!lookupFlag) {
          return EGLYF_STATUS_PUSH(lookupFlag.status());
        }
        lookupData->lookupFlag = *lookupFlag;
        lookupData->markFilteringSet = determineMarkFilteringSet(lookup->marks, font->gdef);
        lookupData->subtables.push_back(extensionSubtable);

        auto ligatureLookup = make_shared<SubtableCollection::Lookup>();
        ligatureLookup->data = lookupData;

        map<size_t, vector<shared_ptr<Coverage>>> inputCoverages;
        for (auto const &[inputs, _] : ligature) {
          auto &glyphs = inputCoverages[inputs.size()];
          for (size_t i = 0; i < inputs.size(); i++) {
            auto const &input = inputs[i];
            auto cov = make_shared<Coverage>();
            collectGIDSet(input, *cov);
            glyphs.push_back(cov);
          }
        }

        lookups.push_back(make_pair(ligatureLookup, inputCoverages));
      }
    }

    if (lookups.empty()) {
      return Status::Ok();
    }

    auto lookupData = make_shared<SubtableCollection::LookupData>();
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
    auto gsubLookup = make_shared<SubtableCollection::Lookup>();
    gsubLookup->data = lookupData;

    result.push_back(gsubLookup);

    return Status::Ok();
  }

  Status createSizeVariants() {
    using namespace std;

    map<WxH, vector<WxH>> variationChain;
    variationChain[76] = {75, 56, 55, 45, 44, 33, 22, 11};
    variationChain[66] = {55, 44, 33, 22, 11};
    variationChain[65] = {55, 54, 45, 44, 32, 21, 11};
    variationChain[64] = {62, 54, 52, 43, 42, 32, 22, 11};
    variationChain[63] = {62, 61, 43, 33, 32, 31, 21, 11};
    variationChain[62] = {61, 52, 51, 42, 32, 31, 21, 11};
    variationChain[61] = {51, 41, 31, 21, 11};
    variationChain[56] = {45, 44, 35, 33, 22, 11};
    variationChain[55] = {54, 45, 44, 43, 34, 33, 22, 11};
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
    variationChain[32] = {31, 21, 11};
    variationChain[26] = {25, 24, 23, 16, 14, 13, 12, 11};
    variationChain[25] = {24, 15, 14, 11};
    variationChain[24] = {23, 14, 13, 12, 11};
    variationChain[23] = {13, 12, 11};
    variationChain[22] = {11};
    variationChain[21] = {11};
    variationChain[16] = {15, 14, 13, 12, 11};
    variationChain[12] = {11};

    // n
    static set<string> const rotate90 = {
        // clang-format off
        "A1", "D27", "F16", "F28", "F32", "F37b", "F51", "J11", "J12", "J21",
        "J8", "K6", "M10", "M17", "M3", "M9", "N11", "N12", "O31", "O36",
        "O39", "Q3", "R24", "S10", "S18", "T1", "T16", "T22", "U7", "U8",
        "V10", "V11", "V26", "V27", "X4a", "Z10", "Z11", "Z7",
        // clang-format on
    };
    // o
    static set<string> const rotate180 = {
        // clang-format off
        "A1", "D28", "H8", "J11", "M3", "M44", "N10", "N11", "N12", "O31",
        "O6", "P8", "T10", "T16", "T2", "T21", "T22", "T35", "T9", "T9a",
        // clang-format on
    };
    // t
    static set<string> const rotate270 = {
        // clang-format off
        "A1", "F23", "F51", "H5", "J11", "J30", "J32", "K6", "M44", "M72",
        "N35", "O29", "P8", "S18", "S20", "S33", "S37", "S42", "S43", "T10",
        "T16a", "T21", "T35", "T8", "T8a", "T9", "T9a", "U22", "U42", "V12a",
        "V19", "V7a", "W1", "W14", "W2", "Y2",
        // clang-format on
    };

    deque<pair<WxH, float>> sizeList;
    for (auto const &[key, _] : variationChain) {
      sizeList.push_back(make_pair(key, AreaFromWxH(key)));
    }
    ranges::stable_sort(sizeList, [](auto const &a, auto const &b) { return a.second < b.second; });

    map<uint16_t, pair<uint32_t, Rect<int16_t>>> bounds;

    if (!holds_alternative<Font::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto &outline = get<Font::TrueTypeOutlines>(font->outlines);
    auto &glyf = outline.glyf;
    auto const process = [&, this](uint32_t cp) {
      auto gid = font->cmap->getGlyphID(cp);
      if (!gid) {
        return;
      }
      auto g = glyf->glyphs[*gid];
      auto b = glyf::GlyphDataTable::Bounds(g);
      if (b) {
        bounds[*gid] = make_pair(cp, *b);
      }
    };
    Unicode::EnumerateHieroglyphUnicode(process);

    if (bounds.empty()) {
      return EGLYF_ERROR;
    }

    uint64_t sumWidth = 0;
    int64_t sumTop = 0;
    int64_t sumBottom = 0;
    uint64_t sumCount = 0;
    for (auto const &name : {"A7", "A10", "A14", "A14a", "A37", "A38", "A39", "A52", "A64", "A70", "C11", "D30", "D33", "D34", "D34a", "D50c", "D52a", "D57", "D59", "D67h", "E1", "E3", "E4", "E6", "E7", "E8", "E8a", "E14", "E15", "E16", "E16a", "E17", "E17a", "E18", "E19", "E20", "E26", "E28", "E28a", "E30", "E31", "E37", "F6", "F14", "F15", "F40", "F50", "G1", "G2", "G3", "G4", "G5", "G6", "G6a", "G7a", "G10", "G11a", "G13", "G14", "G15", "G17", "G19", "G20", "G20a", "G21", "G23", "G25", "G26a", "G29", "G31", "G32", "G33", "G38", "G39", "G40", "G41", "G44", "G45", "G45a", "G47", "G51", "G53", "G54", "I1", "I4", "I10", "I10a", "I11", "I11a", "K7", "L2", "L2a", "M1a", "M1b", "M3a", "M9", "M12b", "M14", "M20", "M22a", "M27", "M42", "M43", "N2", "N13", "N14", "N35a", "NU5", "NU11", "NU17", "O1a", "O2", "O8", "O9", "O10", "O10a", "O10b", "O12", "O13", "O14", "O15", "O18", "O19", "O19a", "O22", "O23", "O27", "P5", "P7", "P9", "P10", "R1", "R2", "R3", "R10", "R26", "S2", "S4", "S6", "S7", "S13", "S14", "S14a", "S14b", "S15", "S28", "S30", "S31", "T5", "T6", "T32a", "T33a", "U1", "U4", "U5", "U35", "U38", "V1d", "V1e", "V1f", "V1g", "V2a", "V4", "V20h", "V21", "V28a", "V81", "W4", "W14a", "W17a", "W18", "W18a", "J22", "O13a"}) {
      auto cp = GlyphNames::GetCodepoint(name);
      if (!cp) {
        continue;
      }
      auto found = ranges::find_if(bounds, [&](auto const &it) { return it.second.first == *cp; });
      if (found == bounds.end()) {
        continue;
      }
      Rect<int16_t> rect = found->second.second;
      sumWidth += rect.width();
      sumTop += rect.yMax;
      sumBottom += rect.yMin;
      sumCount++;
    }

    int averageTop = (int)round(sumTop / (double)sumCount);
    int averageBottom = (int)round(sumBottom / (double)sumCount);
    int height = averageTop - averageBottom;
    int width = (int)round(sumWidth / (double)sumCount);
    int scale = min(height, width);
    lineWidth = (int16_t)max(1, scale / 32);
    int margin = lineWidth / 2;
    int h = height - 8 * lineWidth - 2 * margin;
    double s = h / (double)height;
    vfu = (int16_t)round((h + 2 * margin) / (double)vhu);
    hfu = (int16_t)round(width * s / chu);
    base = averageBottom + 4 * lineWidth + margin;
    sb = 0;

    struct BaseGlyph {
      WxH size;
      uint16_t gid;
      shared_ptr<Glyph> glyph;
      Rect<int16_t> bounds;
      uint32_t codepoint;
    };
    map<string, BaseGlyph> baseGlyphs;

    auto const decideSize = [this, &variationChain, &sizeList](int margin, double s, int width, int height) -> WxH {
      double s2 = min({1.0, (chu * hfu - 2 * margin) / (width * s), (vhu * vfu - 2 * margin) / (height * s)});
      int hGrids = clamp((int)ceil((width * s * s2 + 2 * margin) / (double)hfu), 1, (int)chu);
      int vGrids = clamp((int)ceil((height * s * s2 + 2 * margin) / (double)vfu), 1, (int)vhu);

      auto chain = variationChain.find(hGrids * 10 + vGrids);
      if (chain != variationChain.end()) {
        return hGrids * 10 + vGrids;
      }
      auto first = ranges::find_if(sizeList, [=](auto const &it) {
        return it.second >= hGrids * vGrids;
      });
      if (first == sizeList.end()) {
        return hGrids * 10 + vGrids;
      }
      auto index = distance(sizeList.begin(), first);
      for (size_t i = index; i < sizeList.size(); i++) {
        auto const &it = sizeList[i];
        WxH key = it.first;
        int h = WidthFromWxH(key);
        int v = HeightFromWxH(key);
        if (h < hGrids || v < vGrids) {
          continue;
        }
        chain = variationChain.find(key);
        if (chain != variationChain.end()) {
          hGrids = h;
          vGrids = v;
          break;
        }
      }
      return hGrids * 10 + vGrids;
    };

    auto const createBaseGlyph = [this, &glyf](string const &name,
                                               WxH size,
                                               uint32_t codepoint,
                                               uint16_t gid,
                                               Rect<int16_t> const &r,
                                               Transform<int16_t> const &pre,
                                               shared_ptr<BaseGlyph> &out) -> Status {
      int hGrids = WidthFromWxH(size);
      int vGrids = HeightFromWxH(size);

      Rect<int16_t> rect;
      if (pre.isIdentity()) {
        rect = r;
      } else {
        rect = r.transformed(pre);
      }

      // https://gyazo.com/a419984f058fbf8655e2e30abbac091e
      SizeVariants::Resize op = SizeVariants::Transform(rect, hGrids, vGrids, hfu, vfu, base, lineWidth);
      glyf::GlyphDataTable::CompositeGlyph::GlyphRecord record;
      if (pre.isIdentity()) {
        if (op.scale < 1) {
          record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, op.dx, op.dy, op.scale);
        } else {
          record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, op.dx, op.dy);
        }
      } else {
        Transform<float> left(op.scale, 0, 0, op.scale, op.dx, op.dy);
        Transform<float> right(pre.xscale, pre.scale10, pre.scale01, pre.yscale, pre.dx, pre.dy);
        record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, Transform<float>::Concat(left, right));
      }
      auto classValue = gdef::GlyphDefinitionTable::Class::Mark;
      auto newGid = font->addCompositeGlyph(name, classValue, record, 0, op.lsb, 0, 0);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
      font->cmap->map(codepoint, *newGid);
      auto newGlyph = getGlyphByName(name);
      if (!newGlyph) {
        return EGLYF_ERROR;
      }
      newGlyph->classDef = classValue;
      auto newGlyphData = glyf->glyphs[*newGid];
      auto newBounds = glyf::GlyphDataTable::Bounds(newGlyphData);
      if (!newBounds) {
        return EGLYF_ERROR;
      }
      auto bg = make_shared<BaseGlyph>();
      bg->size = hGrids * 10 + vGrids;
      bg->gid = *newGid;
      bg->glyph = newGlyph;
      bg->bounds = *newBounds;
      bg->codepoint = codepoint;
      out.swap(bg);
      return Status::Ok();
    };

    for (auto const &it : bounds) {
      auto const &gid = it.first;
      auto const &item = it.second;
      auto const &cp = item.first;
      auto const &rect = item.second;

      string name;
      auto found = GlyphNames::GetName(cp);
      if (found) {
        name = *found;
      } else {
        name = format("u{0:x}", cp);
      }
      if (auto currentName = font->postGetName(gid); currentName) {
        if (name == *currentName) {
          if (auto st = font->postSetName(gid, "." + name); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
        }
      }
      WxH sizeNormal = decideSize(margin, s, rect.width(), rect.height());
      WxH sizeRotated = decideSize(margin, s, rect.height(), rect.width());

      shared_ptr<BaseGlyph> normal;
      if (auto st = createBaseGlyph(name, sizeNormal, cp, gid, rect, Transform<int16_t>(), normal); st.ok()) {
        if (normal) {
          baseGlyphs[name] = *normal;
        }
      } else {
        return EGLYF_STATUS_PUSH(st);
      }

      if (rotate90.find(name) != rotate90.end()) {
        shared_ptr<BaseGlyph> rot90;
        string name90 = format("{}n", name);
        if (auto st = createBaseGlyph(name90, sizeRotated, cp, gid, rect, Transform<int16_t>::CW90(), rot90); st.ok()) {
          if (rot90) {
            baseGlyphs[name90] = *rot90;
            if (auto st2 = font->cmap->addUVS(cp, 0xfe00, rot90->gid); !st2.ok()) {
              return EGLYF_STATUS_PUSH(st2);
            }
          }
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (rotate180.find(name) != rotate180.end()) {
        shared_ptr<BaseGlyph> rot180;
        string name180 = format("{}o", name);
        if (auto st = createBaseGlyph(name180, sizeNormal, cp, gid, rect, Transform<int16_t>::CW180(), rot180); st.ok()) {
          if (rot180) {
            baseGlyphs[name180] = *rot180;
            if (auto st2 = font->cmap->addUVS(cp, 0xfe01, rot180->gid); !st2.ok()) {
              return EGLYF_STATUS_PUSH(st2);
            }
          }
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      if (rotate270.find(name) != rotate270.end()) {
        shared_ptr<BaseGlyph> rot270;
        string name270 = format("{}t", name);
        if (auto st = createBaseGlyph(name270, sizeRotated, cp, gid, rect, Transform<int16_t>::CW270(), rot270); st.ok()) {
          if (rot270) {
            baseGlyphs[name270] = *rot270;
            if (auto st2 = font->cmap->addUVS(cp, 0xfe02, rot270->gid); !st2.ok()) {
              return EGLYF_STATUS_PUSH(st2);
            }
          }
        } else {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }

    auto glyphsSet1 = getGroupByName("glyphs_set1");

    for (auto const &it : baseGlyphs) {
      auto const &name = it.first;
      BaseGlyph const &baseGlyph = it.second;
      Rect<int16_t> const &rect = baseGlyph.bounds;
      WxH size = baseGlyph.size;
      int const width = rect.width();
      int const height = rect.height();

      int hGrids = WidthFromWxH(size);
      int vGrids = HeightFromWxH(size);

      SizeVariants sv;
      sv.base = baseGlyph.glyph;
      sv.bounds = rect;
      sv.size = size;
      sv.codepoint = baseGlyph.codepoint;

      glyphsSet1->members.push_back(baseGlyph.glyph);

      auto chain = variationChain.find(size);
      if (chain == variationChain.end()) {
        sizeVariants[name] = sv;
        continue;
      }

      for (WxH key : chain->second) {
        int yLevel = HeightFromWxH(key);
        int xLevel = WidthFromWxH(key);

        string n = format("{0}_{1}{2}", name, xLevel, yLevel);
        SizeVariants::Resize resize = sv.transform(xLevel, yLevel, hfu, vfu, base, lineWidth);
        auto classValue = gdef::GlyphDefinitionTable::Class::Mark;
        auto record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(baseGlyph.gid, resize.dx, resize.dy, resize.scale);
        auto newGid = font->addCompositeGlyph(n, classValue, record, 0, resize.lsb, 0, 0);
        if (!newGid) {
          return EGLYF_STATUS_PUSH(newGid.status());
        }
        auto variationGlyph = getGlyphByID(*newGid);
        if (!variationGlyph) {
          return EGLYF_ERROR;
        }
        variationGlyph->classDef = classValue;
        sv.variants[key] = variationGlyph;
        glyphsSet1->members.push_back(variationGlyph);
      }

      sizeVariants[name] = sv;
    }

    return Status::Ok();
  }

  Status createMirrorGlyphs() {
    using namespace std;

    if (!holds_alternative<Font::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<Font::TrueTypeOutlines>(font->outlines).glyf;

    static vector<pair<uint32_t, uint32_t>> const mirror = {
        {0x13000, 0x1305a},
        // 1305D 𓁝 EGYPTIAN HIEROGLYPH C002B • mirrored version of 1305C
        // 1305E 𓁞 EGYPTIAN HIEROGLYPH C002C • mirrored version of 1305B
        {0x1305c, 0x1305c},
        {0x1305F, 0x13068},
        // 1306A 𓁪 EGYPTIAN HIEROGLYPH C013 • mirrored version of 13069
        {0x1306B, 0x13071},
        // 13072 𓁲 EGYPTIAN HIEROGLYPH C021 • phonemogram : bs
        {0x13073, 0x13076},
        // 13077 𓁷 EGYPTIAN HIEROGLYPH D002 • phonemogram : ḥr
        {0x13078, 0x13081},
        // 13082 𓂂 EGYPTIAN HIEROGLYPH D012
        {0x13083, 0x1308A},
        // 1308B 𓂋 EGYPTIAN HIEROGLYPH D021
        // 1308C 𓂌 EGYPTIAN HIEROGLYPH D022
        // 1308D 𓂍 EGYPTIAN HIEROGLYPH D023
        // 1308E 𓂎 EGYPTIAN HIEROGLYPH D024
        // 1308F 𓂏 EGYPTIAN HIEROGLYPH D025
        {0x13090, 0x13090},
        // 13091 𓂑 EGYPTIAN HIEROGLYPH D027
        // 13092 𓂒 EGYPTIAN HIEROGLYPH D027A
        // 13093 𓂓 EGYPTIAN HIEROGLYPH D028
        {0x13094, 0x13095},
        // 13096 𓂖 EGYPTIAN HIEROGLYPH D031
        // 13097 𓂗 EGYPTIAN HIEROGLYPH D031A
        // 13098 𓂘 EGYPTIAN HIEROGLYPH D032
        {0x13099, 0x1309B},
        // 1309C 𓂜 EGYPTIAN HIEROGLYPH D035
        {0x1309D, 0x130BA},
        // 130BB 𓂻 EGYPTIAN HIEROGLYPH D054
        {0x130BC, 0x130BC},
        // 130BD 𓂽 EGYPTIAN HIEROGLYPH D055
        {0x130BE, 0x130C8},
        // 130C9 𓃉 EGYPTIAN HIEROGLYPH D067
        // 130CA 𓃊 EGYPTIAN HIEROGLYPH D067A
        // 130CB 𓃋 EGYPTIAN HIEROGLYPH D067B
        // 130CC 𓃌 EGYPTIAN HIEROGLYPH D067C
        // 130CD 𓃍 EGYPTIAN HIEROGLYPH D067D
        // 130CE 𓃎 EGYPTIAN HIEROGLYPH D067E
        // 130CF 𓃏 EGYPTIAN HIEROGLYPH D067F
        // 130D0 𓃐 EGYPTIAN HIEROGLYPH D067G
        // 130D1 𓃑 EGYPTIAN HIEROGLYPH D067H
        {0x130D2, 0x1310A},
        // 1310B 𓄋 EGYPTIAN HIEROGLYPH F013
        // 1310C 𓄌 EGYPTIAN HIEROGLYPH F013A
        {0x1310D, 0x1311B},
        // 1311C 𓄜 EGYPTIAN HIEROGLYPH F028
        {0x1311D, 0x1311E},
        // 1311F 𓄟 EGYPTIAN HIEROGLYPH F031
        // 13120 𓄠 EGYPTIAN HIEROGLYPH F031A
        {0x13121, 0x13122},
        // 13123 𓄣 EGYPTIAN HIEROGLYPH F034
        // 13124 𓄤 EGYPTIAN HIEROGLYPH F035
        // 13125 𓄥 EGYPTIAN HIEROGLYPH F036
        {0x13126, 0x1312B},
        // 1312C 𓄬 EGYPTIAN HIEROGLYPH F041
        {0x1312D, 0x1312D},
        // 1312E 𓄮 EGYPTIAN HIEROGLYPH F043
        {0x1312F, 0x1312F},
        // 13130 𓄰 EGYPTIAN HIEROGLYPH F045
        // 13131 𓄱 EGYPTIAN HIEROGLYPH F045A
        // 13132 𓄲 EGYPTIAN HIEROGLYPH F046
        // 13133 𓄳 EGYPTIAN HIEROGLYPH F046A
        // 13134 𓄴 EGYPTIAN HIEROGLYPH F047
        // 13135 𓄵 EGYPTIAN HIEROGLYPH F047A
        // 13136 𓄶 EGYPTIAN HIEROGLYPH F048
        // 13137 𓄷 EGYPTIAN HIEROGLYPH F049
        {0x13138, 0x131A2},
        // 131A3 𓆣 EGYPTIAN HIEROGLYPH L001
        {0x131A4, 0x131A5},
        // 131A6 𓆦 EGYPTIAN HIEROGLYPH L003
        {0x131A7, 0x131A8},
        // 131A9 𓆩 EGYPTIAN HIEROGLYPH L006
        // 131AA 𓆪 EGYPTIAN HIEROGLYPH L006A
        // 131AB 𓆫 EGYPTIAN HIEROGLYPH L007
        // 131AC 𓆬 EGYPTIAN HIEROGLYPH L008
        // 131AD 𓆭 EGYPTIAN HIEROGLYPH M001
        {0x131AE, 0x131B6},
        // 131B7 𓆷 EGYPTIAN HIEROGLYPH M008
        {0x131B8, 0x131C4},
        // 131C5 𓇅 EGYPTIAN HIEROGLYPH M013
        {0x131C6, 0x131C6},
        // 131C7 𓇇 EGYPTIAN HIEROGLYPH M015
        // 131C8 𓇈 EGYPTIAN HIEROGLYPH M015A
        // 131C9 𓇉 EGYPTIAN HIEROGLYPH M016
        // 131CA 𓇊 EGYPTIAN HIEROGLYPH M016A
        {0x131CB, 0x131D4},
        // 131D5 𓇕 EGYPTIAN HIEROGLYPH M024A
        {0x131D6, 0x131D9},
        // 131DA 𓇚 EGYPTIAN HIEROGLYPH M028A
        {0x131DB, 0x131DC},
        // 131DD 𓇝 EGYPTIAN HIEROGLYPH M031
        // 131DE 𓇞 EGYPTIAN HIEROGLYPH M031A
        // 131DF 𓇟 EGYPTIAN HIEROGLYPH M032
        {0x131E0, 0x131E2},
        // 131E3 𓇣 EGYPTIAN HIEROGLYPH M034
        // 131E4 𓇤 EGYPTIAN HIEROGLYPH M035
        {0x131E5, 0x131E7},
        // 131E8 𓇨 EGYPTIAN HIEROGLYPH M039
        {0x131E9, 0x131EB},
        // 131EC 𓇬 EGYPTIAN HIEROGLYPH M042
        {0x131ED, 0x131ED},
        // 131EE 𓇮 EGYPTIAN HIEROGLYPH M044
        // 131EF 𓇯 EGYPTIAN HIEROGLYPH N001
        {0x131F0, 0x131F1},
        // 131F2 𓇲 EGYPTIAN HIEROGLYPH N004
        // 131F3 𓇳 EGYPTIAN HIEROGLYPH N005
        {0x131F4, 0x131F4},
        // 131F5 𓇵 EGYPTIAN HIEROGLYPH N007
        // 131F6 𓇶 EGYPTIAN HIEROGLYPH N008
        // 131F7 𓇷 EGYPTIAN HIEROGLYPH N009
        // 131F8 𓇸 EGYPTIAN HIEROGLYPH N010
        // 131F9 𓇹 EGYPTIAN HIEROGLYPH N011
        // 131FA 𓇺 EGYPTIAN HIEROGLYPH N012
        {0x131FB, 0x131FB},
        // 131FC 𓇼 EGYPTIAN HIEROGLYPH N014
        // 131FD 𓇽 EGYPTIAN HIEROGLYPH N015
        // 131FE 𓇾 EGYPTIAN HIEROGLYPH N016
        // 131FF 𓇿 EGYPTIAN HIEROGLYPH N017
        // 13200 𓈀 EGYPTIAN HIEROGLYPH N018
        // 13201 𓈁 EGYPTIAN HIEROGLYPH N018A
        // 13202 𓈂 EGYPTIAN HIEROGLYPH N018B
        // 13203 𓈃 EGYPTIAN HIEROGLYPH N019
        {0x13204, 0x13207},
        // 13208 𓈈 EGYPTIAN HIEROGLYPH N024
        // 13209 𓈉 EGYPTIAN HIEROGLYPH N025
        // 1320A 𓈊 EGYPTIAN HIEROGLYPH N025A
        // 1320B 𓈋 EGYPTIAN HIEROGLYPH N026
        // 1320C 𓈌 EGYPTIAN HIEROGLYPH N027
        // 1320D 𓈍 EGYPTIAN HIEROGLYPH N028
        {0x1320E, 0x1320E},
        // 1320F 𓈏 EGYPTIAN HIEROGLYPH N030
        // 13210 𓈐 EGYPTIAN HIEROGLYPH N031
        {0x13211, 0x13211},
        // 13212 𓈒 EGYPTIAN HIEROGLYPH N033
        // 13213 𓈓 EGYPTIAN HIEROGLYPH N033A
        {0x13214, 0x13215},
        // 13216 𓈖 EGYPTIAN HIEROGLYPH N035
        // 13217 𓈗 EGYPTIAN HIEROGLYPH N035A
        // 13218 𓈘 EGYPTIAN HIEROGLYPH N036
        // 13219 𓈙 EGYPTIAN HIEROGLYPH N037
        {0x1321A, 0x1321A},
        // 1321B 𓈛 EGYPTIAN HIEROGLYPH N038
        {0x1321C, 0x1321D},
        // 1321E 𓈞 EGYPTIAN HIEROGLYPH N041
        // 1321F 𓈟 EGYPTIAN HIEROGLYPH N042
        {0x13220, 0x1324F},
        // 13250 𓉐 EGYPTIAN HIEROGLYPH O001
        // 13251 𓉑 EGYPTIAN HIEROGLYPH O001A
        // 13252 𓉒 EGYPTIAN HIEROGLYPH O002
        {0x13253, 0x13254},
        // 13255 𓉕 EGYPTIAN HIEROGLYPH O005
        // 13256 𓉖 EGYPTIAN HIEROGLYPH O005A
        {0x13257, 0x1326E},
        // 1326F 𓉯 EGYPTIAN HIEROGLYPH O020
        // 13270 𓉰 EGYPTIAN HIEROGLYPH O020A
        // 13271 𓉱 EGYPTIAN HIEROGLYPH O021
        // 13272 𓉲 EGYPTIAN HIEROGLYPH O022
        // 13273 𓉳 EGYPTIAN HIEROGLYPH O023
        // 13274 𓉴 EGYPTIAN HIEROGLYPH O024
        // 13275 𓉵 EGYPTIAN HIEROGLYPH O024A
        // 13276 𓉶 EGYPTIAN HIEROGLYPH O025
        // 13277 𓉷 EGYPTIAN HIEROGLYPH O025A
        // 13278 𓉸 EGYPTIAN HIEROGLYPH O026
        // 13279 𓉹 EGYPTIAN HIEROGLYPH O027
        // 1327A 𓉺 EGYPTIAN HIEROGLYPH O028
        {0x1327B, 0x1327B},
        // 1327C 𓉼 EGYPTIAN HIEROGLYPH O029A
        // 1327D 𓉽 EGYPTIAN HIEROGLYPH O030
        // 1327E 𓉾 EGYPTIAN HIEROGLYPH O030A
        // 1327F 𓉿 EGYPTIAN HIEROGLYPH O031
        // 13280 𓊀 EGYPTIAN HIEROGLYPH O032
        // 13281 𓊁 EGYPTIAN HIEROGLYPH O033
        {0x13282, 0x13282},
        // 13283 𓊃 EGYPTIAN HIEROGLYPH O034
        {0x13284, 0x13284},
        // 13285 𓊅 EGYPTIAN HIEROGLYPH O036
        // 13286 EGYPTIAN HIEROGLYPH O036A
        // 13287 EGYPTIAN HIEROGLYPH O036B
        // 13288 EGYPTIAN HIEROGLYPH O036C
        // 13289 EGYPTIAN HIEROGLYPH O036D
        {0x1328A, 0x1328B},
        // 1328C 𓊌 EGYPTIAN HIEROGLYPH O039
        {0x1328D, 0x1328D},
        // 1328E 𓊎 EGYPTIAN HIEROGLYPH O041
        {0x1328F, 0x1328F},
        // 13290 𓊐 EGYPTIAN HIEROGLYPH O043
        {0x13291, 0x13294},
        // 13295 𓊕 EGYPTIAN HIEROGLYPH O048
        // 13296 𓊖 EGYPTIAN HIEROGLYPH O049
        // 13297 𓊗 EGYPTIAN HIEROGLYPH O050
        // 13298 𓊘 EGYPTIAN HIEROGLYPH O050A
        // 13299 𓊙 EGYPTIAN HIEROGLYPH O050B
        {0x1329A, 0x132A1},
        // 132A2 𓊢 EGYPTIAN HIEROGLYPH P006
        {0x132A3, 0x132A3},
        // 132A4 𓊤 EGYPTIAN HIEROGLYPH P008
        {0x132A5, 0x132A9},
        // 132AA 𓊪 EGYPTIAN HIEROGLYPH Q003
        // 132AB 𓊫 EGYPTIAN HIEROGLYPH Q004
        // 132AC 𓊬 EGYPTIAN HIEROGLYPH Q005
        // 132AD 𓊭 EGYPTIAN HIEROGLYPH Q006
        {0x132AE, 0x132B0},
        // 132B1 𓊱 EGYPTIAN HIEROGLYPH R002A
        {0x132B2, 0x132B2},
        // 132B3 𓊳 EGYPTIAN HIEROGLYPH R003A
        // 132B4 𓊴 EGYPTIAN HIEROGLYPH R003B
        // 132B5 𓊵 EGYPTIAN HIEROGLYPH R004
        {0x132B6, 0x132BC},
        // 132BD 𓊽 EGYPTIAN HIEROGLYPH R011
        {0x132BE, 0x132C0},
        // 132C1 𓋁 EGYPTIAN HIEROGLYPH R015
        {0x132C2, 0x132C2},
        // 132C3 𓋃 EGYPTIAN HIEROGLYPH R016A
        {0x132C4, 0x132C6},
        // 132C7 𓋇 EGYPTIAN HIEROGLYPH R020
        // 132C8 𓋈 EGYPTIAN HIEROGLYPH R021
        // 132C9 𓋉 EGYPTIAN HIEROGLYPH R022
        // 132CA 𓋊 EGYPTIAN HIEROGLYPH R023
        // 132CB 𓋋 EGYPTIAN HIEROGLYPH R024
        // 132CC 𓋌 EGYPTIAN HIEROGLYPH R025
        // 132CD 𓋍 EGYPTIAN HIEROGLYPH R026
        {0x132CE, 0x132CE},
        // 132CF 𓋏 EGYPTIAN HIEROGLYPH R028
        {0x132D0, 0x132DC},
        // 132DD 𓋝 EGYPTIAN HIEROGLYPH S011
        // 132DE 𓋞 EGYPTIAN HIEROGLYPH S012
        {0x132DF, 0x132DF},
        // 132E0 𓋠 EGYPTIAN HIEROGLYPH S014
        {0x132E1, 0x132E2},
        // 132E3 𓋣 EGYPTIAN HIEROGLYPH S015
        // 132E4 𓋤 EGYPTIAN HIEROGLYPH S016
        // 132E5 𓋥 EGYPTIAN HIEROGLYPH S017
        // 132E6 𓋦 EGYPTIAN HIEROGLYPH S017A
        {0x132E7, 0x132E8},
        // 132E9 𓋩 EGYPTIAN HIEROGLYPH S020
        // 132EA 𓋪 EGYPTIAN HIEROGLYPH S021
        // 132EB 𓋫 EGYPTIAN HIEROGLYPH S022
        // 132EC 𓋬 EGYPTIAN HIEROGLYPH S023
        // 132ED 𓋭 EGYPTIAN HIEROGLYPH S024
        // 132EE 𓋮 EGYPTIAN HIEROGLYPH S025
        {0x132EF, 0x132EF},
        // 132F0 𓋰 EGYPTIAN HIEROGLYPH S026A
        // 132F1 𓋱 EGYPTIAN HIEROGLYPH S026B
        // 132F2 𓋲 EGYPTIAN HIEROGLYPH S027
        {0x132F3, 0x132F8},
        // 132F9 𓋹 EGYPTIAN HIEROGLYPH S034
        // 132FA 𓋺 EGYPTIAN HIEROGLYPH S035
        // 132FB 𓋻 EGYPTIAN HIEROGLYPH S035A
        // 132FC 𓋼 EGYPTIAN HIEROGLYPH S036
        {0x132FD, 0x13301},
        // 13302 𓌂 EGYPTIAN HIEROGLYPH S042
        // 13303 𓌃 EGYPTIAN HIEROGLYPH S043
        {0x13304, 0x13308},
        // 13309 𓌉 EGYPTIAN HIEROGLYPH T003
        // 1330A 𓌊 EGYPTIAN HIEROGLYPH T003A
        {0x1330B, 0x1330F},
        // 13310 𓌐 EGYPTIAN HIEROGLYPH T008
        // 13311 𓌑 EGYPTIAN HIEROGLYPH T008A
        // 13312 𓌒 EGYPTIAN HIEROGLYPH T009
        // 13313 𓌓 EGYPTIAN HIEROGLYPH T009A
        // 13314 𓌔 EGYPTIAN HIEROGLYPH T010
        {0x13315, 0x13315},
        // 13316 𓌖 EGYPTIAN HIEROGLYPH T011A
        {0x13317, 0x13321},
        // 13322 𓌢 EGYPTIAN HIEROGLYPH T022
        // 13323 𓌣 EGYPTIAN HIEROGLYPH T023
        {0x13324, 0x13327},
        // 13328 𓌨 EGYPTIAN HIEROGLYPH T028
        {0x13329, 0x13349},
        // 1334A 𓍊 EGYPTIAN HIEROGLYPH U022
        // 1334B 𓍋 EGYPTIAN HIEROGLYPH U023
        // 1334C 𓍌 EGYPTIAN HIEROGLYPH U023A
        {0x1334D, 0x1334E},
        // 1334F 𓍏 EGYPTIAN HIEROGLYPH U026
        // 13350 𓍐 EGYPTIAN HIEROGLYPH U027
        // 13351 𓍑 EGYPTIAN HIEROGLYPH U028
        // 13352 𓍒 EGYPTIAN HIEROGLYPH U029
        {0x13353, 0x13358},
        // 13359 𓍙 EGYPTIAN HIEROGLYPH U034
        {0x1335A, 0x1335A},
        // 1335B 𓍛 EGYPTIAN HIEROGLYPH U036
        {0x1335C, 0x13360},
        // 13361 𓍡 EGYPTIAN HIEROGLYPH U042
        {0x13362, 0x1336F},
        // 13370 𓍰 EGYPTIAN HIEROGLYPH V005
        // 13371 𓍱 EGYPTIAN HIEROGLYPH V006
        // 13372 𓍲 EGYPTIAN HIEROGLYPH V007
        // 13373 𓍳 EGYPTIAN HIEROGLYPH V007A
        // 13374 𓍴 EGYPTIAN HIEROGLYPH V007B
        // 13375 𓍵 EGYPTIAN HIEROGLYPH V008
        // 13376 𓍶 EGYPTIAN HIEROGLYPH V009
        {0x13377, 0x13381},
        // 13382 𓎂 EGYPTIAN HIEROGLYPH V016
        // 13383 𓎃 EGYPTIAN HIEROGLYPH V017
        // 13384 𓎄 EGYPTIAN HIEROGLYPH V018
        // 13385 𓎅 EGYPTIAN HIEROGLYPH V019
        // 13386 𓎆 EGYPTIAN HIEROGLYPH V020
        // 13387 𓎇 EGYPTIAN HIEROGLYPH V020A
        // 13388 𓎈 EGYPTIAN HIEROGLYPH V020B
        // 13389 𓎉 EGYPTIAN HIEROGLYPH V020C
        // 1338A 𓎊 EGYPTIAN HIEROGLYPH V020D
        // 1338B 𓎋 EGYPTIAN HIEROGLYPH V020E
        // 1338C 𓎌 EGYPTIAN HIEROGLYPH V020F
        // 1338D 𓎍 EGYPTIAN HIEROGLYPH V020G
        // 1338E 𓎎 EGYPTIAN HIEROGLYPH V020H
        // 1338F 𓎏 EGYPTIAN HIEROGLYPH V020I
        // 13390 𓎐 EGYPTIAN HIEROGLYPH V020J
        // 13391 𓎑 EGYPTIAN HIEROGLYPH V020K
        // 13392 𓎒 GYPTIAN HIEROGLYPH V020L
        {0x13393, 0x13396},
        // 13397 𓎗 EGYPTIAN HIEROGLYPH V024
        {0x13398, 0x13398},
        // 13399 𓎙 EGYPTIAN HIEROGLYPH V026
        // 1339A 𓎚 EGYPTIAN HIEROGLYPH V027
        // 1339B 𓎛 EGYPTIAN HIEROGLYPH V028
        {0x1339C, 0x1339C},
        // 1339D 𓎝 EGYPTIAN HIEROGLYPH V029
        {0x1339E, 0x1339E},
        // 1339F 𓎟 EGYPTIAN HIEROGLYPH V030
        // 133A0 𓎠 EGYPTIAN HIEROGLYPH V030A
        // 133A1 𓎡 EGYPTIAN HIEROGLYPH V031
        // 133A2 𓎢 EGYPTIAN HIEROGLYPH V031A
        // 133A3 𓎣 EGYPTIAN HIEROGLYPH V032
        {0x133A4, 0x133A4},
        // 133A5 𓎥 EGYPTIAN HIEROGLYPH V033A
        {0x133A6, 0x133AA},
        // 133AB 𓎫 EGYPTIAN HIEROGLYPH V038
        // 133AC 𓎬 EGYPTIAN HIEROGLYPH V039
        {0x133AD, 0x133AE},
        // 133AF 𓎯 EGYPTIAN HIEROGLYPH W001
        // 133B0 𓎰 EGYPTIAN HIEROGLYPH W002
        // 133B1 𓎱 EGYPTIAN HIEROGLYPH W003
        // 133B2 𓎲 EGYPTIAN HIEROGLYPH W003A
        // 133B3 𓎳 EGYPTIAN HIEROGLYPH W004
        // 133B4 𓎴 EGYPTIAN HIEROGLYPH W005
        // 133B5 𓎵 EGYPTIAN HIEROGLYPH W006
        // 133B6 𓎶 EGYPTIAN HIEROGLYPH W007
        // 133B7 𓎷 EGYPTIAN HIEROGLYPH W008
        // 133B8 𓎸 EGYPTIAN HIEROGLYPH W009
        // 133B9 𓎹 EGYPTIAN HIEROGLYPH W009A
        // 133BA 𓎺 EGYPTIAN HIEROGLYPH W010
        {0x133BB, 0x133BB},
        // 133BC 𓎼 EGYPTIAN HIEROGLYPH W011
        // 133BD 𓎽 EGYPTIAN HIEROGLYPH W012
        // 133BE 𓎾 EGYPTIAN HIEROGLYPH W013
        // 133BF 𓎿 EGYPTIAN HIEROGLYPH W014
        {0x133C0, 0x133C2},
        // 133C3 𓏃 EGYPTIAN HIEROGLYPH W017
        // 133C4 𓏄 EGYPTIAN HIEROGLYPH W017A
        // 133C5 𓏅 EGYPTIAN HIEROGLYPH W018
        // 133C6 𓏆 EGYPTIAN HIEROGLYPH W018A
        // 133C7 𓏇 EGYPTIAN HIEROGLYPH W019
        {0x133C8, 0x133C8},
        // 133C9 𓏉 EGYPTIAN HIEROGLYPH W021
        // 133CA 𓏊 EGYPTIAN HIEROGLYPH W022
        // 133CB 𓏋 EGYPTIAN HIEROGLYPH W023
        // 133CC 𓏌 EGYPTIAN HIEROGLYPH W024
        // 133CD 𓏍 EGYPTIAN HIEROGLYPH W024A
        {0x133CE, 0x133CE},
        // 133CF 𓏏 EGYPTIAN HIEROGLYPH X001
        // 133D0 𓏐 EGYPTIAN HIEROGLYPH X002
        // 133D1 𓏑 EGYPTIAN HIEROGLYPH X003
        // 133D2 𓏒 EGYPTIAN HIEROGLYPH X004
        // 133D3 𓏓 EGYPTIAN HIEROGLYPH X004A
        // 133D4 𓏔 EGYPTIAN HIEROGLYPH X004B
        {0x133D5, 0x133D5},
        // 133D6 𓏖 EGYPTIAN HIEROGLYPH X006
        {0x133D7, 0x133D8},
        // 133D9 𓏙 EGYPTIAN HIEROGLYPH X008
        // 133DA 𓏚 EGYPTIAN HIEROGLYPH X008A
        // 133DB 𓏛 EGYPTIAN HIEROGLYPH Y001
        {0x133DC, 0x133DC},
        // 133DD 𓏝 EGYPTIAN HIEROGLYPH Y002
        // 133DE 𓏞 EGYPTIAN HIEROGLYPH Y003
        // 133DF 𓏟 EGYPTIAN HIEROGLYPH Y004
        // 133E0 𓏠 EGYPTIAN HIEROGLYPH Y005
        // 133E1 𓏡 EGYPTIAN HIEROGLYPH Y006
        {0x133E2, 0x133E2},
        // 133E3 𓏣 EGYPTIAN HIEROGLYPH Y008
        // 133E4 𓏤 EGYPTIAN HIEROGLYPH Z001
        // 133E5 𓏥 EGYPTIAN HIEROGLYPH Z002
        // 133E6 𓏦 EGYPTIAN HIEROGLYPH Z002A
        // 133E7 𓏧 EGYPTIAN HIEROGLYPH Z002B
        // 133E8 𓏨 EGYPTIAN HIEROGLYPH Z002C
        // 133E9 𓏩 EGYPTIAN HIEROGLYPH Z002D
        // 133EA 𓏪 EGYPTIAN HIEROGLYPH Z003
        // 133EB 𓏫 EGYPTIAN HIEROGLYPH Z003A
        // 133EC 𓏬 EGYPTIAN HIEROGLYPH Z003B
        {0x133ED, 0x133ED},
        // 133EE 𓏮 EGYPTIAN HIEROGLYPH Z004A
        {0x133EF, 0x133F2},
        // 133F3 𓏳 EGYPTIAN HIEROGLYPH Z008
        {0x133F4, 0x133F5},
        // 133F6 𓏶 EGYPTIAN HIEROGLYPH Z011
        {0x133F7, 0x133F7},
        // 133F8 𓏸 EGYPTIAN HIEROGLYPH Z013
        {0x133F9, 0x133F9},
        // 133FA 𓏺 EGYPTIAN HIEROGLYPH Z015
        // 133FB 𓏻 EGYPTIAN HIEROGLYPH Z015A
        // 133FC 𓏼 EGYPTIAN HIEROGLYPH Z015B
        // 133FD 𓏽 EGYPTIAN HIEROGLYPH Z015C
        // 133FE 𓏾 EGYPTIAN HIEROGLYPH Z015D
        // 133FF 𓏿 EGYPTIAN HIEROGLYPH Z015E
        // 13400 𓐀 EGYPTIAN HIEROGLYPH Z015F
        // 13401 𓐁 EGYPTIAN HIEROGLYPH Z015G
        // 13402 𓐂 EGYPTIAN HIEROGLYPH Z015H
        // 13403 𓐃 EGYPTIAN HIEROGLYPH Z015I
        // 13404 𓐄 EGYPTIAN HIEROGLYPH Z016
        // 13405 𓐅 EGYPTIAN HIEROGLYPH Z016A
        // 13406 𓐆 EGYPTIAN HIEROGLYPH Z016B
        // 13407 𓐇 EGYPTIAN HIEROGLYPH Z016C
        {0x13408, 0x13408},
        // 13409 𓐉 EGYPTIAN HIEROGLYPH Z016E
        {0x1340A, 0x1340A},
        // 1340B 𓐋 EGYPTIAN HIEROGLYPH Z016G
        // 1340C 𓐌 EGYPTIAN HIEROGLYPH Z016H
        // 1340D 𓐍 EGYPTIAN HIEROGLYPH AA001
        {0x1340E, 0x13410},
        // 13411 𓐑 EGYPTIAN HIEROGLYPH AA005
        // 13412 𓐒 EGYPTIAN HIEROGLYPH AA006
        // 13413 𓐓 EGYPTIAN HIEROGLYPH AA007
        // 13414 𓐔 EGYPTIAN HIEROGLYPH AA007A
        {0x13415, 0x13415},
        // 13416 𓐖 EGYPTIAN HIEROGLYPH AA008
        // 13417 𓐗 EGYPTIAN HIEROGLYPH AA009
        {0x13418, 0x13419},
        // 1341A 𓐚 EGYPTIAN HIEROGLYPH AA012
        {0x1341B, 0x13420},
        // 13421 𓐡 EGYPTIAN HIEROGLYPH AA019
        // 13422 𓐢 EGYPTIAN HIEROGLYPH AA020
        // 13423 𓐣 EGYPTIAN HIEROGLYPH AA021
        {0x13424, 0x13424},
        // 13425 𓐥 EGYPTIAN HIEROGLYPH AA023
        // 13426 𓐦 EGYPTIAN HIEROGLYPH AA024
        // 13427 𓐧 EGYPTIAN HIEROGLYPH AA025
        {0x13428, 0x13428},
        // 13429 𓐩 EGYPTIAN HIEROGLYPH AA027
        {0x1342A, 0x1342B},
        // 1342C 𓐬 EGYPTIAN HIEROGLYPH AA030
        {0x1342D, 0x1342F},
    };

    Transform<float> txm(-1, 0, 0, 1, 0, 0);
    auto classValue = gdef::GlyphDefinitionTable::Class::Mark;

    auto mirrorAll = getGroupByName("mirror_all");

    for (auto const &[name, sv] : sizeVariants) {
      if (!sv.base || !sv.base->id) {
        continue;
      }
      if (ranges::find_if(mirror, [=](pair<uint32_t, uint32_t> const &it) {
            auto [from, to] = it;
            return from <= sv.codepoint && sv.codepoint <= to;
          }) == mirror.end()) {
        continue;
      }
      auto record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(*sv.base->id, txm);
      auto n = format("{}R", name);
      auto newGid = font->addCompositeGlyph(n, classValue, record, 0, -sv.bounds.xMax, 0, 0);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
      mirrorAll->members.push_back(getGlyphByID(*newGid));
      for (auto const &[size, v] : sv.variants) {
        if (!v->id) {
          continue;
        }
        auto g = glyf->glyphs[*v->id];
        auto bounds = glyf::GlyphDataTable::Bounds(g);
        if (!bounds) {
          return EGLYF_ERROR;
        }
        auto rec = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(*v->id, txm);
        auto rn = format("{}R", v->name);
        auto gid = font->addCompositeGlyph(rn, classValue, rec, 0, -bounds->xMax, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
        mirrorAll->members.push_back(getGlyphByID(*gid));
      }
    }
    return Status::Ok();
  }

  Status preprocess() {
    using namespace std;
    if (!font->gdef) {
      font->gdef = make_shared<gdef::GlyphDefinitionTable>();
      font->gdef->majorVersion = 1;
      font->gdef->minorVersion = 3;
    }
    if (!font->gdef->glyphClassDef) {
      font->gdef->glyphClassDef = make_shared<ClassDef>();
    }
    font->ensureVmtx();
    if (auto st = createSizeVariants(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = createMirrorGlyphs(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = insertLatinGlyphs(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status insertLatinGlyphs() {
    using namespace std;
    // clang-format off
    static set<char> const belowBaseline = {
      'f', 'g', 'j', 'p', 'q', 'y',
    };
    static set<char> const aboveBaseline = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'h', 'i', 'k', 'l', 'm', 'n', 'o', 'r', 's', 't', 'u', 'v', 'w', 'x',
    };
    static set<char> const other = {
      'Q', 'z',
    };
    // clang-format on

    // Check if all Latin glyphs are present
    bool allLatinGlyphsExist = true;
    for (uint32_t code = BasicGlyphs::kMinCodepoint; code <= BasicGlyphs::kMaxCodepoint; code++) {
      auto gid = font->cmap->getGlyphID(code);
      if (!gid) {
        allLatinGlyphsExist = false;
        break;
      }
    }
    if (allLatinGlyphsExist) {
      return Status::Ok();
    }

    if (!holds_alternative<Font::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto const &glyf = get<Font::TrueTypeOutlines>(font->outlines).glyf;

    struct Metrics {
      optional<int16_t> yMin;
      optional<int16_t> baseline;
      optional<int16_t> yMax;

      void updateMinMax(Rect<int16_t> const &bounds) {
        if (yMin) {
          yMin = std::min(*yMin, bounds.yMin);
        } else {
          yMin = bounds.yMin;
        }
        if (yMax) {
          yMax = std::max(*yMax, bounds.yMax);
        }
      }

      void updateBaselineMax(Rect<int16_t> const &bounds) {
        if (yMax) {
          yMax = std::max(*yMax, bounds.yMax);
        } else {
          yMax = bounds.yMax;
        }
        if (baseline) {
          baseline = std::min(*baseline, bounds.yMin);
        } else {
          baseline = bounds.yMin;
        }
      }
    };

    // Measure latin glyphs metrics for target/tuffy font
    Metrics target;
    Metrics basic;
    for (char ch : belowBaseline) {
      auto gid = font->cmap->getGlyphID((uint32_t)ch);
      if (gid) {
        auto glyph = glyf->glyphs[*gid];
        auto bounds = glyf::GlyphDataTable::Bounds(glyph);
        if (bounds) {
          target.updateMinMax(*bounds);
        }
      }
      auto basicGlyph = BasicGlyphs::GetGlyph((uint32_t)ch);
      if (basicGlyph) {
        auto bounds = glyf::GlyphDataTable::Bounds(*basicGlyph);
        basic.updateMinMax(*bounds);
      }
    }
    for (char ch : aboveBaseline) {
      auto gid = font->cmap->getGlyphID((uint32_t)ch);
      if (gid) {
        auto glyph = glyf->glyphs[*gid];
        auto bounds = glyf::GlyphDataTable::Bounds(glyph);
        if (bounds) {
          target.updateBaselineMax(*bounds);
        }
      }
      auto basicGlyph = BasicGlyphs::GetGlyph((uint32_t)ch);
      if (basicGlyph) {
        auto bounds = glyf::GlyphDataTable::Bounds(*basicGlyph);
        if (bounds) {
          basic.updateBaselineMax(*bounds);
        }
      }
    }
    if (!basic.baseline || !basic.yMin || !basic.yMax) {
      return EGLYF_ERROR;
    }
    if (*basic.baseline < *basic.yMin) {
      return EGLYF_ERROR;
    }
    if (*basic.yMax < *basic.baseline) {
      return EGLYF_ERROR;
    }

    // Add latin glyphs
    int16_t height = *basic.yMax - *basic.yMin;
    double scale = vhu * vfu / (double)height;
    double upperRatio = (*basic.yMax - *basic.baseline) / (double)height;
    double lowerRatio = (*basic.baseline - *basic.yMin) / (double)height;
    int margin = lineWidth / 2;

    Transform<double> mtx(scale, 0, 0, scale, 0, (base - margin - *basic.yMin) * scale);
    for (uint32_t code = BasicGlyphs::kMinCodepoint; code <= BasicGlyphs::kMaxCodepoint; code++) {
      auto originalAdvanceWidth = BasicGlyphs::GetAdvanceWidth(code);
      if (!originalAdvanceWidth) {
        continue;
      }
      uint16_t advanceWidth = (uint16_t)round(*originalAdvanceWidth * scale);
      string name = cff::StdStrings::Get(code - 31);

      auto basicGlyph = BasicGlyphs::GetGlyph(code);
      if (basicGlyph) {
        vector<glyf::GlyphDataTable::Contour> contours;
        for (auto const &c : basicGlyph->contours) {
          auto t = c.transformed(mtx);
          contours.push_back(t);
        }
        Vec<double> min(basicGlyph->header.xMin, basicGlyph->header.yMin);
        auto tmin = min.transformed(mtx);
        auto gid = font->addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Base, contours, advanceWidth, round(tmin.x), 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
        font->cmap->map(code, *gid);
      } else {
        auto gid = font->addEmptyGlyph(name, gdef::GlyphDefinitionTable::Class::Base, advanceWidth, 0, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
        font->cmap->map(code, *gid);
      }
    }

    return Status::Ok();
  }

  Status replaceLookups() {
    if (auto st = replaceAltSeq(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_pr021_tsg_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps045_targetglyphs_0_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps046_targetglyphs_1_A(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_bl_perglyphsize(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ab_perglyphsize(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_mk_dist_offset(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_rt005_swaprtlsigns_M(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps076_cntrlmirrorglyphsL_M(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = replaceLookup_ps077_cntrlmirrorglyphsR_M(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status replaceAltSeq() {
    using namespace std;

    auto const createLookup = [this]() {
      auto lookup = make_shared<Lookup>();
      lookup->base = Lookup::ProcessBase{};
      lookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});

      for (auto const &n : {"vj", "hj", "om"}) {
        auto g = getGlyphByName(n);
        auto leftOnly = make_shared<Lookup::Context>();
        leftOnly->left.push_back(g);
        lookup->exceptContexts.push_back(leftOnly);
        auto rightOnly = make_shared<Lookup::Context>();
        rightOnly->right.push_back(g);
        lookup->exceptContexts.push_back(rightOnly);
      }
      {
        auto corners = getGroupByName("corners");
        auto leftOnly = make_shared<Lookup::Context>();
        leftOnly->left.push_back(corners);
        lookup->exceptContexts.push_back(leftOnly);
        auto rightOnly = make_shared<Lookup::Context>();
        rightOnly->right.push_back(corners);
        lookup->exceptContexts.push_back(rightOnly);
      }
      return lookup;
    };

    map<size_t, shared_ptr<Lookup>> shrinkList;
    map<size_t, shared_ptr<Lookup>> expandList;

    auto const accept = [this, &shrinkList, &expandList, &createLookup](uint32_t target, vector<uint32_t> const &alt) -> Status {
      auto targetGid = font->getGlyphID(target);
      auto s = make_shared<Lookup::Substitution>();
      bool ok = true;
      size_t const count = alt.size();
      if (targetGid) {
        // shrink
        shared_ptr<Lookup> shrink;
        bool created = false;
        if (auto found = shrinkList.find(count); found != shrinkList.end()) {
          shrink = found->second;
        } else {
          shrink = createLookup();
          created = true;
        }
        auto targetGlyph = getGlyphByID(*targetGid);
        s->output.push_back(targetGlyph);
        for (uint32_t a : alt) {
          auto g = font->getGlyphID(a);
          if (!g) {
            ok = false;
            break;
          }
          auto glyph = getGlyphByID(*g);
          s->input.push_back(glyph);
        }
        if (ok) {
          shrink->substitutions.push_back(s);
          if (created) {
            shrinkList[count] = shrink;
          }
        }
      } else {
        // expand
        shared_ptr<Lookup> expand;
        bool created = false;
        if (auto found = expandList.find(count); found != expandList.end()) {
          expand = found->second;
        } else {
          expand = createLookup();
          created = true;
        }
        auto targetGlyph = make_shared<Glyph>();
        auto name = format("u{0:x}", target);
        auto classDef = gdef::GlyphDefinitionTable::Class::Base;
        targetGlyph->name = name;
        targetGlyph->classDef = classDef;
        glyphs[name] = targetGlyph;

        auto targetGidEmpty = font->addEmptyGlyph(name, classDef, 0, 0, 0, 0);
        if (!targetGidEmpty) {
          return EGLYF_STATUS_PUSH(targetGidEmpty.status());
        }
        font->cmap->map(target, *targetGidEmpty);
        if (auto st = font->gdef->glyphClassDef->add(*targetGidEmpty, static_cast<uint16_t>(classDef)); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
        targetGlyph->id = *targetGidEmpty;
        glyphsLut[*targetGidEmpty] = targetGlyph;

        s->input.push_back(targetGlyph);

        for (uint32_t a : alt) {
          auto g = font->getGlyphID(a);
          if (!g) {
            ok = false;
            break;
          }
          auto glyph = getGlyphByID(*g);
          s->output.push_back(glyph);
        }
        if (ok) {
          expand->substitutions.push_back(s);
          if (created) {
            expandList[count] = expand;
          }
        }
      }
      return Status::Ok();
    };

    size_t index = 0;
    size_t tables = 0;
    auto const flush = [this, &shrinkList, &expandList, &index, &tables]() {
      for (auto &[cnt, shrink] : shrinkList) {
        tables++;
        auto name = format("ha{0:03d}_ligatures_internal_{1}", tables, cnt);
        shrink->name = name;
        lookups.insert(lookups.begin() + index, make_pair(name, shrink));
        index++;
      }
      for (auto &[cnt, expand] : expandList) {
        tables++;
        auto name = format("ha{0:03d}_multiple_internal_{1}", tables, cnt);
        expand->name = name;
        lookups.insert(lookups.begin() + index, make_pair(name, expand));
        index++;
      }
      shrinkList.clear();
      expandList.clear();
    };

    auto st = Unikemet::EnumerateAltSeq(accept);
    if (!st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    flush();

    uint32_t D50 = 0x130ad;
    uint32_t M17 = 0x131cb;
    uint32_t hj = 0x13431;
    uint32_t vj = 0x13430;
    // D50A 𓂮
    if (auto st = accept(0x130ae, {D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50B 𓂯
    if (auto st = accept(0x130af, {D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50C 𓂰
    if (auto st = accept(0x130b0, {D50, hj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50D 𓂱
    if (auto st = accept(0x130b1, {D50, hj, D50, hj, D50, vj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50E 𓂲
    if (auto st = accept(0x130b2, {D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50F 𓂳
    if (auto st = accept(0x130b3, {D50, hj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50G 𓂴
    if (auto st = accept(0x130b4, {D50, hj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50H 𓂵
    if (auto st = accept(0x130b5, {D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // D50I 𓂶
    if (auto st = accept(0x130b6, {D50, hj, D50, hj, D50, hj, D50, hj, D50}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    // M17A 𓇌
    if (auto st = accept(0x131cc, {M17, hj, M17}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    flush();

    return Status::Ok();
  }

  Status replaceLookup_rt005_swaprtlsigns_M() {
    using namespace std;
    auto a = getLookupByName("rt005_swaprtlsigns_M");
    if (!a) {
      return EGLYF_ERROR;
    }
    for (auto const &it : sizeVariants) {
      SizeVariants const &sv = it.second;
      auto g = getGlyphByName(format("{}R", sv.base->name));
      if (g->id) {
        auto s1 = make_shared<Lookup::Substitution>();
        s1->input.push_back(sv.base);
        s1->output.push_back(g);
        a->substitutions.push_back(s1);

        auto s2 = make_shared<Lookup::Substitution>();
        s2->input.push_back(g);
        s2->output.push_back(sv.base);
        a->substitutions.push_back(s2);
      }
      for (auto const &i : sv.variants) {
        auto b = getGlyphByName(format("{}R", i.second->name));
        if (b->id) {
          auto s1 = make_shared<Lookup::Substitution>();
          s1->input.push_back(i.second);
          s1->output.push_back(b);
          a->substitutions.push_back(s1);

          auto s2 = make_shared<Lookup::Substitution>();
          s2->input.push_back(b);
          s2->output.push_back(i.second);
          a->substitutions.push_back(s2);
        }
      }
    }
    return Status::Ok();
  }

  Status replaceLookup_mk_dist_offset() {
    using namespace std;
    using Pos = Insertion::Pos;
    using Info = Insertion::Info;

    auto first = ranges::find_if(lookups, [](auto const &l) { return l.first.starts_with("mk") && l.first.find("_dist_offset_") != string::npos; });
    if (first == lookups.end()) {
      return EGLYF_ERROR;
    }
    size_t index = distance(lookups.begin(), first);
    ranges::for_each(lookups, [](auto &it) {
      if (it.first.starts_with("mk") && it.first.find("_dist_offset_") != string::npos) {
        it.second->clear();
      }
    });

    shared_ptr<Feature> feature;
    int indexInFeature = -1;
    size_t count = 0;
    for (auto const &[_, script] : scripts) {
      for (auto const &langSys : script->langSysList) {
        for (auto const &f : langSys->features) {
          auto found = ranges::find(f->lookups, first->second);
          if (found != f->lookups.end()) {
            feature = f;
            indexInFeature = distance(f->lookups.begin(), found);
            break;
          }
        }
        if (feature) {
          break;
        }
      }
      if (feature) {
        break;
      }
    }
    if (!feature) {
      return EGLYF_ERROR;
    }

    map<int, vector<tuple<string, Pos, WxH>>> x;
    map<int, vector<tuple<string, Pos, WxH>>> y;
    map<pair<int, int>, vector<tuple<string, Pos, WxH>>> xy;

    for (auto i = insertionPlans.begin(); i != insertionPlans.end(); i++) {
      string const &name = i->first;
      Insertion::Plan const &plan = i->second;
      for (auto j = plan.insertions.begin(); j != plan.insertions.end(); j++) {
        Pos pos = j->first;
        map<WxH, Info> const &infos = j->second;
        for (auto k = infos.begin(); k != infos.end(); k++) {
          Info const &info = k->second;
          if (!info.dx && !info.dy) {
            continue;
          }
          WxH size = info.size;
          if (info.dx && info.dy) {
            auto key = make_pair(*info.dx, *info.dy);
            xy[key].push_back(make_tuple(name, pos, size));
          } else if (info.dx) {
            x[*info.dx].push_back(make_tuple(name, pos, size));
          } else {
            y[*info.dy].push_back(make_tuple(name, pos, size));
          }
        }
      }
    }

    auto const process = [&](vector<tuple<string, Pos, WxH>> const &glyphs, optional<int16_t> dx, optional<int16_t> dy) {
      auto lookup = make_shared<Lookup>();
      lookup->base = Lookup::ProcessBase{};
      lookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});

      auto adjust = make_shared<Lookup::AdjustSingle>();
      lookup->adjustSingle = adjust;

      set<WxH> sizes;

      for (auto j = glyphs.begin(); j != glyphs.end(); j++) {
        string const &name = get<0>(*j);
        Pos pos = get<1>(*j);
        WxH size = get<2>(*j);

        // IN_CONTEXT
        //  LEFT GLYPH "G27"
        //  LEFT GLYPH "bs22"
        // END_CONTEXT

        auto mark = format("{}{}", Insertion::StringFromPos(pos), size);
        auto mg = getGlyphByName(mark);
        auto context = make_shared<Lookup::Context>();
        shared_ptr<Glyph> glyph = getGlyphByName(name);
        context->left.push_back(glyph);
        context->left.push_back(mg);

        lookup->inContexts.push_back(context);
        sizes.insert(size);
      }

      for (WxH size : sizes) {
        // ADJUST_SINGLE GLYPH "it22" BY POS DY 279 END_POS
        auto it = getGlyphByName(format("it{}", size));
        adjust->glyphs.push_back(Lookup::AdjustGlyph(it, dx, dy));

        auto itR = getGlyphByName(format("it{}R", size));
        adjust->glyphs.push_back(Lookup::AdjustGlyph(itR, dx, dy));
      }

      string n = format("_mk_dist_offset_{}", count);
      lookups.insert(lookups.begin() + index + count, make_pair(n, lookup));
      feature->lookups.insert(feature->lookups.begin() + indexInFeature + count, lookup);

      count++;
    };

    for (auto i = x.begin(); i != x.end(); i++) {
      int dx = i->first;
      int16_t dx16 = (int16_t)round(dx * hfu * chu / (double)insertionResolution);
      vector<tuple<string, Pos, WxH>> const &glyphs = i->second;
      process(glyphs, dx16, nullopt);
    }
    for (auto i = y.begin(); i != y.end(); i++) {
      int dy = i->first;
      int16_t dy16 = (int16_t)round(dy * vfu * vhu / (double)insertionResolution);
      vector<tuple<string, Pos, WxH>> const &glyphs = i->second;
      process(glyphs, nullopt, dy16);
    }
    for (auto i = xy.begin(); i != xy.end(); i++) {
      int dx = i->first.first;
      int dy = i->first.second;
      int16_t dx16 = (int16_t)round(dx * hfu * chu / (double)insertionResolution);
      int16_t dy16 = (int16_t)round(dy * vfu * vhu / (double)insertionResolution);
      vector<tuple<string, Pos, WxH>> const &glyphs = i->second;
      process(glyphs, dx16, dy16);
    }

    return Status::Ok();
  }

  Status replaceLookup_bl_perglyphsize() {
    using namespace std;
    using Pos = Insertion::Pos;

    auto first = ranges::find_if(lookups, [](auto const &l) { return l.first.starts_with("bl") && l.first.find("_perglyphsize_") != string::npos; });
    if (first == lookups.end()) {
      return EGLYF_ERROR;
    }
    size_t index = distance(lookups.begin(), first);
    ranges::for_each(lookups, [](auto &it) {
      if (it.first.starts_with("bl") && it.first.find("_perglyphsize_") != string::npos) {
        it.second->clear();
      }
    });
    if (auto st = replaceLookup_perglyphsize(index, "bl", ""); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status replaceLookup_ab_perglyphsize() {
    using namespace std;
    using Pos = Insertion::Pos;

    auto first = ranges::find_if(lookups, [](auto const &l) { return l.first.starts_with("ab") && l.first.find("_perglyphsize_") != string::npos; });
    if (first == lookups.end()) {
      return EGLYF_ERROR;
    }
    size_t index = distance(lookups.begin(), first);
    ranges::for_each(lookups, [](auto &it) {
      if (it.first.starts_with("ab") && it.first.find("_perglyphsize_") != string::npos) {
        it.second->clear();
      }
    });
    if (auto st = replaceLookup_perglyphsize(index, "ab", "2"); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  Status replaceLookup_perglyphsize(size_t index, std::string const &lookupTableName, std::string const &suffix) {
    using namespace std;
    using Pos = Insertion::Pos;

    shared_ptr<Lookup> insertAfterLookup = lookups[index].second;

    shared_ptr<Feature> feature;
    int indexInFeature = -1;
    for (auto const &[_, script] : scripts) {
      for (auto const &langSys : script->langSysList) {
        for (auto const &f : langSys->features) {
          auto found = ranges::find(f->lookups, insertAfterLookup);
          if (found != f->lookups.end()) {
            feature = f;
            indexInFeature = distance(f->lookups.begin(), found);
            break;
          }
        }
        if (feature) {
          break;
        }
      }
      if (feature) {
        break;
      }
    }
    if (!feature) {
      return EGLYF_ERROR;
    }
    auto block = getGlyphByName("block");
    size_t count = 0;
    for (auto pos : {Pos::TopStart, Pos::BottomStart, Pos::TopEnd, Pos::BottomEnd, Pos::Top, Pos::Middle, Pos::Bottom}) {
      auto spos = Insertion::StringFromPos(pos);
      map<map<WxH, WxH>, vector<string>> sizes;
      for (auto const &[name, plan] : insertionPlans) {
        auto found = plan.insertions.find(pos);
        if (found == plan.insertions.end()) {
          continue;
        }
        if (found->second.empty()) {
          return EGLYF_ERROR;
        }
        map<WxH, WxH> key;
        for (auto const &[size, info] : found->second) {
          key[size] = info.size;
        }
        sizes[key].push_back(name);
      }
      Lookup::ProcessMarks marks(Lookup::ProcessMarks::All{});
      switch (pos) {
      case Pos::TopStart:
      case Pos::Top:
        break;
      default: {
        auto markGroupName = format("insmarkset{}", spos);
        auto markGroup = getGroupByName(markGroupName);
        marks = Lookup::ProcessMarks(Lookup::ProcessMarks::MarkGroup(markGroup));
        break;
      }
      }
      for (auto const &[key, names] : sizes) {
        auto lookup = make_shared<Lookup>();
        lookup->base = Lookup::ProcessBase{};
        lookup->marks = marks;
        for (auto const &name : names) {
          auto g = getGlyphByName(name);
          if (!g->id) {
            continue;
          }
          auto context = make_shared<Lookup::Context>();
          context->left.push_back(g);
          lookup->inContexts.push_back(context);
        }
        set<WxH> done;
        for (auto [input, output] : key) {
          int iw = WidthFromWxH(input);
          int ih = HeightFromWxH(input);
          int ow = WidthFromWxH(output);
          int oh = HeightFromWxH(output);

          auto s = make_shared<Lookup::Substitution>();
          auto posIn = getGlyphByName(format("{}{}{}{}", spos, suffix, iw, ih));
          auto posOut = getGlyphByName(format("{}{}{}{}", spos, suffix, ow, oh));
          s->input.push_back(posIn);
          s->output.push_back(block);
          s->output.push_back(posOut);
          lookup->substitutions.push_back(s);

          done.insert(input);
        }
        auto largest = key.rbegin();
        int xMaxIn = WidthFromWxH(largest->first);
        int yMaxIn = HeightFromWxH(largest->first);
        int xMaxOut = WidthFromWxH(largest->second);
        int yMaxOut = HeightFromWxH(largest->second);
        double shrinkScale = min(xMaxOut / (double)xMaxIn, yMaxOut / (double)yMaxIn);
        for (int x = 1; x <= chu; x++) {
          for (int y = 1; y <= vhu; y++) {
            WxH inputSize = x * 10 + y;
            if (done.find(inputSize) != done.end()) {
              continue;
            }
            int outputX = (int)floor(x * shrinkScale);
            int outputY = (int)floor(y * shrinkScale);
            if (outputX < 1 || outputY < 1) {
              continue;
            }
            auto s = make_shared<Lookup::Substitution>();
            auto posIn = getGlyphByName(format("{}{}{}{}", spos, suffix, x, y));
            auto posOut = getGlyphByName(format("{}{}{}{}", spos, suffix, outputX, outputY));
            s->input.push_back(posIn);
            s->output.push_back(block);
            s->output.push_back(posOut);
            lookup->substitutions.push_back(s);
          }
        }
        auto n = format("_{}_{}_{}", lookupTableName, spos, count);
        lookups.insert(lookups.begin() + index + count, make_pair(n, lookup));
        feature->lookups.insert(feature->lookups.begin() + indexInFeature + count, lookup);
        count++;
      }
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

      if (name == "O33a") {
        continue;
      }

      auto s = make_shared<Lookup::Substitution>();
      s->input.push_back(sv.base);
      auto et = getGlyphByName(format("et{0}{1}", WidthFromWxH(sv.size), HeightFromWxH(sv.size)));
      s->output.push_back(et);
      string tsh = "";
      for (auto const &[variant, glyph] : sv.variants) {
        tsh = format("{0}", variant) + tsh;
      }
      tsh = format("{0}{1}", WidthFromWxH(sv.size), HeightFromWxH(sv.size)) + tsh;
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
      auto et = format("et{0}{1}", WidthFromWxH(sv.size), HeightFromWxH(sv.size));
      s->input.push_back(getGlyphByName(et));
      s->input.push_back(sv.base);
      s->output.push_back(sv.base);
      a->substitutions.push_back(s);
    }

    return Status::Ok();
  }

  Status replaceLookup_ps046_targetglyphs_1_A() {
    using namespace std;
    auto a = getLookupByName("ps046_targetglyphs_1_A");
    if (!a) {
      return EGLYF_ERROR;
    }
    auto position = ranges::find_if(lookups, [=](auto const &it) { return it.second == a; });
    if (position == lookups.end()) {
      return EGLYF_ERROR;
    }
    size_t index = distance(lookups.begin(), position);
    shared_ptr<Feature> feature;
    size_t lookupIndex = 0;
    for (auto const &script : scripts) {
      for (auto const &langSys : script.second->langSysList) {
        for (auto const &f : langSys->features) {
          auto found = ranges::find(f->lookups, a);
          if (found != f->lookups.end()) {
            feature = f;
            lookupIndex = distance(f->lookups.begin(), found);
            break;
          }
        }
        if (feature) {
          break;
        }
      }
      if (feature) {
        break;
      }
    }
    if (!feature) {
      return EGLYF_ERROR;
    }

    a->substitutions.clear();
    size_t constexpr splitThreshold = 10000;

    shared_ptr<Lookup> current = a;
    size_t count = 0;
    size_t split = 0;

    for (auto const &it : sizeVariants) {
      auto const &name = it.first;
      auto const &sv = it.second;
      for (auto const [key, glyph] : sv.variants) {
        // SUB GLYPH "et44" GLYPH "A1"
        // WITH GLYPH "A1_44"

        if (count + 3 > splitThreshold) {
          auto next = make_shared<Lookup>();
          next->base = a->base;
          next->marks = a->marks;
          auto nextName = format("{}_{}", a->name, split + 1);
          next->name = nextName;
          lookups.insert(lookups.begin() + index + split + 1, make_pair(nextName, next));
          feature->lookups.insert(feature->lookups.begin() + lookupIndex + split, next);
          split++;
          current = next;
          count = 0;
        }

        auto s = make_shared<Lookup::Substitution>();
        auto et = format("et{0}", key);
        s->input.push_back(getGlyphByName(et));
        s->input.push_back(sv.base);
        auto v = format("{0}_{1}", name, key);
        s->output.push_back(getGlyphByName(v));
        current->substitutions.push_back(s);
        count += 3;
      }
    }
    setupSubst(*current, {"et11", "BF1"}, {"BF1_11"});
    setupSubst(*current, {"et12", "BF1"}, {"BF1_12"});
    setupSubst(*current, {"et13", "BF1"}, {"BF1_13"});
    setupSubst(*current, {"et14", "BF1"}, {"BF1_14"});
    setupSubst(*current, {"et15", "BF1"}, {"BF1_15"});
    setupSubst(*current, {"et16", "BF1"}, {"BF1_16"});
    setupSubst(*current, {"et21", "BF1"}, {"BF1_21"});
    setupSubst(*current, {"et22", "BF1"}, {"BF1_22"});
    setupSubst(*current, {"et23", "BF1"}, {"BF1_23"});
    setupSubst(*current, {"et24", "BF1"}, {"BF1_24"});
    setupSubst(*current, {"et25", "BF1"}, {"BF1_25"});
    setupSubst(*current, {"et26", "BF1"}, {"BF1_26"});
    setupSubst(*current, {"et31", "BF1"}, {"BF1_31"});
    setupSubst(*current, {"et32", "BF1"}, {"BF1_32"});
    setupSubst(*current, {"et33", "BF1"}, {"BF1_33"});
    setupSubst(*current, {"et34", "BF1"}, {"BF1_34"});
    setupSubst(*current, {"et35", "BF1"}, {"BF1_35"});
    setupSubst(*current, {"et36", "BF1"}, {"BF1_36"});
    setupSubst(*current, {"et41", "BF1"}, {"BF1_41"});
    setupSubst(*current, {"et42", "BF1"}, {"BF1_42"});
    setupSubst(*current, {"et43", "BF1"}, {"BF1_43"});
    setupSubst(*current, {"et44", "BF1"}, {"BF1_44"});
    setupSubst(*current, {"et45", "BF1"}, {"BF1_45"});
    setupSubst(*current, {"et46", "BF1"}, {"BF1_46"});
    setupSubst(*current, {"et51", "BF1"}, {"BF1_51"});
    setupSubst(*current, {"et52", "BF1"}, {"BF1_52"});
    setupSubst(*current, {"et53", "BF1"}, {"BF1_53"});
    setupSubst(*current, {"et54", "BF1"}, {"BF1_54"});
    setupSubst(*current, {"et55", "BF1"}, {"BF1_55"});
    setupSubst(*current, {"et56", "BF1"}, {"BF1_56"});
    setupSubst(*current, {"et61", "BF1"}, {"BF1_61"});
    setupSubst(*current, {"et62", "BF1"}, {"BF1_62"});
    setupSubst(*current, {"et63", "BF1"}, {"BF1_63"});
    setupSubst(*current, {"et64", "BF1"}, {"BF1_64"});
    setupSubst(*current, {"et65", "BF1"}, {"BF1_65"});

    return Status::Ok();
  }

  Status replaceLookup_ps076_cntrlmirrorglyphsL_M() {
    using namespace std;
    auto a = getLookupByName("ps076_cntrlmirrorglyphsL_Ma");
    if (!a) {
      return EGLYF_ERROR;
    }
    auto b = getLookupByName("ps076_cntrlmirrorglyphsL_Mb");
    if (!b) {
      return EGLYF_ERROR;
    }
    deque<shared_ptr<Lookup::Substitution>> all;
    auto mr = getGlyphByName("mr");
    for (auto const &it : sizeVariants) {
      // SUB GLYPH "A1" GLYPH "mr"
      // WITH GLYPH "A1R"
      // END_SUB
      SizeVariants const &sv = it.second;
      auto g = getGlyphByName(format("{}R", sv.base->name));
      if (g->id) {
        auto s = make_shared<Lookup::Substitution>();
        s->input.push_back(sv.base);
        s->input.push_back(mr);
        s->output.push_back(g);
        all.push_back(s);
      }
      for (auto const &i : sv.variants) {
        auto b = getGlyphByName(format("{}R", i.second->name));
        if (b->id) {
          auto s = make_shared<Lookup::Substitution>();
          s->input.push_back(i.second);
          s->input.push_back(mr);
          s->output.push_back(b);
          all.push_back(s);
        }
      }
    }
    // Split lookup table as it exceeds the maximum value representable by Offset16
    size_t half = all.size() / 2;
    copy(all.begin(), all.begin() + half, back_inserter(a->substitutions));
    copy(all.begin() + half, all.end(), back_inserter(b->substitutions));
    return Status::Ok();
  }

  Status replaceLookup_ps077_cntrlmirrorglyphsR_M() {
    using namespace std;
    auto a = getLookupByName("ps077_cntrlmirrorglyphsR_Ma");
    if (!a) {
      return EGLYF_ERROR;
    }
    auto b = getLookupByName("ps077_cntrlmirrorglyphsR_Mb");
    if (!b) {
      return EGLYF_ERROR;
    }
    deque<shared_ptr<Lookup::Substitution>> all;
    auto mr = getGlyphByName("mr");
    for (auto const &it : sizeVariants) {
      //   SUB GLYPH "M23R" GLYPH "mr"
      //   WITH GLYPH "M23"
      // END_SUB
      SizeVariants const &sv = it.second;
      auto g = getGlyphByName(format("{}R", sv.base->name));
      if (g->id) {
        auto s = make_shared<Lookup::Substitution>();
        s->input.push_back(g);
        s->input.push_back(mr);
        s->output.push_back(sv.base);
        all.push_back(s);
      }
      for (auto const &i : sv.variants) {
        auto b = getGlyphByName(format("{}R", i.second->name));
        if (b->id) {
          auto s = make_shared<Lookup::Substitution>();
          s->input.push_back(b);
          s->input.push_back(mr);
          s->output.push_back(i.second);
          all.push_back(s);
        }
      }
    }
    // Split lookup table as it exceeds the maximum value representable by Offset16
    size_t half = all.size() / 2;
    copy(all.begin(), all.begin() + half, back_inserter(a->substitutions));
    copy(all.begin() + half, all.end(), back_inserter(b->substitutions));
    return Status::Ok();
  }

  Status insertMdCLookup() {
    using namespace std;
    using Pos = Insertion::Pos;

    map<string, vector<string>> mapping;

    mapping["A"] = {"G1"};
    mapping["i"] = {"M17"};
    mapping["y"] = {"Z4"};
    mapping["a"] = {"D36"};
    mapping["w"] = {"G43"};
    mapping["W"] = {"Z7"};
    mapping["b"] = {"D58"};
    mapping["p"] = {"Q3"};
    mapping["f"] = {"I9"};
    mapping["m"] = {"G17"};
    mapping["n"] = {"N35"};
    mapping["r"] = {"D21"};
    mapping["h"] = {"O4"};
    mapping["H"] = {"V28"};
    mapping["x"] = {"J1"};
    mapping["X"] = {"F32"};
    mapping["z"] = {"O34"};
    mapping["s"] = {"S29"};
    mapping["S"] = {"N37"};
    mapping["q"] = {"N29"};
    mapping["k"] = {"V31"};
    mapping["g"] = {"W11"};
    mapping["t"] = {"X1"};
    mapping["T"] = {"V13"};
    mapping["d"] = {"D46"};
    mapping["D"] = {"I10"};
    mapping["1"] = {"Z1"};
    mapping["qiz"] = {"A38"};
    mapping["Xrd"] = {"A17"};
    mapping["iry"] = {"A47"};
    mapping["Sps"] = {"A50"};
    mapping["Spsi"] = {"A51"};
    mapping["msi"] = {"B3"};
    mapping["DHwty"] = {"C3"};
    mapping["Xnmw"] = {"C4"};
    mapping["inpw"] = {"C6"};
    mapping["stX"] = {"C7"};
    mapping["mnw"] = {"C8"};
    mapping["mAat"] = {"C10"};
    mapping["HH"] = {"C11"};
    mapping["tp"] = {"D1"};
    mapping["Hr"] = {"D2"};
    mapping["Sny"] = {"D3"};
    mapping["ir"] = {"D4"};
    mapping["rmi"] = {"D9"};
    mapping["wDAt"] = {"D10"};
    mapping["fnD"] = {"D19"};
    mapping["rA"] = {"D21"};
    mapping["spt"] = {"D24"};
    mapping["spty"] = {"D25"};
    mapping["mnD"] = {"D27"};
    mapping["kA"] = {"D28"};
    mapping["aHA"] = {"D34"};
    mapping["Dsr"] = {"D45"};
    mapping["mt"] = {"D52"};
    mapping["rd"] = {"D56"};
    mapping["sbq"] = {"D56"};
    mapping["gH"] = {"D56"};
    mapping["gHs"] = {"D56"};
    mapping["ab"] = {"D59"};
    mapping["wab"] = {"D60"};
    mapping["sAH"] = {"D61"};
    mapping["zzmt"] = {"E6"};
    mapping["zAb"] = {"E17"};
    mapping["mAi"] = {"E22"};
    mapping["rw"] = {"E23"};
    mapping["l"] = {"E23"};
    mapping["Aby"] = {"E24"};
    mapping["wn"] = {"E34a"};
    mapping["HAt"] = {"F4"};
    mapping["SsA"] = {"F5"};
    mapping["wsr"] = {"F12"};
    mapping["wp"] = {"F13"};
    mapping["db"] = {"F16"};
    mapping["Hw"] = {"F18"};
    mapping["bH"] = {"F18"};
    mapping["ns"] = {"F20"};
    mapping["idn"] = {"F21"};
    mapping["msDr"] = {"F21"};
    mapping["sDm"] = {"F21"};
    mapping["DrD"] = {"F21"};
    mapping["pH"] = {"F22"};
    mapping["kfA"] = {"F22"};
    mapping["xpS"] = {"F23"};
    mapping["wHm"] = {"F25"};
    mapping["Xn"] = {"F26"};
    mapping["sti"] = {"F29"};
    mapping["Sd"] = {"F30"};
    mapping["ms"] = {"F31"};
    mapping["sd"] = {"F33"};
    mapping["ib"] = {"F34"};
    mapping["nfr"] = {"F35"};
    mapping["zmA"] = {"F36"};
    mapping["imAx"] = {"F39"};
    mapping["Aw"] = {"F40"};
    mapping["spr"] = {"F42"};
    mapping["iwa"] = {"F44"};
    mapping["isw"] = {"F44"};
    mapping["pXr"] = {"F46"};
    mapping["qAb"] = {"F46"};
    mapping["tyw"] = {"G4"};
    mapping["mwt"] = {"G14"};
    mapping["nbty"] = {"G16"};
    mapping["mm"] = {"G18"};
    mapping["nH"] = {"G21"};
    mapping["Db"] = {"G22"};
    mapping["rxyt"] = {"G23"};
    mapping["Ax"] = {"G25"};
    mapping["dSr"] = {"G27"};
    mapping["gm"] = {"G28"};
    mapping["bA"] = {"G29"};
    mapping["baHi"] = {"G32"};
    mapping["aq"] = {"G35"};
    mapping["wr"] = {"G36a"};
    mapping["gb"] = {"G38"};
    mapping["zA"] = {"G39"};
    mapping["pA"] = {"G40"};
    mapping["xn"] = {"G41"};
    mapping["wSA"] = {"G42"};
    mapping["ww"] = {"G44"};
    mapping["mAw"] = {"G46"};
    mapping["TA"] = {"G47"};
    mapping["snD"] = {"G54"};
    mapping["wSm"] = {"H2"};
    mapping["pAq"] = {"H3"};
    mapping["Sw"] = {"H6"};
    mapping["aSA"] = {"I1"};
    mapping["Styw"] = {"I2"};
    mapping["mzH"] = {"I3"};
    mapping["sbk"] = {"I4"};
    mapping["sAq"] = {"I5"};
    mapping["km"] = {"I6"};
    mapping["Hfn"] = {"I8"};
    mapping["DD"] = {"I11"};
    mapping["in"] = {"K1"};
    mapping["ad"] = {"K3"};
    mapping["XA"] = {"K4"};
    mapping["bz"] = {"K5"};
    mapping["nSmt"] = {"K6"};
    mapping["xpr"] = {"L1"};
    mapping["bit"] = {"L2"};
    mapping["srqt"] = {"L7"};
    mapping["iAm"] = {"M1"};
    mapping["Hn"] = {"M2"};
    mapping["xt"] = {"M3"};
    mapping["rnp"] = {"M4"};
    mapping["tr"] = {"M6"};
    mapping["SA"] = {"M8"};
    mapping["zSn"] = {"M9"};
    mapping["wdn"] = {"M11"};
    mapping["xA"] = {"M12"};
    mapping["wAD"] = {"M13"};
    mapping["HA"] = {"M16"};
    mapping["ii"] = {"M18"};
    mapping["sxt"] = {"M20"};
    mapping["sm"] = {"M21"};
    mapping["sw"] = {"M23"};
    mapping["rsw"] = {"M24"};
    mapping["Sma"] = {"M26"};
    mapping["nDm"] = {"M29"};
    mapping["bnr"] = {"M30"};
    mapping["bdt"] = {"M34"};
    mapping["Dr"] = {"M36"};
    mapping["iz"] = {"M40"};
    mapping["pt"] = {"N1"};
    mapping["iAdt"] = {"N4"};
    mapping["idt"] = {"N4"};
    mapping["ra"] = {"N5"};
    mapping["zw"] = {"N5"};
    mapping["hrw"] = {"N5"};
    mapping["Hnmmt"] = {"N8"};
    mapping["pzD"] = {"N9"};
    mapping["Abd"] = {"N11"};
    mapping["iaH"] = {"N11"};
    mapping["dwA"] = {"N14"};
    mapping["sbA"] = {"N14"};
    mapping["dwAt"] = {"N15"};
    mapping["tA"] = {"N16"};
    mapping["iw"] = {"X4b"};
    mapping["wDb"] = {"N20"};
    mapping["spAt"] = {"N24"};
    mapping["xAst"] = {"N25"};
    mapping["Dw"] = {"N26"};
    mapping["Axt"] = {"N27"};
    mapping["xa"] = {"N28"};
    mapping["iAt"] = {"N30"};
    mapping["mw"] = {"N35a"};
    mapping["Sm"] = {"N40"};
    mapping["id"] = {"N42"};
    mapping["pr"] = {"O1"};
    mapping["Hwt"] = {"O6"};
    mapping["aH"] = {"O11"};
    mapping["wsxt"] = {"O15"};
    mapping["kAr"] = {"O18"};
    mapping["zH"] = {"O22"};
    mapping["txn"] = {"O25"};
    mapping["iwn"] = {"O28"};
    mapping["aAv"] = {"O29a"};
    mapping["O29v"] = {"O29a"};
    mapping["aA"] = {"O29"};
    mapping["zxnt"] = {"O30"};
    mapping["zb"] = {"O35"};
    mapping["inb"] = {"O36"};
    mapping["Szp"] = {"O42"};
    mapping["ipt"] = {"O45"};
    mapping["nxn"] = {"O47"};
    mapping["niwt"] = {"O49"};
    mapping["zp"] = {"O50"};
    mapping["Snwt"] = {"O51"};
    mapping["wHa"] = {"P4"};
    mapping["TAw"] = {"P5"};
    mapping["nfw"] = {"P5"};
    mapping["aHa"] = {"P6"};
    mapping["xrw"] = {"P8"};
    mapping["st"] = {"Q1"};
    mapping["wz"] = {"Q2"};
    mapping["qrsw"] = {"Q6"};
    mapping["qrs"] = {"Q6"};
    mapping["xAwt"] = {"R1"};
    mapping["xAt"] = {"R1"};
    mapping["Htp"] = {"R4"};
    mapping["kAp"] = {"R5"};
    mapping["kp"] = {"R5"};
    mapping["snTr"] = {"R7"};
    mapping["nTr"] = {"R8"};
    mapping["bd"] = {"R9"};
    mapping["dd"] = {"R11"};
    mapping["Dd"] = {"R11"};
    mapping["imnt"] = {"R14"};
    mapping["iAb"] = {"R15"};
    mapping["wx"] = {"R16"};
    mapping["xm"] = {"R22"};
    mapping["HDt"] = {"S1"};
    mapping["dSrt"] = {"S3"};
    mapping["sxmty"] = {"S6"};
    mapping["xprS"] = {"S7"};
    mapping["Atf"] = {"S8"};
    mapping["Swty"] = {"S9"};
    mapping["mDH"] = {"S10"};
    mapping["wsx"] = {"S11"};
    mapping["nbw"] = {"S12"};
    mapping["tHn"] = {"S15"};
    mapping["THn"] = {"S15"};
    mapping["mnit"] = {"S18"};
    mapping["sDAw"] = {"S19"};
    mapping["xtm"] = {"S20"};
    mapping["sT"] = {"S22"};
    mapping["dmD"] = {"S23"};
    mapping["Tz"] = {"S24"};
    mapping["Sndyt"] = {"S26"};
    mapping["mnxt"] = {"S27"};
    mapping["sf"] = {"S30"};
    mapping["siA"] = {"S32"};
    mapping["Tb"] = {"S33"};
    mapping["anx"] = {"S34"};
    mapping["Swt"] = {"S35"};
    mapping["xw"] = {"S37"};
    mapping["HqA"] = {"S38"};
    mapping["awt"] = {"S39"};
    mapping["wAs"] = {"S40"};
    mapping["Dam"] = {"S41"};
    mapping["abA"] = {"S42"};
    mapping["sxm"] = {"S42"};
    mapping["xrp"] = {"S42"};
    mapping["md"] = {"S43"};
    mapping["Ams"] = {"S44"};
    mapping["nxxw"] = {"S45"};
    mapping["HD"] = {"T3"};
    mapping["HDD"] = {"T6"};
    mapping["pd"] = {"T9"};
    mapping["pD"] = {"T10"};
    mapping["zin"] = {"T11"};
    mapping["zwn"] = {"T11"};
    mapping["sXr"] = {"T11"};
    mapping["Ai"] = {"T12"};
    mapping["Ar"] = {"T12"};
    mapping["rwd"] = {"T12"};
    mapping["rwD"] = {"T12"};
    mapping["rs"] = {"T13"};
    mapping["qmA"] = {"T14"};
    mapping["wrrt"] = {"T17"};
    mapping["Sms"] = {"T18"};
    mapping["qs"] = {"T19"};
    mapping["wa"] = {"T21"};
    mapping["sn"] = {"T22"};
    mapping["iH"] = {"T24"};
    mapping["DbA"] = {"T25"};
    mapping["Xr"] = {"T28"};
    mapping["nmt"] = {"T29"};
    mapping["sSm"] = {"T31"};
    mapping["nm"] = {"T34"};
    mapping["mA"] = {"U1"};
    mapping["mr"] = {"U6"};
    mapping["it"] = {"U10"};
    mapping["HqAt"] = {"U11"};
    mapping["hb"] = {"U13"};
    mapping["Sna"] = {"U13"};
    mapping["tm"] = {"U15"};
    mapping["biA"] = {"U16"};
    mapping["grg"] = {"U17"};
    mapping["stp"] = {"U21"};
    mapping["mnx"] = {"U22"};
    mapping["Ab"] = {"U23"};
    mapping["Hmt"] = {"U24"};
    mapping["wbA"] = {"U26"};
    mapping["DA"] = {"U28"};
    mapping["rtH"] = {"U31"};
    mapping["zmn"] = {"U32"};
    mapping["ti"] = {"U33"};
    mapping["xsf"] = {"U34"};
    mapping["Hm"] = {"U36"};
    mapping["mxAt"] = {"U38"};
    mapping["St"] = {"V1"};
    mapping["Snt"] = {"V1"};
    mapping["100"] = {"V1"};
    mapping["sTA"] = {"V2"};
    mapping["sTAw"] = {"V3"};
    mapping["wA"] = {"V4"};
    mapping["snT"] = {"V5"};
    mapping["Sn"] = {"V7"};
    mapping["arq"] = {"V12"};
    mapping["iTi"] = {"V15"};
    mapping["mDt"] = {"V19"};
    mapping["XAr"] = {"V19"};
    mapping["TmA"] = {"V19"};
    mapping["10"] = {"V20"};
    mapping["mD"] = {"V20"};
    mapping["mH"] = {"V22"};
    mapping["wD"] = {"V24"};
    mapping["aD"] = {"V26"};
    mapping["wAH"] = {"V29"};
    mapping["sk"] = {"V29"};
    mapping["nb"] = {"V30"};
    mapping["msn"] = {"V32"};
    mapping["sSr"] = {"V33"};
    mapping["idr"] = {"V37"};
    mapping["bAs"] = {"W2"};
    mapping["Hb"] = {"W3a"};
    mapping["Xnm"] = {"W9"};
    mapping["iab"] = {"W10"};
    mapping["nst"] = {"W11"};
    mapping["Hz"] = {"W14"};
    mapping["xnt"] = {"W17"};
    mapping["mi"] = {"W19"};
    mapping["Hnqt"] = {"W22"};
    mapping["nw"] = {"W24"};
    mapping["ini"] = {"W25"};
    mapping["rdi"] = {"X8"};
    mapping["di"] = {"X8"};
    mapping["Y1v"] = {"Y1a"};
    mapping["mDAt"] = {"Y1"};
    mapping["mnhd"] = {"Y3"};
    mapping["mn"] = {"Y5"};
    mapping["ibA"] = {"Y6"};
    mapping["zSSt"] = {"Y8"};
    mapping["imi"] = {"Z11"};
    mapping["wnm"] = {"Z11"};
    mapping["`"] = {"Z14"};
    mapping["Hp"] = {"J5"};
    mapping["qn"] = {"J8"};
    mapping["mAa"] = {"J11"};
    mapping["im"] = {"J13"};
    mapping["gs"] = {"J13"};
    mapping["sA"] = {"J17"};
    mapping["apr"] = {"J20"};
    mapping["wDa"] = {"J21"};
    mapping["nD"] = {"J27"};
    mapping["qd"] = {"J28"};
    mapping["Xkr"] = {"J30"};
    mapping["2"] = {"Z15a"};
    mapping["3"] = {"Z15b"};
    mapping["4"] = {"Z15c"};
    mapping["5"] = {"Z15d"};
    mapping["6"] = {"Z15e"};
    mapping["7"] = {"Z15f"};
    mapping["8"] = {"Z15g"};
    mapping["9"] = {"Z15h"};
    mapping["nn"] = {"M22a"};
    // MdC    dq
    // 1 2    1 3
    // 3 4    2 4
    mapping["#1234"] = {"dq1234"};
    mapping["#123"] = {"dq123"};
    mapping["#124"] = {"dq134"};
    mapping["#134"] = {"dq124"};
    mapping["#234"] = {"dq234"};
    mapping["#12"] = {"dq13"};
    mapping["#13"] = {"dq12"};
    mapping["#14"] = {"dq14"};
    mapping["#23"] = {"dq23"};
    mapping["#24"] = {"dq34"};
    mapping["#34"] = {"dq24"};
    mapping["#1"] = {"dq1"};
    mapping["#2"] = {"dq3"};
    mapping["#3"] = {"dq2"};
    mapping["#4"] = {"dq4"};

    auto const add = [&](string mdc, char32_t cp) {
      auto gid = font->cmap->getGlyphID((uint32_t)cp);
      if (!gid) {
        return;
      }
      auto name = font->postGetName(*gid);
      if (!name) {
        return;
      }
      vector<string> c;
      c.push_back(*name);
      mapping[mdc] = c;
    };

    add("Aa1", U'𓐍');
    add("Aa2", U'𓐎');
    add("Aa3", U'𓐏');
    add("Aa4", U'𓐐');
    add("Aa5", U'𓐑');
    add("Aa6", U'𓐒');
    add("Aa7", U'𓐓');
    add("Aa7A", U'𓐔');
    add("Aa7B", U'𓐕');
    add("Aa8", U'𓐖');
    add("Aa9", U'𓐗');
    add("Aa10", U'𓐘');
    add("Aa11", U'𓐙');
    add("Aa12", U'𓐚');
    add("Aa13", U'𓐛');
    add("Aa14", U'𓐜');
    add("Aa15", U'𓐝');
    add("Aa16", U'𓐞');
    add("Aa17", U'𓐟');
    add("Aa18", U'𓐠');
    add("Aa19", U'𓐡');
    add("Aa20", U'𓐢');
    add("Aa21", U'𓐣');
    add("Aa22", U'𓐤');
    add("Aa23", U'𓐥');
    add("Aa24", U'𓐦');
    add("Aa25", U'𓐧');
    add("Aa26", U'𓐨');
    add("Aa27", U'𓐩');
    add("Aa28", U'𓐪');
    add("Aa29", U'𓐫');
    add("Aa30", U'𓐬');
    add("Aa31", U'𓐭');
    add("Aa32", U'𓐮');

    auto const &table = GlyphNames::GetTable();
    for (auto const &[cp, name] : table) {
      mapping[name] = {name};
    }

    mapping[":"] = {"vj"};
    mapping["*"] = {"hj"};
    mapping["("] = {"ss"};
    mapping[")"] = {"se"};
    mapping["##"] = {"mi"};
    mapping["\\"] = {"mr"};
    mapping["\\r1"] = {"VS3"};
    mapping["\\r2"] = {"VS2"};
    mapping["\\r3"] = {"VS1"};
    mapping["<"] = {"cb", "esb"};
    mapping["<1"] = {"cb", "esb"};
    mapping["<2"] = {"crb", "esb"};
    mapping[">"] = {"ese", "ce"};
    mapping["2>"] = {"ese", "ce"};
    mapping["1>"] = {"ese", "cre"};
    mapping["<h1"] = {"hwtb", "esb"};
    mapping["<h2"] = {"hwtbb", "esb"};
    mapping["<h3"] = {"hwttb", "esb"};
    mapping["h1>"] = {"ese", "hwte"};
    mapping["h2>"] = {"ese", "hwtbe"};
    mapping["h3>"] = {"ese", "hwtte"};
    mapping["<s1"] = {"hwtb", "esb"};
    mapping["s2>"] = {"ese", "O33a"};

    vector<shared_ptr<Lookup>> reorder;
    map<Pos, shared_ptr<Lookup>> insertionLookups;
    insertionLookups[Pos::TopStart] = getLookupByName("mdc004");
    insertionLookups[Pos::BottomStart] = getLookupByName("mdc005");
    insertionLookups[Pos::Top] = getLookupByName("mdc006");
    insertionLookups[Pos::TopEnd] = getLookupByName("mdc007");
    insertionLookups[Pos::BottomEnd] = getLookupByName("mdc008");
    insertionLookups[Pos::Bottom] = getLookupByName("mdc009");
    insertionLookups[Pos::Middle] = getLookupByName("mdc010");

    auto ampGID = font->cmap->getGlyphID('&');
    if (!ampGID) {
      return EGLYF_ERROR;
    }
    auto ampGlyph = getGlyphByID(*ampGID);
    auto spaceGID = font->cmap->getGlyphID(' ');
    if (!spaceGID) {
      return EGLYF_ERROR;
    }
    auto spaceGlyph = getGlyphByID(*spaceGID);

    for (auto &[pos, lookup] : insertionLookups) {
      auto s = make_shared<Lookup::Substitution>();
      s->input.push_back(ampGlyph);
      auto ps = Insertion::StringFromPos(pos);
      s->output.push_back(getGlyphByName(ps));
      lookup->substitutions.push_back(s);
      lookup->base = Lookup::ProcessBase{};
      lookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
    }
    for (auto const &[name, plan] : insertionPlans) {
      auto g = getGlyphByName(name);
      if (plan.insertions.size() == 1) {
        auto const &[pos, infos] = *plan.insertions.begin();

        auto context3 = make_shared<Lookup::Context>();
        context3->left.push_back(g);

        auto context4 = make_shared<Lookup::Context>();
        context4->left.push_back(g);
        context4->left.push_back(spaceGlyph);

        insertionLookups[pos]->inContexts.push_back(context3);
        insertionLookups[pos]->inContexts.push_back(context4);
        continue;
      }
      for (auto const &[pos, infos] : plan.insertions) {
        auto context1 = make_shared<Lookup::Context>();
        context1->left.push_back(g);
        auto context2 = make_shared<Lookup::Context>();
        context2->left.push_back(g);
        context2->left.push_back(spaceGlyph);
        insertionLookups[pos]->inContexts.push_back(context1);
        insertionLookups[pos]->inContexts.push_back(context2);
      }
    }
    set<Pos> removeAsEmpty;
    for (auto const &[pos, lookup] : insertionLookups) {
      if (lookup->inContexts.empty()) {
        removeAsEmpty.insert(pos);
      }
    }
    for (auto pos : removeAsEmpty) {
      insertionLookups.erase(pos);
    }

    set<string> ligs;
    map<string, string> codes;
    map<string, vector<string>> multiple;
    for (auto const &[input, output] : mapping) {
      if (output.size() == 1) {
        codes[input] = output[0];
      } else {
        string name = "lig";
        for (auto const &o : output) {
          name += format(".{}", o);
        }
        if (ligs.find(name) == ligs.end()) {
          if (auto gid = font->addEmptyGlyph(name, gdef::GlyphDefinitionTable::Class::Mark, 0, 0, 0, 0); !gid) {
            return EGLYF_STATUS_PUSH(gid.status());
          }
          ligs.insert(name);
        }
        codes[input] = name;
        multiple[name] = output;
      }
    }

    map<char, shared_ptr<Glyph>> single;
    map<size_t, map<string, shared_ptr<Glyph>>> ligature;
    for (auto const &[code, name] : codes) {
      auto glyph = getGlyphByName(name);
      if (code.size() == 1) {
        single[code[0]] = glyph;
        ligature[code.size() + 1][code + " "] = glyph;
        ligature[code.size() + 1][code + "-"] = glyph;
      } else {
        ligature[code.size()][code] = glyph;
        ligature[code.size() + 1][code + " "] = glyph;
        ligature[code.size() + 1][code + "-"] = glyph;
      }
    }

    auto ligatureLookup = getLookupByName("mdc001");
    ligatureLookup->base = Lookup::ProcessBase{};
    ligatureLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
    reorder.push_back(ligatureLookup);
    for (auto it = ligature.rbegin(); it != ligature.rend(); it++) {
      size_t count = it->first;
      map<string, shared_ptr<Glyph>> const &glyphList = it->second;
      for (auto const &[code, output] : glyphList) {
        auto s = make_shared<Lookup::Substitution>();
        bool ok = true;
        for (auto c : code) {
          auto gid = font->cmap->getGlyphID(c);
          if (!gid) {
            ok = false;
            break;
          }
          auto g = getGlyphByID(*gid);
          s->input.push_back(g);
        }
        if (!ok) {
          continue;
        }
        s->output.push_back(output);
        ligatureLookup->substitutions.push_back(s);
      }
    }

    auto singleLookup = getLookupByName("mdc002");
    singleLookup->base = Lookup::ProcessBase{};
    singleLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
    reorder.push_back(singleLookup);
    for (auto const &[code, output] : single) {
      auto s = make_shared<Lookup::Substitution>();
      auto gid = font->cmap->getGlyphID(code);
      if (!gid) {
        continue;
      }
      auto g = getGlyphByID(*gid);
      s->input.push_back(g);
      s->output.push_back(output);
      singleLookup->substitutions.push_back(s);
    }

    auto multipleLookup = getLookupByName("mdc003");
    multipleLookup->base = Lookup::ProcessBase{};
    multipleLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
    reorder.push_back(multipleLookup);
    for (auto const &[input, output] : multiple) {
      auto s = make_shared<Lookup::Substitution>();
      s->input.push_back(getGlyphByName(input));
      for (auto const &o : output) {
        s->output.push_back(getGlyphByName(o));
      }
      multipleLookup->substitutions.push_back(s);
    }

    for (auto it = insertionLookups.begin(); it != insertionLookups.end(); it++) {
      auto pos = it->first;
      auto lookup = it->second;
      reorder.push_back(lookup);
    }

    auto cleanupLookup = getLookupByName("mdc011");
    cleanupLookup->base = Lookup::ProcessBase{};
    cleanupLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
    reorder.push_back(cleanupLookup);
    for (auto const &[pos, lookup] : insertionLookups) {
      auto ps = getGlyphByName(Insertion::StringFromPos(pos));

      auto s1 = make_shared<Lookup::Substitution>();
      s1->input.push_back(spaceGlyph);
      s1->input.push_back(ps);
      s1->output.push_back(ps);
      cleanupLookup->substitutions.push_back(s1);

      auto s2 = make_shared<Lookup::Substitution>();
      s2->input.push_back(ps);
      s2->input.push_back(spaceGlyph);
      s2->output.push_back(ps);
      cleanupLookup->substitutions.push_back(s2);
    }

    lookups.erase(ranges::remove_if(lookups, [&](auto const &it) { return ranges::find(reorder, it.second) != reorder.end(); }).begin(), lookups.end());
    for (auto it = reorder.rbegin(); it != reorder.rend(); it++) {
      lookups.insert(lookups.begin(), make_pair((*it)->name, *it));
    }

    auto feature = make_shared<Feature>("Ligature", FCC("liga"));
    for (auto const &lookup : reorder) {
      feature->lookups.push_back(lookup);
    }

    set<shared_ptr<LangSys>> done;
    for (auto &script : scripts) {
      for (auto &langSys : script.second->langSysList) {
        if (done.count(langSys) == 0) {
          langSys->features.insert(langSys->features.begin(), feature);
          done.insert(langSys);
        }
      }
    }

    return Status::Ok();
  }

  Status createQuadratBase() {
    using namespace std;
    if (!holds_alternative<Font::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<Font::TrueTypeOutlines>(font->outlines).glyf;
    for (int h = 1; h <= hhu; h++) {
      string name = format("QB{}", h);
      auto gid = font->postGetGlyphID(name);
      if (!gid) {
        return EGLYF_ERROR;
      }
      glyf::GlyphDataTable::EmptyGlyph eg;
      glyf->glyphs[*gid] = eg;

      hmtx::HorizontalMetricsTable::LongHorMetric hm;
      hm.advanceWidth = sb + h * hfu + sb;
      hm.lsb = 0;
      font->hmtx->metrics[*gid] = hm;

      auto vmtx = font->ensureVmtx();
      vmtx->set(*gid, vhu * vfu, vhu * vfu);

      if (auto st = font->setGlyphClass(*gid, gdef::GlyphDefinitionTable::Class::Base); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return Status::Ok();
  }

  Status postprocess() {
    using namespace std;
    if (auto st = createQuadratBase(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = PlaceholderGlyph::Create(*font, base, hfu, sb, chu, vfu, vhu); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = CartoucheGlyph::Create(*font, base, hfu, sb, hhu, chu, vfu, vhu, lineWidth); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Insertion::CreatePlan(*font, sizeVariants, chu, vhu, hfu, vfu, base, lineWidth, insertionResolution, insertionPlans); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = ShadingGlyph::Create(*font, hhu, chu, vhu, hfu, vfu, base, lineWidth); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = BracketGlyph::Create(*font, base, hhu, vfu, vhu); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if constexpr (false) {
      if (holds_alternative<Font::TrueTypeOutlines>(font->outlines)) {
        using Contour = glyf::GlyphDataTable::Contour;
        auto process = [this](string const &name, int x, int y) {
          auto &glyf = get<Font::TrueTypeOutlines>(font->outlines).glyf;
          auto bs11 = getGlyphByName(name);
          vector<Contour> contours;
          int margin = lineWidth / 2;
          Contour::MakeRect(-hfu * x / 2, 0, hfu * x / 2, vfu * y, contours);
          glyf->replaceOutline(*bs11->id, contours, *font->maxp);
          hmtx::HorizontalMetricsTable::LongHorMetric hm;
          hm.advanceWidth = 0;
          hm.lsb = -hfu * x / 2;
          font->hmtx->metrics[*bs11->id] = hm;
        };
        process("G37", 6, 5);
        process("bs11", 1, 1);
        process("it11", 1, 1);
        process("m0", 1, 1);
      }
    }
    if (cfg.enableSubstMdc) {
      if (auto st = insertMdCLookup(); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
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
      for (auto const &name : {"O33aeL"}) {
        auto glyph = getGlyphByName(name);
        a1->second->glyphs[glyph] = Vec<optional<int16_t>>(sb, dy);
      }
    }
    if (auto r1 = anchors.find("r1"); r1 != anchors.end()) {
      int16_t const dy = vfu * vhu;
      for (auto const &[prefix, suffix] : initializer_list<pair<string, string>>{{"QB", ""}, {"QD", ""}, {"QD", "V"}, {"QF", ""}, {"QF", "V"}, {"QO", ""}, {"QO", "V"}, {"QC", ""}, {"QC", "V"}, {"QW", ""}, {"QW", "V"}}) {
        for (int h = 1; h <= hhu; h++) {
          // DEF_ANCHOR "r1" ON 469 GLYPH QB1 COMPONENT 1 AT  POS DX 420 DY 1860 END_POS END_ANCHOR
          auto name = format("{}{}{}", prefix, h, suffix);
          auto glyph = getGlyphByName(name);
          int16_t const dx = sb + hfu * h;
          r1->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
    }
    if (auto bottom = anchors.find("bottom"); bottom != anchors.end()) {
      for (auto const &prefix : {"r0v", "r1v", "r2v"}) {
        for (int v = 1; v <= vhu; v++) {
          for (auto const &suffix : {"", "R"}) {
            // DEF_ANCHOR "bottom" ON None GLYPH r0v1 COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, v, suffix);
            auto glyph = getGlyphByName(name);
            int16_t const dy = -v * vfu;
            bottom->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
      for (auto &it : bottom->second->glyphs) {
        auto const &glyph = it.first;
        auto const &name = glyph->name;
        // type == 'NYSPC'): # Negative Y spacer
        static regex const re("^r[012]s([0-9])p([0-9]+)R?");
        smatch m;
        if (regex_search(name, m, re)) {
          string sp = m.format("$1.$2");
          float fsp;
          try {
            fsp = stof(sp);
          } catch (...) {
            continue;
          }
          int16_t dy = (int16_t)round(-fsp * vfu);
          it.second = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    if (auto MARK_top = anchors.find("MARK_top"); MARK_top != anchors.end()) {
      for (auto const &key : {"a", "b", "l", "p", "r", "u"}) {
        for (int i = 0; i <= 2; i++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_top" ON 1600 GLYPH tcab0_1 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
            auto name = format("tc{}b{}_{}", key, i, v);
            auto glyph = getGlyphByName(name);
            int16_t const dy = v * vfu;
            MARK_top->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    if (auto MARK_right = anchors.find("MARK_right"); MARK_right != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_right" ON None GLYPH es11 COMPONENT 1 AT  POS DX 315 DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t const dx = h * hfu;
          int16_t const dy = v * vfu;
          MARK_right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (auto const &key : {"a", "b", "l", "p", "r", "u"}) {
        for (int i = 0; i <= 2; i++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_right" ON 1618 GLYPH tcae0_1 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
            auto name = format("tc{}e{}_{}", key, i, v);
            auto glyph = getGlyphByName(name);
            int16_t const dy = v * vfu;
            MARK_right->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
    }
    if (auto right = anchors.find("right"); right != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        // DEF_ANCHOR "right" ON None GLYPH c0h1 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        auto name = format("c0h{}", h);
        auto glyph = getGlyphByName(name);
        int16_t const dx = h * hfu;
        right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
      }
      for (int i = 1; i <= 2; i++) {
        for (int h = 1; h <= chu; h++) {
          // DEF_ANCHOR "right" ON None GLYPH c1h1 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("c{}h{}", i, h);
          auto glyph = getGlyphByName(name);
          int16_t const dx = h * hfu;
          right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (auto &it : right->second->glyphs) {
        auto const &glyph = it.first;
        auto const &name = glyph->name;
        // type == 'XSPC'
        static regex const re("^c[012]s([0-9])p([0-9]+)R?");
        smatch m;
        if (regex_search(name, m, re)) {
          string sp = m.format("$1.$2");
          float fsp;
          try {
            fsp = stof(sp);
          } catch (...) {
            continue;
          }
          int16_t dx = (int16_t)round(fsp * hfu);
          it.second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "right" ON None GLYPH o11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t const dx = h * hfu;
          right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (auto const &key : {"s", "i"}) {
        // DEF_ANCHOR "right" ON None GLYPH s11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH i11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            auto name = format("{}{}{}", key, h, v);
            auto glyph = getGlyphByName(name);
            int16_t const dx = h * hfu;
            right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "right" ON None GLYPH es12 COMPONENT 1 AT  POS DX 315 DY 620 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = v * vfu;
          right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (auto const &key : {"om", "om2"}) {
        // DEF_ANCHOR "right" ON None GLYPH om11 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        // DEF_ANCHOR "right" ON None GLYPH om211 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            auto name = format("{}{}{}", key, h, v);
            auto glyph = getGlyphByName(name);
            int16_t const dx = h * hfu;
            right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "right" ON None GLYPH it11R COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("it{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          int16_t const dx = h * hfu;
          right->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
    }
    if (auto left = anchors.find("left"); left != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        // DEF_ANCHOR "left" ON None GLYPH c0h1R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
        auto name = format("c0h{}R", h);
        auto glyph = getGlyphByName(name);
        int16_t const dx = -h * hfu;
        left->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
      }
      for (int i = 1; i <= 2; i++) {
        for (int h = 1; h <= chu; h++) {
          // DEF_ANCHOR "left" ON None GLYPH c1h1R COMPONENT 1 AT  POS DX -315 END_POS END_ANCHOR
          auto name = format("c{}h{}R", i, h);
          auto glyph = getGlyphByName(name);
          int16_t const dx = -h * hfu;
          left->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (auto &it : left->second->glyphs) {
        auto const &glyph = it.first;
        auto const &name = glyph->name;
        // type == 'NXSPC'
        static regex const re("^c[012]s([0-9])p([0-9]+)R?");
        smatch m;
        if (regex_search(name, m, re)) {
          string sp = m.format("$1.$2");
          float fsp;
          try {
            fsp = stof(sp);
          } catch (...) {
            continue;
          }
          int16_t dx = (int16_t)round(-fsp * hfu);
          it.second = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (int i = 1; i <= chu; i++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "left" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, v);
          auto glyph = getGlyphByName(name);
          int16_t const dy = v * vfu;
          left->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    if (auto MARK_left = anchors.find("MARK_left"); MARK_left != anchors.end()) {
      for (int i = 1; i <= chu; i++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_left" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, v);
          auto glyph = getGlyphByName(name);
          int16_t const dy = v * vfu;
          MARK_left->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    if (auto MARK_ts = anchors.find("MARK_ts"); MARK_ts != anchors.end()) {
      for (int i = 1; i <= chu; i++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_ts" ON None GLYPH es11 COMPONENT 1 AT  POS DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", i, v);
          auto glyph = getGlyphByName(name);
          int16_t const dy = v * vfu;
          MARK_ts->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    auto bs = anchors.find("bs");
    auto te = anchors.find("te");
    auto be = anchors.find("be");
    if (bs != anchors.end() && te != anchors.end() && be != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "bs" ON None GLYPH o8(6) COMPONENT 1 AT  POS DY -(1860) END_POS END_ANCHOR
          // DEF_ANCHOR "te" ON None GLYPH o[8]6 COMPONENT 1 AT  POS DX [2520] END_POS END_ANCHOR
          // DEF_ANCHOR "be" ON None GLYPH o[8](6) COMPONENT 1 AT  POS DX [2520] DY -(1860) END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          int16_t dy = -v * vfu;
          bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          te->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          te->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          MARK_bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          MARK_be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          MARK_bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          MARK_be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          MARK_te->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_te" ON None GLYPH te266 COMPONENT 1 AT  POS DX 1890 END_POS END_ANCHOR
          auto name = format("te2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu;
          MARK_te->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
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
          MARK_be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          MARK_bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
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
          MARK_be->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          MARK_bs->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
    }
    if (auto MARK_ti = anchors.find("MARK_ti"); MARK_ti != anchors.end()) {
      for (auto const &prefix : {"ti", "ti2", "it", "it2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_ti" ON None GLYPH ti11 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_ti" ON None GLYPH ti211 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_ti" ON None GLYPH it11 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_ti" ON None GLYPH it211 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            MARK_ti->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (auto const &prefix : {"it", "it2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_ti" ON None GLYPH it11R COMPONENT 1 AT  POS DX -157 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_ti" ON None GLYPH it211R COMPONENT 1 AT  POS DX -157 END_POS END_ANCHOR
            auto name = format("{}{}{}R", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = -h * hfu / 2;
            MARK_ti->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_ti" ON None GLYPH es11 COMPONENT 1 AT  POS DX 157 DY 310 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu;
          MARK_ti->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
    }
    if (auto ti = anchors.find("ti"); ti != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "ti" ON None GLYPH o85 COMPONENT 1 AT  POS DX 1260 END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          ti->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
      for (auto const &prefix : {"s", "ti", "ti2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "ti" ON None GLYPH s11 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            // DEF_ANCHOR "ti" ON None GLYPH ti11 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            // DEF_ANCHOR "ti" ON None GLYPH ti211 COMPONENT 1 AT  POS DX 157 END_POS END_ANCHOR
            auto name = format("{}{}{}R", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            ti->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
          }
        }
      }
    }
    if (auto MARK_bi = anchors.find("MARK_bi"); MARK_bi != anchors.end()) {
      for (auto const &prefix : {"bi", "bi2", "it", "it2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_bi" ON None GLYPH bi23 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_bi" ON None GLYPH bi223 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_bi" ON None GLYPH it23 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_bi" ON None GLYPH it223 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            int16_t dy = -v * vfu;
            MARK_bi->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (auto const &prefix : {"it", "it2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "MARK_bi" ON None GLYPH it223R COMPONENT 1 AT  POS DX -315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "MARK_bi" ON None GLYPH it23R COMPONENT 1 AT  POS DX -315 DY -930 END_POS END_ANCHOR
            auto name = format("{}{}{}R", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = -h * hfu / 2;
            int16_t dy = -v * vfu;
            MARK_bi->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_bi" ON 749 GLYPH es23 COMPONENT 1 AT  POS DX 315 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          MARK_bi->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, nullopt);
        }
      }
    }
    if (auto bi = anchors.find("bi"); bi != anchors.end()) {
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "bi" ON None GLYPH o23 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
          auto name = format("o{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu;
          bi->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (auto const &prefix : {"s", "bi", "bi2"}) {
        for (int h = 1; h <= chu; h++) {
          for (int v = 1; v <= vhu; v++) {
            // DEF_ANCHOR "bi" ON None GLYPH s23 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "bi" ON None GLYPH bi23 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            // DEF_ANCHOR "bi" ON None GLYPH bi223 COMPONENT 1 AT  POS DX 315 DY -930 END_POS END_ANCHOR
            auto name = format("{}{}{}", prefix, h, v);
            auto glyph = getGlyphByName(name);
            int16_t dx = h * hfu / 2;
            int16_t dy = -v * vfu;
            bi->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
    }
    if (auto MARK_center = anchors.find("MARK_center"); MARK_center != anchors.end()) {
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it12 COMPONENT 1 AT  POS DX 157 DY -310 END_POS END_ANCHOR
          auto name = format("it{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it11R COMPONENT 1 AT  POS DX -157 DY -155 END_POS END_ANCHOR
          auto name = format("it{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = -h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("it2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH it211R COMPONENT 1 AT  POS DX -157 DY -155 END_POS END_ANCHOR
          auto name = format("it2{}{}R", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = -h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH om11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("om{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH om211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("om2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH mi11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("mi{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH mi211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("mi2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH bi11 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("bi{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH bi211 COMPONENT 1 AT  POS DX 157 DY -155 END_POS END_ANCHOR
          auto name = format("bi2{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = -v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH es11 COMPONENT 1 AT  POS DX 157 DY 155 END_POS END_ANCHOR
          auto name = format("es{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
        }
      }
      for (int h = 1; h <= chu; h++) {
        for (int v = 1; v <= vhu; v++) {
          // DEF_ANCHOR "MARK_center" ON None GLYPH df11 COMPONENT 1 AT  POS DX 157 DY 155 END_POS END_ANCHOR
          auto name = format("df{}{}", h, v);
          auto glyph = getGlyphByName(name);
          int16_t dx = h * hfu / 2;
          int16_t dy = v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
            MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          }
        }
      }
      for (auto const &[n, sv] : sizeVariants) {
        // A1 = 5x6
        // DEF_ANCHOR "MARK_center" ON 1811 GLYPH A1 COMPONENT 1 AT  POS DY 930 END_POS END_ANCHOR
        // DEF_ANCHOR "MARK_center" ON 8087 GLYPH A1R COMPONENT 1 AT  POS DY 930 END_POS END_ANCHOR
        {
          auto glyph = getGlyphByName(n);
          int16_t dy = HeightFromWxH(sv.size) * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
        {
          auto glyphR = getGlyphByName(format("{}R", n));
          int16_t dy = HeightFromWxH(sv.size) * vfu / 2;
          MARK_center->second->glyphs[glyphR] = Vec<optional<int16_t>>(nullopt, dy);
        }
        for (auto const &[key, glyph] : sv.variants) {
          // DEF_ANCHOR "MARK_center" ON 2888 GLYPH A1_11 COMPONENT 1 AT  POS DY 144 END_POS END_ANCHOR
          // DEF_ANCHOR "MARK_center" ON 8093 GLYPH A1_11R COMPONENT 1 AT  POS DY 144 END_POS END_ANCHOR
          int v = HeightFromWxH(key);
          int16_t dy = v * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
          auto glyphR = getGlyphByName(format("{}R", glyph->name));
          MARK_center->second->glyphs[glyphR] = Vec<optional<int16_t>>(nullopt, dy);
        }
      }
      for (int h = 1; h <= hhu; h++) {
        for (int v = 1; v <= vhu; v++) {
          auto name = format("GB1_{}{}", h, v);
          auto glyph = getGlyphByName(name);
          auto rect = PlaceholderGlyph::Bounds(h, v, base, hfu, chu, vfu, vhu);
          int16_t dy = (rect.yMax + rect.yMin) / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
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
          center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
            center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
          center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
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
            center->second->glyphs[glyph] = Vec<optional<int16_t>>(dx, dy);
          }
        }
      }
    }
    return Status::Ok();
  }

  Status compile() {
    using namespace std;

    lookups.erase(ranges::remove_if(lookups, [](auto const &it) {
                    return it.second->substitutions.empty() && !it.second->adjustSingle && !it.second->attach;
                  }).begin(),
                  lookups.end());

    auto gpos = font->gpos;
    if (!gpos) {
      auto next = make_shared<gpos::GlyphPositioningTable>();
      next->majorVersion = 1;
      next->minorVersion = 0;
      font->gpos = next;
      gpos = next;
    }
    auto gsub = font->gsub;
    if (!gsub) {
      auto next = make_shared<gsub::GlyphSubstitutionTable>();
      next->majorVersion = 1;
      next->minorVersion = 0;
      font->gsub = next;
      gsub = next;
    }

    struct ConvertedLookups {
      vector<shared_ptr<SubtableCollection::Lookup>> direct;
      vector<shared_ptr<SubtableCollection::Lookup>> indirect;
    };
    ConvertedLookups convertedGposLookups;
    ConvertedLookups convertedGsubLookups;
    ConvertedLookups convertedGsubVrt2Lookups;
    ConvertedLookups convertedGsubRtlmLookups;

    for (auto const &[name, lookup] : lookups) {
      bool const isGsub = !lookup->substitutions.empty();
      bool const isGpos = lookup->adjustSingle || lookup->attach;
      if (!isGsub && !isGpos) {
        continue;
      }
      vector<shared_ptr<SubtableCollection::Lookup>> converted;
      vector<shared_ptr<SubtableCollection::Lookup>> indirect;
      if (auto st = convertLookup(lookup, converted, indirect); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (converted.empty()) {
        continue;
      }
      if (isGsub) {
        if (name.starts_with("rt") || name.starts_with("s1")) {
          ranges::copy(converted, back_inserter(convertedGsubRtlmLookups.direct));
          ranges::copy(indirect, back_inserter(convertedGsubRtlmLookups.indirect));
        } else if (name.starts_with("vr")) {
          ranges::copy(converted, back_inserter(convertedGsubVrt2Lookups.direct));
          ranges::copy(indirect, back_inserter(convertedGsubVrt2Lookups.indirect));
        } else {
          ranges::copy(converted, back_inserter(convertedGsubLookups.direct));
          ranges::copy(indirect, back_inserter(convertedGsubLookups.indirect));
        }
      } else if (isGpos) {
        ranges::copy(converted, back_inserter(convertedGposLookups.direct));
        ranges::copy(indirect, back_inserter(convertedGposLookups.indirect));
      }
    }

    SubtableCollection::Script gposScript;
    gposScript.tag = FCC("DFLT");
    deque<shared_ptr<SubtableCollection::Lookup>> gposLookups;
    auto gposLangSys = make_shared<SubtableCollection::LangSys>();
    auto gposFeature = make_shared<SubtableCollection::Feature>();
    gposFeature->tag = FCC("mark");
    auto gposFeatureData = make_shared<SubtableCollection::FeatureData>();
    gposFeature->data = gposFeatureData;
    ranges::copy(convertedGposLookups.direct, back_inserter(gposFeatureData->lookups));
    ranges::copy(convertedGposLookups.direct, back_inserter(gposLookups));
    ranges::copy(convertedGposLookups.indirect, back_inserter(gposLookups));
    gpos->features.push_back(gposFeature);
    gposLangSys->features.push_back(gposFeature);
    gposLangSys->requiredFeature = gposFeature;
    gposScript.defaultLangSys = gposLangSys;
    ReplaceScript(*gpos, gposScript);

    SubtableCollection::Script gsubScript;
    gsubScript.tag = FCC("DFLT");
    deque<shared_ptr<SubtableCollection::Lookup>> gsubLookups;
    auto gsubLangSys = make_shared<SubtableCollection::LangSys>();

    auto gsubFeature = make_shared<SubtableCollection::Feature>();
    gsubFeature->tag = FCC("liga");
    auto gsubFeatureData = make_shared<SubtableCollection::FeatureData>();
    gsubFeature->data = gsubFeatureData;
    ranges::copy(convertedGsubLookups.direct, back_inserter(gsubFeatureData->lookups));
    ranges::copy(convertedGsubLookups.direct, back_inserter(gsubLookups));

    auto gsubVrt2Feature = make_shared<SubtableCollection::Feature>();
    gsubVrt2Feature->tag = FCC("vrt2");
    auto gsubVrt2FeatureData = make_shared<SubtableCollection::FeatureData>();
    gsubVrt2Feature->data = gsubVrt2FeatureData;
    ranges::copy(convertedGsubVrt2Lookups.direct, back_inserter(gsubVrt2FeatureData->lookups));
    ranges::copy(convertedGsubVrt2Lookups.direct, back_inserter(gsubLookups));

    auto gsubRtlmFeature = make_shared<SubtableCollection::Feature>();
    gsubRtlmFeature->tag = FCC("rtlm");
    auto gsubRtlmFeatureData = make_shared<SubtableCollection::FeatureData>();
    gsubRtlmFeature->data = gsubRtlmFeatureData;
    ranges::copy(convertedGsubRtlmLookups.direct, back_inserter(gsubRtlmFeatureData->lookups));
    ranges::copy(convertedGsubRtlmLookups.direct, back_inserter(gsubLookups));

    ranges::copy(convertedGsubLookups.indirect, back_inserter(gsubLookups));
    ranges::copy(convertedGsubVrt2Lookups.indirect, back_inserter(gsubLookups));
    ranges::copy(convertedGsubRtlmLookups.indirect, back_inserter(gsubLookups));

    gsub->features.push_back(gsubFeature);
    gsub->features.push_back(gsubVrt2Feature);
    gsub->features.push_back(gsubRtlmFeature);
    gsubLangSys->features.push_back(gsubFeature);
    gsubLangSys->features.push_back(gsubVrt2Feature);
    gsubLangSys->features.push_back(gsubRtlmFeature);
    gsubLangSys->requiredFeature = gsubFeature;
    gsubScript.defaultLangSys = gsubLangSys;
    ReplaceScript(*gsub, gsubScript);

    if (auto st = AppendLookups(*gpos, gposLookups); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = AppendLookups(*gsub, gsubLookups); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    if (cfg.name) {
      font->name->setName(cfg.name->family, cfg.name->subFamily, cfg.name->fullName, cfg.name->psName);
    }

    return Status::Ok();
  }

  static void ReplaceScript(SubtableCollection &collection, SubtableCollection::Script script) {
    using namespace std;
    auto found = ranges::find_if(collection.scripts, [&script](auto const &s) { return s.tag == script.tag; });
    if (found == collection.scripts.end()) {
      collection.scripts.push_back(script);
    } else {
      auto idx = distance(collection.scripts.begin(), found);
      collection.scripts[idx] = script;
    }
  }

  static Status AppendLookups(SubtableCollection &collection, std::deque<std::shared_ptr<SubtableCollection::Lookup>> const &lookups) {
    using namespace std;

    ranges::copy(lookups, back_inserter(collection.lookups));

    for (auto const &lookup : lookups) {
      for (auto const &subtable : lookup->data->subtables) {
        auto sub = subtable;
        if (auto extension = dynamic_pointer_cast<gpos::PositioningExtension>(subtable); extension) {
          sub = extension->extension;
        } else if (auto extension = dynamic_pointer_cast<gsub::SubstitutionExtension>(subtable); extension) {
          sub = extension->extension;
        }
        if (auto chained = dynamic_pointer_cast<ChainedContexts>(sub); chained) {
          if (auto st = chained->updateLookupToLookupListIndex(collection.lookups); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
        }
      }
    }
    return Status::Ok();
  }

private:
  // clang-format off
  template <class Extension, uint16_t ChainedContextLookupType /* 6 for GSUB, 8 for GPOS */>
  void addContextConditions(std::vector<
                              std::pair<
                                std::shared_ptr<SubtableCollection::Lookup>,
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
        auto glyphIDs = make_shared<Coverage>();
        auto const &item = context->left[index];
        collectGIDSet(item, *glyphIDs);
        if (glyphIDs->size() > 0) {
          backtrackCoverage.push_back(glyphIDs);
        }
      }

      vector<shared_ptr<Coverage>> lookaheadCoverage;
      for (size_t i = 0; i < right; i++) {
        size_t index = right - i - 1;
        auto glyphIDs = make_shared<Coverage>();
        auto const &item = context->right[index];
        collectGIDSet(item, *glyphIDs);
        if (glyphIDs->size() > 0) {
          lookaheadCoverage.push_back(glyphIDs);
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
          auto glyphIDs = make_shared<Coverage>();
          auto const &item = context->left[index];
          collectGIDSet(item, *glyphIDs);
          if (glyphIDs->size() > 0) {
            backtrackCoverage.push_back(glyphIDs);
          }
        }

        vector<shared_ptr<Coverage>> lookaheadCoverage;
        for (size_t i = 0; i < right; i++) {
          auto glyphIDs = make_shared<Coverage>();
          auto const &item = context->right[i];
          collectGIDSet(item, *glyphIDs);
          if (glyphIDs->size() > 0) {
            lookaheadCoverage.push_back(glyphIDs);
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
                                       std::shared_ptr<gdef::GlyphDefinitionTable> const &gdef) {
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
    for (auto const &[glyphID, valueRecord] : glyphValueRecords) {
      if (valueRecord.xPlacement != firstValueRecord.xPlacement ||
          valueRecord.yPlacement != firstValueRecord.yPlacement) {
        allSameValueRecord = false;
        break;
      }
    }

    // Create Coverage
    auto glyphIDs = make_shared<Coverage>();
    for (auto const &[glyphID, valueRecord] : glyphValueRecords) {
      glyphIDs->insert(glyphID);
    }

    auto coverage = glyphIDs;

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
      for (auto const &[glyphID, valueRecord] : glyphValueRecords) {
        if (valueRecord.xPlacement) {
          valueFormat |= gpos::ValueRecord::X_PLACEMENT;
        }
        if (valueRecord.yPlacement) {
          valueFormat |= gpos::ValueRecord::Y_PLACEMENT;
        }
      }
      subtable->valueFormat = valueFormat;

      // Create valueRecords
      for (auto glyphID : glyphIDs->glyphIDs) {
        subtable->valueRecords.push_back(glyphValueRecords[glyphID]);
      }

      return subtable;
    }
  }

  // Determine markFilteringSet index from Editor::Lookup marks
  uint16_t determineMarkFilteringSet(std::variant<Lookup::SkipMarks, Lookup::ProcessMarks> const &marks,
                                     std::shared_ptr<gdef::GlyphDefinitionTable> const &gdef) {
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
      gdef->markGlyphSets = gdef::MarkGlyphSets();
      // Update minorVersion to 2 if it's 1 or less
      if (gdef->minorVersion <= 1) {
        gdef->minorVersion = 2;
      }
    }

    // Collect glyph IDs
    auto glyphIDs = make_shared<Coverage>();
    collectGIDSetFromGroup(what.group, *glyphIDs);

    // Return 0 if no glyph IDs
    if (glyphIDs->size() == 0) {
      return 0;
    }

    if (!markFilteringSets) {
      markFilteringSets = make_shared<map<set<uint16_t>, pair<shared_ptr<Coverage>, size_t>>>();

      for (size_t i = 0; i < gdef->markGlyphSets->coverages.size(); i++) {
        auto const &coverage = gdef->markGlyphSets->coverages[i];
        (*markFilteringSets)[coverage->glyphIDs] = make_pair(coverage, i);
      }
    }

    if (auto found = markFilteringSets->find(glyphIDs->glyphIDs); found == markFilteringSets->end()) {
      auto index = gdef->markGlyphSets->coverages.size();
      gdef->markGlyphSets->coverages.push_back(glyphIDs);
      (*markFilteringSets)[glyphIDs->glyphIDs] = make_pair(glyphIDs, index);
      return index;
    } else {
      return found->second.second;
    }
  }

  Optional<uint16_t> determineMarkAttachmentClass(std::shared_ptr<Group> const &group, std::shared_ptr<gdef::GlyphDefinitionTable> const &gdef) {
    using namespace std;
    if (!markAttachClasses) {
      markAttachClasses = make_shared<unordered_map<uint16_t, uint16_t>>();
      if (gdef->markAttachClassDef) {
        gdef->markAttachClassDef->enumerateClassValues([this](uint16_t gid, uint16_t classValue) {
          (*markAttachClasses)[gid] = classValue;
        });
      } else {
        gdef->markAttachClassDef = make_shared<ClassDef>();
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

  template <class Container>
    requires requires(Container &c, uint16_t gid) {
      c.insert(gid);
    }
  void collectGIDSet(GG const &item, Container &glyphIDs) const {
    using namespace std;

    if (holds_alternative<shared_ptr<Glyph>>(item)) {
      auto glyph = get<shared_ptr<Glyph>>(item);
      if (glyph->id) {
        glyphIDs.insert(*glyph->id);
      }
    } else if (holds_alternative<shared_ptr<Group>>(item)) {
      auto group = get<shared_ptr<Group>>(item);
      collectGIDSetFromGroup(group, glyphIDs);
    }
  }

  template <class Container>
    requires requires(Container &c, uint16_t gid) {
      c.insert(gid);
    }
  void collectGIDSetFromGroup(std::shared_ptr<Group> const &group, Container &glyphIDs) const {
    using namespace std;

    for (auto const &member : group->members) {
      if (holds_alternative<shared_ptr<Glyph>>(member)) {
        auto glyph = get<shared_ptr<Glyph>>(member);
        if (glyph->id) {
          glyphIDs.insert(*glyph->id);
        }
      } else if (holds_alternative<shared_ptr<Group>>(member)) {
        auto subgroup = get<shared_ptr<Group>>(member);
        collectGIDSetFromGroup(subgroup, glyphIDs);
      }
    }
  }

  // Function to extract glyph IDs from a variant (glyph or group) while preserving order
  template <class C>
    requires requires(C &container, std::shared_ptr<Glyph> const &g) {
      container.push_back(g);
    }
  void collectGlyphVector(GG const &item, C &glyphs) const {
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
  void collectGlyphVectorFromGroup(std::shared_ptr<Group> const &group, C &glyphs) const {
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
    return glyph->classDef == gdef::GlyphDefinitionTable::Class::Mark;
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
  Status createAttachmentSubtable(std::shared_ptr<Lookup> const &lookup, std::vector<std::pair<std::shared_ptr<Subtable>, std::shared_ptr<Coverage>>> &result, uint16_t &lookupType) {
    using namespace std;
    using ClassID = uint16_t;

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

      map<uint16_t, shared_ptr<Glyph>> receptorGlyphs;
      for (auto const &receptor : lookup->attach->receptors) {
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(receptor, glyphs);
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            receptorGlyphs[*glyph->id] = glyph;
          }
        }
      }
      if (receptorGlyphs.empty()) {
        return Status::Ok();
      }
      auto receptorGlyphIDs = make_shared<Coverage>();
      for (auto const &[gid, glyph] : receptorGlyphs) {
        receptorGlyphIDs->insert(gid);
      }

      ClassID nextMarkClass = 0;
      map<shared_ptr<Anchor>, ClassID> anchors;
      map<uint16_t, pair<shared_ptr<Glyph>, shared_ptr<Anchor>>> ligandGlyphs;
      for (auto const &item : lookup->attach->ligands) {
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(item.ligand, glyphs);
        bool empty = true;
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            auto existing = ligandGlyphs.find(*glyph->id);
            if (existing != ligandGlyphs.end() && existing->second.second != item.anchor) {
              return EGLYF_ERROR;
            }
            ligandGlyphs[*glyph->id] = make_pair(glyph, item.anchor);
            empty = false;
          }
        }
        if (!empty) {
          if (auto found = anchors.find(item.anchor); found == anchors.end()) {
            anchors[item.anchor] = nextMarkClass;
            nextMarkClass++;
          }
        }
      }
      auto markCoverage = make_shared<Coverage>();
      for (auto const &[gid, _] : ligandGlyphs) {
        markCoverage->insert(gid);
      }

      auto mark = make_shared<gpos::MarkToBaseAttachment>();
      mark->markCoverage = markCoverage;
      mark->baseCoverage = receptorGlyphIDs;

      for (auto const &[ligandGID, it] : ligandGlyphs) {
        auto const &[ligand, anchor] = it;
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

      for (auto const &[receptorGID, receptor] : receptorGlyphs) {
        gpos::MarkToBaseAttachment::BaseRecord record;
        record.baseAnchors.resize(nextMarkClass, nullptr);

        for (auto const &[anchor, classID] : anchors) {
          auto found = anchor->glyphs.find(receptor);
          if (found == anchor->glyphs.end()) [[unlikely]] {
            return EGLYF_ERROR_WHAT("Receptor glyph not found in anchor glyphs map");
          }
          auto gposAnchor = make_shared<gpos::Anchor1>();
          gposAnchor->xCoordinate = found->second.x.value_or(0);
          gposAnchor->yCoordinate = found->second.y.value_or(0);
          record.baseAnchors[classID] = gposAnchor;
        }
        mark->baseArray.baseRecords.push_back(record);
      }
      mark->baseArray.markClassCount = nextMarkClass;

      result.push_back(make_pair(mark, mark->baseCoverage));
      return Status::Ok();
    } else if (receptorMark > 0 && ligandMark > 0) {
      // mkmk
      lookupType = 6;

      // mark2: receptor
      // mark: ligand

      map<uint16_t, shared_ptr<Glyph>> receptorGlyphs;
      for (auto const &receptor : lookup->attach->receptors) {
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(receptor, glyphs);
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            receptorGlyphs[*glyph->id] = glyph;
          }
        }
      }
      if (receptorGlyphs.empty()) {
        return Status::Ok();
      }
      auto receptorGlyphIDs = make_shared<Coverage>();
      for (auto const &[gid, _] : receptorGlyphs) {
        receptorGlyphIDs->insert(gid);
      }

      ClassID nextMarkClass = 0;
      map<shared_ptr<Anchor>, ClassID> anchors;
      map<uint16_t, pair<shared_ptr<Glyph>, shared_ptr<Anchor>>> ligandGlyphs;
      for (auto const &item : lookup->attach->ligands) {
        vector<shared_ptr<Glyph>> glyphs;
        collectGlyphVector(item.ligand, glyphs);
        bool empty = true;
        for (auto const &glyph : glyphs) {
          if (glyph->id) {
            auto existing = ligandGlyphs.find(*glyph->id);
            if (existing != ligandGlyphs.end() && existing->second.second != item.anchor) {
              return EGLYF_ERROR;
            }
            ligandGlyphs[*glyph->id] = make_pair(glyph, item.anchor);
            empty = false;
          }
        }
        if (!empty) {
          if (auto found = anchors.find(item.anchor); found == anchors.end()) {
            anchors[item.anchor] = nextMarkClass;
            nextMarkClass++;
          }
        }
      }
      auto markCoverage = make_shared<Coverage>();
      for (auto const &[gid, _] : ligandGlyphs) {
        markCoverage->insert(gid);
      }

      // NOTE: Split tables to ensure their size does not exceed the uint16_t range.
      size_t constexpr splitThreshold = 10000;
      if (markCoverage->size() > splitThreshold && receptorGlyphIDs->size() == 1) {
        vector<map<uint16_t, pair<shared_ptr<Glyph>, shared_ptr<Anchor>>>> splitLigandGlyphsList;
        map<uint16_t, pair<shared_ptr<Glyph>, shared_ptr<Anchor>>> work;
        for (auto const &[ligandGID, it] : ligandGlyphs) {
          if (work.size() + 1 > splitThreshold) {
            splitLigandGlyphsList.push_back(work);
            work.clear();
          }
          work[ligandGID] = it;
        }
        if (work.size() > 0) {
          splitLigandGlyphsList.push_back(work);
        }
        for (auto const &splitLigandGlyphs : splitLigandGlyphsList) {
          auto mkmk = make_shared<gpos::MarkToMarkAttachment>();
          mkmk->mark1Coverage = make_shared<Coverage>();
          for (auto const &[gid, _] : splitLigandGlyphs) {
            mkmk->mark1Coverage->insert(gid);
          }
          mkmk->mark2Coverage = receptorGlyphIDs;

          for (auto const &[ligandGID, it] : splitLigandGlyphs) {
            auto const &[ligand, anchor] = it;
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
            mkmk->mark1Array.markRecords.push_back(record);
          }

          auto const &begin = receptorGlyphs.begin();
          uint16_t receptorGID = begin->first;
          shared_ptr<Glyph> const &receptor = begin->second;

          gpos::MarkToMarkAttachment::Mark2 record;
          record.mark2Anchors.resize(nextMarkClass, nullptr);

          for (auto const &[anchor, classID] : anchors) {
            auto found = anchor->glyphs.find(receptor);
            auto gposAnchor = make_shared<gpos::Anchor1>();
            if (found == anchor->glyphs.end()) [[unlikely]] {
              gposAnchor->xCoordinate = 0;
              gposAnchor->yCoordinate = 0;
            } else {
              gposAnchor->xCoordinate = found->second.x.value_or(0);
              gposAnchor->yCoordinate = found->second.y.value_or(0);
            }
            record.mark2Anchors[classID] = gposAnchor;
          }
          mkmk->mark2Array.mark2Records.push_back(record);
          mkmk->mark2Array.markClassCount = nextMarkClass;

          result.push_back(make_pair(mkmk, mkmk->mark2Coverage));
        }
        return Status::Ok();
      } else {
        auto mkmk = make_shared<gpos::MarkToMarkAttachment>();
        mkmk->mark1Coverage = markCoverage;
        mkmk->mark2Coverage = receptorGlyphIDs;

        for (auto const &[ligandGID, it] : ligandGlyphs) {
          auto const &[ligand, anchor] = it;
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
          mkmk->mark1Array.markRecords.push_back(record);
        }

        for (auto const &[receptorGID, receptor] : receptorGlyphs) {
          gpos::MarkToMarkAttachment::Mark2 record;
          record.mark2Anchors.resize(nextMarkClass, nullptr);

          for (auto const &[anchor, classID] : anchors) {
            auto found = anchor->glyphs.find(receptor);
            auto gposAnchor = make_shared<gpos::Anchor1>();
            if (found == anchor->glyphs.end()) [[unlikely]] {
              gposAnchor->xCoordinate = 0;
              gposAnchor->yCoordinate = 0;
            } else {
              gposAnchor->xCoordinate = found->second.x.value_or(0);
              gposAnchor->yCoordinate = found->second.y.value_or(0);
            }
            record.mark2Anchors[classID] = gposAnchor;
          }
          mkmk->mark2Array.mark2Records.push_back(record);
        }
        mkmk->mark2Array.markClassCount = nextMarkClass;

        result.push_back(make_pair(mkmk, mkmk->mark2Coverage));
      }
      return Status::Ok();
    } else {
      return EGLYF_ERROR_WHAT("Invalid combination of receptor and ligand glyph types");
    }
  }

public:
  static int constexpr hhu = 8;
  static int constexpr vhu = 6;
  static int constexpr chu = 6;
  static int constexpr insertionResolution = 20;

  std::shared_ptr<Font> font;
  Config cfg;

  std::unordered_map<std::string, std::shared_ptr<Glyph>> glyphs;
  std::unordered_map<uint16_t, std::shared_ptr<Glyph>> glyphsLut;
  std::unordered_map<std::string, std::shared_ptr<Group>> groups;
  std::unordered_map<std::string, std::shared_ptr<Anchor>> anchors;
  std::deque<std::pair<std::string, std::shared_ptr<Lookup>>> lookups;
  std::unordered_map<std::string, std::shared_ptr<Script>> scripts;

  std::shared_ptr<std::map<std::set<uint16_t>, std::pair<std::shared_ptr<Coverage>, size_t>>> markFilteringSets;
  std::shared_ptr<std::unordered_map<uint16_t, uint16_t>> markAttachClasses;

  std::map<std::string, Insertion::Plan> insertionPlans;

  // unit per horizontal grid
  int16_t hfu;
  // unit per vertical grid
  int16_t vfu;
  std::unordered_map<std::string, SizeVariants> sizeVariants;
  // font side bearings: hfu / 3
  int16_t sb;
  int16_t base;
  int16_t lineWidth;
};

} // namespace eglyf
