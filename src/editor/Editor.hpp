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
  explicit Editor(std::shared_ptr<FontFile> const &font) : font(font) {
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
      shared_ptr<Subtable> subtable;
      uint16_t lookupType = 0;
      if (auto st = createAttachmentSubtable(lookup, subtable, lookupType); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (subtable) {
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
        auto coverage = make_shared<Coverage>();
        for (auto const &receptor : lookup->attach->receptors) {
          collectGIDSet(receptor, *coverage);
        }
        inputCoverages[1].push_back(coverage);

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

    deque<pair<WxH, float>> sizeList;
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
    uint64_t sumHeight = 0;
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
      sumHeight += rect.height();
      sumCount++;
    }

    float scale = min(sumWidth / (float)sumCount, sumHeight / (float)sumCount);
    float lineWidth = max(1.0f, scale / 32);
    float s = (sumHeight / (float)sumCount) / (8 * lineWidth + sumHeight / (float)sumCount);
    hfu = (int16_t)ceil(sumWidth / (float)sumCount / chu * s);
    vfu = (int16_t)ceil(sumHeight / (float)sumCount / vhu * s);
    base = (int16_t)round(4 * lineWidth);
    sb = hfu / 3;

    struct BaseGlyph {
      uint16_t gid;
      shared_ptr<Glyph> glyph;
      Rect<int16_t> bounds;
    };
    map<string, BaseGlyph> baseGlyphs;

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
      // https://gyazo.com/574d65263fcd74d9d9163bd9e9179a6e
      auto w = rect.width();
      auto h = rect.height();
      float const scale = min({1.0f, vhu * vfu / (float)h, chu * hfu / (float)w});
      int16_t xMid = (rect.xMin + rect.xMax) / 2;
      int16_t dx;
      int16_t dy;
      int16_t lsb;
      if (scale < 1) {
        dx = (int16_t)round(-xMid * scale);
        dy = (int16_t)round((base - rect.yMin) * scale);
        lsb = (int16_t)round((rect.xMin - xMid) * scale);
      } else {
        dx = -xMid;
        dy = base - rect.yMin;
        lsb = rect.xMin - xMid;
      }
      glyf::GlyphDataTable::CompositeGlyph::GlyphRecord record;
      if (scale < 1) {
        record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, dx, dy, scale);
      } else {
        record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(gid, dx, dy);
      }
      auto classValue = gdef::GlyphDefinitionTable::Class::Mark;
      auto newGid = font->addCompositeGlyph(name, classValue, record, 0, lsb);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
      if (auto st = font->cmap->map(cp, *newGid); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (!found) {
        continue;
      }
      auto newGlyph = getGlyphByName(name);
      if (!newGlyph) {
        return EGLYF_ERROR;
      }
      newGlyph->classDef = classValue;
      auto newGlyphData = glyf->glyphs[*newGid];
      auto b = glyf::GlyphDataTable::Bounds(newGlyphData);
      if (!b) {
        return EGLYF_ERROR;
      }
      BaseGlyph bg;
      bg.gid = *newGid;
      bg.glyph = newGlyph;
      bg.bounds = *b;
      baseGlyphs[name] = bg;
    }

    auto glyphsSet1 = getGroupByName("glyphs_set1");

    for (auto const &it : baseGlyphs) {
      auto const &name = it.first;
      BaseGlyph const &baseGlyph = it.second;
      Rect<int16_t> const &rect = baseGlyph.bounds;
      int const width = rect.width();
      int const height = rect.height();

      float const baseScale = min({1.0f, vhu * vfu / (float)height, chu * hfu / (float)width});
      int hGrids = clamp((int)ceilf(width * baseScale / hfu), 1, hhu);
      int vGrids = clamp((int)ceilf(height * baseScale / vfu), 1, vhu);

      int16_t xMid = (rect.xMin + rect.xMax) / 2;

      auto chain = variationChain.find(hGrids * 10 + vGrids);
      if (chain == variationChain.end()) {
        if (auto first = ranges::find_if(sizeList, [=](auto const &it) { return it.second >= hGrids * vGrids; }); first != sizeList.end()) {
          auto index = distance(sizeList.begin(), first);
          for (size_t i = index; i < sizeList.size(); i++) {
            auto const &it = sizeList[i];
            WxH key = it.first;
            int h = key / 10;
            int v = key % 10;
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
        }
      }

      SizeVariants sv;
      sv.base = baseGlyph.glyph;
      sv.bounds = rect;
      sv.hGrids = hGrids;
      sv.vGrids = vGrids;

      if (chain == variationChain.end()) {
        sizeVariants[name] = sv;
        continue;
      }

      for (WxH key : chain->second) {
        int yLevel = key % 10;
        int xLevel = key / 10;

        string n = format("{0}_{1}{2}", name, xLevel, yLevel);
        SizeVariants::Resize resize = sv.transform(xLevel, yLevel, hfu, vfu, base);
        auto classValue = gdef::GlyphDefinitionTable::Class::Mark;
        auto record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(baseGlyph.gid, resize.dx, resize.dy, resize.scale);
        auto newGid = font->addCompositeGlyph(n, classValue, record, 0, resize.lsb);
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
        it.second->substitutions.clear();
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
        auto sv = sizeVariants.find(name);
        if (sv == sizeVariants.end()) {
          continue;
        }
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
      int16_t dx16 = dx * hfu * chu / insertionResolution;
      vector<tuple<string, Pos, WxH>> const &glyphs = i->second;
      process(glyphs, dx16, nullopt);
    }
    for (auto i = y.begin(); i != y.end(); i++) {
      int dy = i->first;
      int16_t dy16 = dy * vfu * vhu / insertionResolution;
      vector<tuple<string, Pos, WxH>> const &glyphs = i->second;
      process(glyphs, nullopt, dy16);
    }
    for (auto i = xy.begin(); i != xy.end(); i++) {
      int dx = i->first.first;
      int dy = i->first.second;
      int16_t dx16 = dx * hfu * chu / insertionResolution;
      int16_t dy16 = dy * vfu * vhu / insertionResolution;
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
        it.second->substitutions.clear();
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
        it.second->substitutions.clear();
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
      for (auto const &[key, names] : sizes) {
        auto lookup = make_shared<Lookup>();
        lookup->base = Lookup::ProcessBase{};
        lookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
        for (auto const &name : names) {
          auto g = getGlyphByName(name);
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
          if (auto gid = font->addEmptyGlyph(name, gdef::GlyphDefinitionTable::Class::Mark, 0, 0); !gid) {
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
    if (!holds_alternative<FontFile::TrueTypeOutlines>(font->outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(font->outlines).glyf;
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
    if (auto st = CartoucheGlyph::Create(*font, base, hfu, sb, hhu, chu, vfu, vhu); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Insertion::CreatePlan(*font, sizeVariants, chu, vhu, hfu, vfu, base, insertionResolution, insertionPlans); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
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
            auto name = format("it{}{}R", h, v);
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
        {
          auto glyph = getGlyphByName(n);
          int16_t dy = sv.vGrids * vfu / 2;
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
        }
        for (auto const &[key, glyph] : sv.variants) {
          // DEF_ANCHOR "MARK_center" ON 2888 GLYPH A1_11 COMPONENT 1 AT  POS DY 144 END_POS END_ANCHOR
          int v = key % 10;
          int16_t dy = v * vfu / 2; // TODO: this equation is not sure
          MARK_center->second->glyphs[glyph] = Vec<optional<int16_t>>(nullopt, dy);
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

    auto gpos = make_shared<gpos::GlyphPositioningTable>();
    auto gsub = make_shared<gsub::GlyphSubstitutionTable>();

    gpos->majorVersion = 1;
    gpos->minorVersion = 0;

    gsub->majorVersion = 1;
    gsub->minorVersion = 0;

    struct ConvertedLookups {
      vector<shared_ptr<SubtableCollection::Lookup>> direct;
      vector<shared_ptr<SubtableCollection::Lookup>> indirect;
    };
    ConvertedLookups convertedGposLookups;
    ConvertedLookups convertedGsubLookups;
    ConvertedLookups convertedGsubVertLookups;

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
        if (name.starts_with("rt") || name.starts_with("vr") || name.starts_with("s1")) {
          ranges::copy(converted, back_inserter(convertedGsubVertLookups.direct));
          ranges::copy(indirect, back_inserter(convertedGsubVertLookups.indirect));
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
    auto gposLangSys = make_shared<SubtableCollection::LangSys>();
    auto gposFeature = make_shared<SubtableCollection::Feature>();
    gposFeature->tag = FCC("mark");
    auto gposFeatureData = make_shared<SubtableCollection::FeatureData>();
    gposFeature->data = gposFeatureData;
    ranges::copy(convertedGposLookups.direct, back_inserter(gposFeatureData->lookups));
    ranges::copy(convertedGposLookups.direct, back_inserter(gpos->lookups));
    ranges::copy(convertedGposLookups.indirect, back_inserter(gpos->lookups));
    gpos->features.push_back(gposFeature);
    gposLangSys->features.push_back(gposFeature);
    gposLangSys->requiredFeature = gposFeature;
    gposScript.defaultLangSys = gposLangSys;
    gpos->scripts.push_back(gposScript);

    SubtableCollection::Script gsubScript;
    gsubScript.tag = FCC("DFLT");
    auto gsubLangSys = make_shared<SubtableCollection::LangSys>();

    auto gsubFeature = make_shared<SubtableCollection::Feature>();
    gsubFeature->tag = FCC("liga");
    auto gsubFeatureData = make_shared<SubtableCollection::FeatureData>();
    gsubFeature->data = gsubFeatureData;
    ranges::copy(convertedGsubLookups.direct, back_inserter(gsubFeatureData->lookups));
    ranges::copy(convertedGsubLookups.direct, back_inserter(gsub->lookups));

    auto gsubVertFeature = make_shared<SubtableCollection::Feature>();
    gsubVertFeature->tag = FCC("vrt2");
    auto gsubVertFeatureData = make_shared<SubtableCollection::FeatureData>();
    gsubVertFeature->data = gsubVertFeatureData;
    ranges::copy(convertedGsubVertLookups.direct, back_inserter(gsubVertFeatureData->lookups));
    ranges::copy(convertedGsubVertLookups.direct, back_inserter(gsub->lookups));

    ranges::copy(convertedGsubLookups.indirect, back_inserter(gsub->lookups));
    ranges::copy(convertedGsubVertLookups.indirect, back_inserter(gsub->lookups));

    gsub->features.push_back(gsubFeature);
    gsub->features.push_back(gsubVertFeature);
    gsubLangSys->features.push_back(gsubFeature);
    gsubLangSys->features.push_back(gsubVertFeature);
    gsubLangSys->requiredFeature = gsubFeature;
    gsubScript.defaultLangSys = gsubLangSys;
    gsub->scripts.push_back(gsubScript);

    if (auto st = ReorderLookups(*gpos); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = ReorderLookups(*gsub); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    font->gpos = gpos;
    font->gsub = gsub;

    return Status::Ok();
  }

  static Status ReorderLookups(SubtableCollection &collection) {
    using namespace std;

    for (auto const &lookup : collection.lookups) {
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
  Status createAttachmentSubtable(std::shared_ptr<Lookup> const &lookup, std::shared_ptr<Subtable> &result, uint16_t &lookupType) {
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

      auto mark = make_unique<gpos::MarkToBaseAttachment>();
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

      result.reset(mark.release());
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

      auto mark = make_unique<gpos::MarkToMarkAttachment>();
      mark->mark1Coverage = markCoverage;
      mark->mark2Coverage = receptorGlyphIDs;

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
        mark->mark1Array.markRecords.push_back(record);
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
  static int constexpr insertionResolution = 20;

  Config cfg;

  std::shared_ptr<FontFile> font;
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
};

} // namespace eglyf
