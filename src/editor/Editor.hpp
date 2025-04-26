#pragma once

namespace eglyf {

class Editor : public std::enable_shared_from_this<Editor> {
public:
  struct Glyph {
    std::string name;
    std::optional<uint16_t> id;
    gdef::GlyphDefinitionTable::Class classDef = gdef::GlyphDefinitionTable::Class::Mark;
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
    std::shared_ptr<Feature> requiredFeature;

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
      auto gid = font->post->getGlyphID(name);
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
    if (auto name = font->post->getName(gid); name) {
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

    map<int, vector<int>> variationChain;
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

    if (!font->gdef) {
      font->gdef = make_shared<gdef::GlyphDefinitionTable>();
    }
    if (!font->gdef->glyphClassDef) {
      font->gdef->glyphClassDef = make_shared<ClassDef>();
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
    hfu = (int16_t)ceil(sumWidth / (float)sumCount / chu);
    vfu = (int16_t)ceil(sumHeight / (float)sumCount / vhu);
    sb = hfu / 3;
    base = 0;

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
      if (auto currentName = font->post->getName(gid); currentName) {
        if (name == *currentName) {
          if (auto st = font->post->setName(gid, "." + name); !st.ok()) {
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

    for (int h = 1; h <= hhu; h++) {
      string name = format("QB{}", h);
      if (auto gid = font->post->getGlyphID(name); gid) {
        if (auto st = font->post->setName(*gid, "." + name); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
      int16_t width = sb + h * hfu + sb;
      auto newGid = font->addEmptyGlyph(name, gdef::GlyphDefinitionTable::Class::Base, width, 0);
      if (!newGid) {
        return EGLYF_STATUS_PUSH(newGid.status());
      }
    }

    if (auto st = PlaceholderGlyph::Create(*font, base, hfu, sb, chu, vfu, vhu); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = CartoucheGlyph::Create(*font, base, hfu, sb, chu, vfu, vhu); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
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
            int key = it.first;
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
      sv.hGrids = hGrids;
      sv.vGrids = vGrids;

      if (chain == variationChain.end()) {
        sizeVariants[name] = sv;
        continue;
      }

      for (auto key : chain->second) {
        int yLevel = key % 10;
        int xLevel = key / 10;

        string n = format("{0}_{1}{2}", name, xLevel, yLevel);
        float xScale = hfu * xLevel / (float)width;
        float yScale = vfu * yLevel / (float)height;
        float scale = min({1.0f, xScale, yScale});
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
        auto classValue = gdef::GlyphDefinitionTable::Class::Mark;
        auto record = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord::New(baseGlyph.gid, dx, dy, scale);
        auto newGid = font->addCompositeGlyph(n, classValue, record, 0, lsb);
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

  static juce::String JuceStringFromU32String(std::u32string const &s) {
    return juce::String(juce::CharPointer_UTF32((juce::juce_wchar *)s.c_str()), juce::CharPointer_UTF32((juce::juce_wchar *)(s.c_str() + s.size())));
  }

  Status insertMdCLookup() {
    using namespace std;
    map<string, string> codes;

    codes["A"] = "G1";
    codes["i"] = "M17";
    codes["y"] = "Z4";
    codes["a"] = "D36";
    codes["w"] = "G43";
    codes["W"] = "Z7";
    codes["b"] = "D58";
    codes["p"] = "Q3";
    codes["f"] = "I9";
    codes["m"] = "G17";
    codes["n"] = "N35";
    codes["r"] = "D21";
    codes["h"] = "O4";
    codes["H"] = "V28";
    codes["x"] = "J1";
    codes["X"] = "F32";
    codes["z"] = "O34";
    codes["s"] = "S29";
    codes["S"] = "N37";
    codes["q"] = "N29";
    codes["k"] = "V31";
    codes["g"] = "W11";
    codes["t"] = "X1";
    codes["T"] = "V13";
    codes["d"] = "D46";
    codes["D"] = "I10";
    codes["1"] = "Z1";
    codes["qiz"] = "A38";
    codes["Xrd"] = "A17";
    codes["iry"] = "A47";
    codes["Sps"] = "A50";
    codes["Spsi"] = "A51";
    codes["msi"] = "B3";
    codes["DHwty"] = "C3";
    codes["Xnmw"] = "C4";
    codes["inpw"] = "C6";
    codes["stX"] = "C7";
    codes["mnw"] = "C8";
    codes["mAat"] = "C10";
    codes["HH"] = "C11";
    codes["tp"] = "D1";
    codes["Hr"] = "D2";
    codes["Sny"] = "D3";
    codes["ir"] = "D4";
    codes["rmi"] = "D9";
    codes["wDAt"] = "D10";
    codes["fnD"] = "D19";
    codes["rA"] = "D21";
    codes["spt"] = "D24";
    codes["spty"] = "D25";
    codes["mnD"] = "D27";
    codes["kA"] = "D28";
    codes["aHA"] = "D34";
    codes["Dsr"] = "D45";
    codes["mt"] = "D52";
    codes["rd"] = "D56";
    codes["sbq"] = "D56";
    codes["gH"] = "D56";
    codes["gHs"] = "D56";
    codes["ab"] = "D59";
    codes["wab"] = "D60";
    codes["sAH"] = "D61";
    codes["zzmt"] = "E6";
    codes["zAb"] = "E17";
    codes["mAi"] = "E22";
    codes["rw"] = "E23";
    codes["l"] = "E23";
    codes["Aby"] = "E24";
    codes["wn"] = "E34a";
    codes["HAt"] = "F4";
    codes["SsA"] = "F5";
    codes["wsr"] = "F12";
    codes["wp"] = "F13";
    codes["db"] = "F16";
    codes["Hw"] = "F18";
    codes["bH"] = "F18";
    codes["ns"] = "F20";
    codes["idn"] = "F21";
    codes["msDr"] = "F21";
    codes["sDm"] = "F21";
    codes["DrD"] = "F21";
    codes["pH"] = "F22";
    codes["kfA"] = "F22";
    codes["xpS"] = "F23";
    codes["wHm"] = "F25";
    codes["Xn"] = "F26";
    codes["sti"] = "F29";
    codes["Sd"] = "F30";
    codes["ms"] = "F31";
    codes["sd"] = "F33";
    codes["ib"] = "F34";
    codes["nfr"] = "F35";
    codes["zmA"] = "F36";
    codes["imAx"] = "F39";
    codes["Aw"] = "F40";
    codes["spr"] = "F42";
    codes["iwa"] = "F44";
    codes["isw"] = "F44";
    codes["pXr"] = "F46";
    codes["qAb"] = "F46";
    codes["tyw"] = "G4";
    codes["mwt"] = "G14";
    codes["nbty"] = "G16";
    codes["mm"] = "G18";
    codes["nH"] = "G21";
    codes["Db"] = "G22";
    codes["rxyt"] = "G23";
    codes["Ax"] = "G25";
    codes["dSr"] = "G27";
    codes["gm"] = "G28";
    codes["bA"] = "G29";
    codes["baHi"] = "G32";
    codes["aq"] = "G35";
    codes["wr"] = "G36a";
    codes["gb"] = "G38";
    codes["zA"] = "G39";
    codes["pA"] = "G40";
    codes["xn"] = "G41";
    codes["wSA"] = "G42";
    codes["ww"] = "G44";
    codes["mAw"] = "G46";
    codes["TA"] = "G47";
    codes["snD"] = "G54";
    codes["wSm"] = "H2";
    codes["pAq"] = "H3";
    codes["Sw"] = "H6";
    codes["aSA"] = "I1";
    codes["Styw"] = "I2";
    codes["mzH"] = "I3";
    codes["sbk"] = "I4";
    codes["sAq"] = "I5";
    codes["km"] = "I6";
    codes["Hfn"] = "I8";
    codes["DD"] = "I11";
    codes["in"] = "K1";
    codes["ad"] = "K3";
    codes["XA"] = "K4";
    codes["bz"] = "K5";
    codes["nSmt"] = "K6";
    codes["xpr"] = "L1";
    codes["bit"] = "L2";
    codes["srqt"] = "L7";
    codes["iAm"] = "M1";
    codes["Hn"] = "M2";
    codes["xt"] = "M3";
    codes["rnp"] = "M4";
    codes["tr"] = "M6";
    codes["SA"] = "M8";
    codes["zSn"] = "M9";
    codes["wdn"] = "M11";
    codes["xA"] = "M12";
    codes["wAD"] = "M13";
    codes["HA"] = "M16";
    codes["ii"] = "M18";
    codes["sxt"] = "M20";
    codes["sm"] = "M21";
    codes["sw"] = "M23";
    codes["rsw"] = "M24";
    codes["Sma"] = "M26";
    codes["nDm"] = "M29";
    codes["bnr"] = "M30";
    codes["bdt"] = "M34";
    codes["Dr"] = "M36";
    codes["iz"] = "M40";
    codes["pt"] = "N1";
    codes["iAdt"] = "N4";
    codes["idt"] = "N4";
    codes["ra"] = "N5";
    codes["zw"] = "N5";
    codes["hrw"] = "N5";
    codes["Hnmmt"] = "N8";
    codes["pzD"] = "N9";
    codes["Abd"] = "N11";
    codes["iaH"] = "N11";
    codes["dwA"] = "N14";
    codes["sbA"] = "N14";
    codes["dwAt"] = "N15";
    codes["tA"] = "N16";
    codes["iw"] = "X4b";
    codes["wDb"] = "N20";
    codes["spAt"] = "N24";
    codes["xAst"] = "N25";
    codes["Dw"] = "N26";
    codes["Axt"] = "N27";
    codes["xa"] = "N28";
    codes["iAt"] = "N30";
    codes["mw"] = "N35a";
    codes["Sm"] = "N40";
    codes["id"] = "N42";
    codes["pr"] = "O1";
    codes["Hwt"] = "O6";
    codes["aH"] = "O11";
    codes["wsxt"] = "O15";
    codes["kAr"] = "O18";
    codes["zH"] = "O22";
    codes["txn"] = "O25";
    codes["iwn"] = "O28";
    codes["aAv"] = "O29a";
    codes["O29v"] = "O29a";
    codes["aA"] = "O29";
    codes["zxnt"] = "O30";
    codes["zb"] = "O35";
    codes["inb"] = "O36";
    codes["Szp"] = "O42";
    codes["ipt"] = "O45";
    codes["nxn"] = "O47";
    codes["niwt"] = "O49";
    codes["zp"] = "O50";
    codes["Snwt"] = "O51";
    codes["wHa"] = "P4";
    codes["TAw"] = "P5";
    codes["nfw"] = "P5";
    codes["aHa"] = "P6";
    codes["xrw"] = "P8";
    codes["st"] = "Q1";
    codes["wz"] = "Q2";
    codes["qrsw"] = "Q6";
    codes["qrs"] = "Q6";
    codes["xAwt"] = "R1";
    codes["xAt"] = "R1";
    codes["Htp"] = "R4";
    codes["kAp"] = "R5";
    codes["kp"] = "R5";
    codes["snTr"] = "R7";
    codes["nTr"] = "R8";
    codes["bd"] = "R9";
    codes["dd"] = "R11";
    codes["Dd"] = "R11";
    codes["imnt"] = "R14";
    codes["iAb"] = "R15";
    codes["wx"] = "R16";
    codes["xm"] = "R22";
    codes["HDt"] = "S1";
    codes["dSrt"] = "S3";
    codes["sxmty"] = "S6";
    codes["xprS"] = "S7";
    codes["Atf"] = "S8";
    codes["Swty"] = "S9";
    codes["mDH"] = "S10";
    codes["wsx"] = "S11";
    codes["nbw"] = "S12";
    codes["tHn"] = "S15";
    codes["THn"] = "S15";
    codes["mnit"] = "S18";
    codes["sDAw"] = "S19";
    codes["xtm"] = "S20";
    codes["sT"] = "S22";
    codes["dmD"] = "S23";
    codes["Tz"] = "S24";
    codes["Sndyt"] = "S26";
    codes["mnxt"] = "S27";
    codes["sf"] = "S30";
    codes["siA"] = "S32";
    codes["Tb"] = "S33";
    codes["anx"] = "S34";
    codes["Swt"] = "S35";
    codes["xw"] = "S37";
    codes["HqA"] = "S38";
    codes["awt"] = "S39";
    codes["wAs"] = "S40";
    codes["Dam"] = "S41";
    codes["abA"] = "S42";
    codes["sxm"] = "S42";
    codes["xrp"] = "S42";
    codes["md"] = "S43";
    codes["Ams"] = "S44";
    codes["nxxw"] = "S45";
    codes["HD"] = "T3";
    codes["HDD"] = "T6";
    codes["pd"] = "T9";
    codes["pD"] = "T10";
    codes["zin"] = "T11";
    codes["zwn"] = "T11";
    codes["sXr"] = "T11";
    codes["Ai"] = "T12";
    codes["Ar"] = "T12";
    codes["rwd"] = "T12";
    codes["rwD"] = "T12";
    codes["rs"] = "T13";
    codes["qmA"] = "T14";
    codes["wrrt"] = "T17";
    codes["Sms"] = "T18";
    codes["qs"] = "T19";
    codes["wa"] = "T21";
    codes["sn"] = "T22";
    codes["iH"] = "T24";
    codes["DbA"] = "T25";
    codes["Xr"] = "T28";
    codes["nmt"] = "T29";
    codes["sSm"] = "T31";
    codes["nm"] = "T34";
    codes["mA"] = "U1";
    codes["mr"] = "U6";
    codes["it"] = "U10";
    codes["HqAt"] = "U11";
    codes["hb"] = "U13";
    codes["Sna"] = "U13";
    codes["tm"] = "U15";
    codes["biA"] = "U16";
    codes["grg"] = "U17";
    codes["stp"] = "U21";
    codes["mnx"] = "U22";
    codes["Ab"] = "U23";
    codes["Hmt"] = "U24";
    codes["wbA"] = "U26";
    codes["DA"] = "U28";
    codes["rtH"] = "U31";
    codes["zmn"] = "U32";
    codes["ti"] = "U33";
    codes["xsf"] = "U34";
    codes["Hm"] = "U36";
    codes["mxAt"] = "U38";
    codes["St"] = "V1";
    codes["Snt"] = "V1";
    codes["100"] = "V1";
    codes["sTA"] = "V2";
    codes["sTAw"] = "V3";
    codes["wA"] = "V4";
    codes["snT"] = "V5";
    codes["Sn"] = "V7";
    codes["arq"] = "V12";
    codes["iTi"] = "V15";
    codes["mDt"] = "V19";
    codes["XAr"] = "V19";
    codes["TmA"] = "V19";
    codes["10"] = "V20";
    codes["mD"] = "V20";
    codes["mH"] = "V22";
    codes["wD"] = "V24";
    codes["aD"] = "V26";
    codes["wAH"] = "V29";
    codes["sk"] = "V29";
    codes["nb"] = "V30";
    codes["msn"] = "V32";
    codes["sSr"] = "V33";
    codes["idr"] = "V37";
    codes["bAs"] = "W2";
    codes["Hb"] = "W3a";
    codes["Xnm"] = "W9";
    codes["iab"] = "W10";
    codes["nst"] = "W11";
    codes["Hz"] = "W14";
    codes["xnt"] = "W17";
    codes["mi"] = "W19";
    codes["Hnqt"] = "W22";
    codes["nw"] = "W24";
    codes["ini"] = "W25";
    codes["rdi"] = "X8";
    codes["di"] = "X8";
    codes["Y1v"] = "Y1a";
    codes["mDAt"] = "Y1";
    codes["mnhd"] = "Y3";
    codes["mn"] = "Y5";
    codes["ibA"] = "Y6";
    codes["zSSt"] = "Y8";
    codes["imi"] = "Z11";
    codes["wnm"] = "Z11";
    codes["`"] = "Z14";
    codes["Hp"] = "J5";
    codes["qn"] = "J8";
    codes["mAa"] = "J11";
    codes["im"] = "J13";
    codes["gs"] = "J13";
    codes["sA"] = "J17";
    codes["apr"] = "J20";
    codes["wDa"] = "J21";
    codes["nD"] = "J27";
    codes["qd"] = "J28";
    codes["Xkr"] = "J30";
    codes["2"] = "Z15a";
    codes["3"] = "Z15b";
    codes["4"] = "Z15c";
    codes["5"] = "Z15d";
    codes["6"] = "Z15e";
    codes["7"] = "Z15f";
    codes["8"] = "Z15g";
    codes["9"] = "Z15h";
    codes["nn"] = "M22a";

    auto const &table = GlyphNames::GetTable();
    for (auto const &[cp, name] : table) {
      codes[name] = name;
    }

    codes[":"] = "vj";
    codes["*"] = "hj";
    codes["("] = "ss";
    codes[")"] = "se";
    codes["<"] = "cb";
    codes[">"] = "ce";

    map<char, shared_ptr<Glyph>> single;
    map<size_t, map<string, shared_ptr<Glyph>>> ligature;
    for (auto const &[code, name] : codes) {
      auto glyph = getGlyphByName(name);
      if (code.size() == 1) {
        single[code[0]] = glyph;
      } else {
        ligature[code.size()][code] = glyph;
      }
    }

    auto singleLookup = getLookupByName("mdc002");
    singleLookup->base = Lookup::ProcessBase{};
    singleLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
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

    auto ligatureLookup = getLookupByName("mdc001");
    ligatureLookup->base = Lookup::ProcessBase{};
    ligatureLookup->marks = Lookup::ProcessMarks(Lookup::ProcessMarks::All{});
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

    lookups.erase(ranges::remove_if(lookups, [&](auto const &it) { return it.second == singleLookup || it.second == ligatureLookup; }).begin(), lookups.end());
    lookups.insert(lookups.begin(), make_pair(singleLookup->name, singleLookup));
    lookups.insert(lookups.begin(), make_pair(ligatureLookup->name, ligatureLookup));

    auto feature = make_shared<Feature>("Ligature", FCC("liga"));
    feature->lookups.push_back(ligatureLookup);
    feature->lookups.push_back(singleLookup);

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

  Status postprocess() {
    if (auto st = insertMdCLookup(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
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
  // unit per vertical grid
  int16_t vfu;
  std::unordered_map<std::string, SizeVariants> sizeVariants;
  // font side bearings: hfu / 3
  int16_t sb;
  int16_t base;
};

} // namespace eglyf
