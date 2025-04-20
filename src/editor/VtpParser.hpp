#pragma once

namespace eglyf {

class VtpParser {
private:
  struct GroupBuilder {
    GroupBuilder(std::shared_ptr<Editor> const &editor, std::shared_ptr<Editor::Group> const &group) : editor(editor), group(group) {
    }

    std::shared_ptr<Editor::Group> endGroup() {
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
    std::shared_ptr<Editor::Group> group;
  };

  std::shared_ptr<GroupBuilder> defineGroup(std::string const &name) {
    using namespace std;
    auto g = editor->getGroupByName(name);
    return make_shared<GroupBuilder>(editor, g);
  }

  void defineAnchor(std::string const &name, std::string const &glyph, std::optional<int16_t> dx, std::optional<int16_t> dy) {
    using namespace std;
    auto a = editor->getAnchorByName(name);
    a->name = name;
    if (auto g = editor->getGlyphByName(glyph); g) {
      a->glyphs[g] = Vec<optional<int16_t>>(dx, dy);
    }
  }

  Status defineGlyph(std::string const &name, std::optional<uint32_t> unicode, GlyphDefinitionTable::Class classDef) {
    using namespace std;
    auto g = editor->getGlyphByName(name);
    auto font = editor->font;
    uint16_t glyphID = 0;
    if (auto gid = font->post->getGlyphID(name); gid) {
      glyphID = *gid;
    } else {
      auto underbar = name.find('_');
      if (underbar != string::npos) {
        auto prefix = name.substr(0, underbar);
        if (GlyphNames::IsNamedGlyph(prefix)) {
          return Status::Ok();
        }
      }
      if (auto gid1 = font->addEmptyGlyph(name, 0, 0); gid1) {
        glyphID = *gid1;
      } else {
        return EGLYF_STATUS_PUSH(gid1.status());
      }
    }
    if (unicode) {
      if (auto st = font->cmap->map(*unicode, glyphID); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    if (!font->gdef) {
      font->gdef = make_shared<GlyphDefinitionTable>();
      font->gdef->majorVersion = 1;
      font->gdef->minorVersion = 2;
    }
    if (!font->gdef->glyphClassDef) {
      font->gdef->glyphClassDef = make_shared<ClassDef>();
    }
    if (auto st = font->gdef->glyphClassDef->add(glyphID, static_cast<uint16_t>(classDef)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    g->id = glyphID;
    g->classDef = classDef;
    return Status::Ok();
  }

  std::shared_ptr<Editor::Script> defineScript(std::string const &name, Tag const &tag) {
    using namespace std;
    auto s = editor->getScriptByName(name);
    s->tag = tag;
    return s;
  }

  std::shared_ptr<Editor::LangSys> defineLangSys(std::shared_ptr<Editor::Script> script, std::string const &name, Tag const &tag) {
    using namespace std;
    auto ls = editor->getLangSysByName(script, name);
    ls->tag = tag;
    return ls;
  }

  std::shared_ptr<Editor::Feature> defineFeature(std::shared_ptr<Editor::LangSys> langsys, std::string const &name, Tag const &tag) {
    using namespace std;
    auto f = editor->getFeatureByName(langsys, name);
    f->tag = tag;
    return f;
  }

public:
  explicit VtpParser(std::shared_ptr<Editor> const &editor) : editor(editor) {
  }

  // Parse VTP data from string_view
  Status parseVtp(std::string_view vtpContent) {
    using namespace std;
    // Split into lines
    vector<string_view> lines;

    size_t pos = 0;
    size_t found;
    while ((found = vtpContent.find('\n', pos)) != string_view::npos) {
      lines.push_back(vtpContent.substr(pos, found - pos));
      pos = found + 1;
    }
    if (pos < vtpContent.size()) {
      lines.push_back(vtpContent.substr(pos));
    }

    // Parse lines
    for (size_t i = 0; i < lines.size();) {
      string_view const &l = lines[i];
      if (l.starts_with("DEF_LOOKUP")) {
        auto status = parseLookup(lines, i);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
      } else if (l.starts_with("DEF_GLYPH") && l.ends_with("END_GLYPH")) {
        auto status = parseGlyph(l);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        i++;
      } else if (l.starts_with("DEF_GROUP")) {
        auto status = parseGroup(lines, i);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
      } else if (l.starts_with("DEF_ANCHOR")) {
        auto status = parseAnchor(l);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        i++;
      } else if (l.starts_with("DEF_SCRIPT")) {
        auto status = parseScript(lines, i);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
      } else if (l.empty()) {
        i++;
      } else if (l.starts_with("GRID_PPEM") || l.starts_with("PRESENTATION_PPEM") || l.starts_with("PPOSITIONING_PPEM") || l.starts_with("COMPILER_USEEXTENSIONLOOKUPS") || l.starts_with("DO_NOT_TOUCH_CMAP") || l.starts_with("CMAP_FORMAT")) {
        i++;
      } else {
        return EGLYF_ERROR_WHAT("Unimplemented vtp element");
      }
    }

    return Status::Ok();
  }

private:
  // Parse functions for each section
  Status parseLookup(std::vector<std::string_view> const &lines, size_t &index) {
    using namespace std;

    auto first = lines[index];
    if (first.find("DEF_LOOKUP") != 0) {
      return EGLYF_ERROR_WHAT("Expected DEF_LOOKUP");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 4) {
      return EGLYF_ERROR_WHAT("Invalid DEF_LOOKUP format");
    }

    // Parse tokens
    tokens.pop_front(); // Remove "DEF_LOOKUP"

    // Get lookup name
    if (tokens.empty()) {
      return EGLYF_ERROR_WHAT("Missing lookup name");
    }
    auto name = unquote(tokens[0]);
    auto lookup = editor->getLookupByName(string(name));
    tokens.pop_front(); // Remove name

    // PROCESS_BASE or SKIP_BASE
    if (tokens.empty()) {
      return EGLYF_ERROR_WHAT("Missing base type");
    }
    auto baseType = tokens[0];
    if (baseType == "PROCESS_BASE") {
      lookup->base = Editor::Lookup::ProcessBase{};
    } else if (baseType == "SKIP_BASE") {
      lookup->base = Editor::Lookup::SkipBase{};
    } else {
      return EGLYF_ERROR_WHAT("Invalid base type: " + string(baseType));
    }
    tokens.pop_front(); // Remove base type

    // PROCESS_MARKS or SKIP_MARKS
    if (tokens.empty()) {
      return EGLYF_ERROR_WHAT("Missing marks type");
    }
    auto marksType = tokens[0];
    tokens.pop_front(); // Remove marks type

    if (marksType == "PROCESS_MARKS") {
      if (tokens.empty()) {
        return EGLYF_ERROR_WHAT("Missing PROCESS_MARKS type");
      }
      auto marksWhat = tokens[0];
      tokens.pop_front(); // Remove marks what

      if (marksWhat == "MARK_GLYPH_SET") {
        if (tokens.empty()) {
          return EGLYF_ERROR_WHAT("Missing glyph name for MARK_GLYPH_SET");
        }
        auto groupName = unquote(tokens[0]);
        auto group = editor->getGroupByName(string(groupName));
        lookup->marks = Editor::Lookup::ProcessMarks(Editor::Lookup::ProcessMarks::MarkFilteringSet{group});
        tokens.pop_front(); // Remove glyph name
      } else if (marksWhat == "ALL" || marksWhat == "\"ALL\"") {
        lookup->marks = Editor::Lookup::ProcessMarks(Editor::Lookup::ProcessMarks::All{});
      } else if (marksWhat.starts_with('"')) {
        // Group name
        auto groupName = unquote(marksWhat);
        auto group = editor->getGroupByName(string(groupName));
        lookup->marks = Editor::Lookup::ProcessMarks(Editor::Lookup::ProcessMarks::MarkGroup{group});
      } else {
        return EGLYF_ERROR_WHAT("Invalid PROCESS_MARKS type: " + string(marksWhat));
      }
    } else if (marksType == "SKIP_MARKS") {
      lookup->marks = Editor::Lookup::SkipMarks{};
    } else {
      return EGLYF_ERROR_WHAT("Invalid marks type: " + string(marksType));
    }

    // DIRECTION
    if (tokens.size() != 2 || tokens[0] != "DIRECTION") {
      return EGLYF_ERROR_WHAT("Missing DIRECTION");
    }

    auto direction = tokens[1];
    if (direction != "LTR") {
      return EGLYF_ERROR_WHAT("Unsupported direction: " + string(direction));
    }

    // Parse from next line
    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l == "EXCEPT_CONTEXT") {
        auto context = make_shared<Editor::Lookup::Context>(
            initializer_list<variant<shared_ptr<Editor::Glyph>, shared_ptr<Editor::Group>>>{},
            initializer_list<variant<shared_ptr<Editor::Glyph>, shared_ptr<Editor::Group>>>{});
        auto status = parseContext(lines, i, *context);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        if (!context->left.empty() || !context->right.empty()) {
          lookup->exceptContexts.push_back(context);
        }
      } else if (l == "IN_CONTEXT") {
        auto context = make_shared<Editor::Lookup::Context>(
            initializer_list<variant<shared_ptr<Editor::Glyph>, shared_ptr<Editor::Group>>>{},
            initializer_list<variant<shared_ptr<Editor::Glyph>, shared_ptr<Editor::Group>>>{});
        auto status = parseContext(lines, i, *context);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        if (!context->left.empty() || !context->right.empty()) {
          lookup->inContexts.push_back(context);
        }
      } else if (l == "AS_SUBSTITUTION") {
        lookup->substitutions.clear();
        auto status = parseSubstitution(lines, i, *lookup);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        index = i;
        return Status::Ok();
      } else if (l == "AS_POSITION") {
        auto status = parsePosition(lines, i, *lookup);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        index = i;
        return Status::Ok();
      } else {
        i++;
      }
    }

    index = lines.size();
    return Status::Ok();
  }

  Status parseContext(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup::Context &context) {
    using namespace std;

    auto first = trim(lines[index]);
    if (first != "EXCEPT_CONTEXT" && first != "IN_CONTEXT") {
      return EGLYF_ERROR_WHAT("Expected EXCEPT_CONTEXT or IN_CONTEXT");
    }

    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);
      if (l == "END_CONTEXT") {
        index = i + 1;
        return Status::Ok();
      } else {
        auto tokens = splitString(l);

        for (size_t j = 0; j < tokens.size();) {
          auto op = tokens[j++];

          if (op == "LEFT") {
            if (j + 1 >= tokens.size()) {
              return EGLYF_ERROR_WHAT("Invalid LEFT format");
            }

            auto type = tokens[j++];
            auto name = tokens[j++];

            if (type == "GLYPH") {
              auto glyph = editor->getGlyphByName(string(unquote(name)));
              context.left.push_back(glyph);
            } else if (type == "GROUP") {
              auto group = editor->getGroupByName(string(unquote(name)));
              context.left.push_back(group);
            } else {
              return EGLYF_ERROR_WHAT("Invalid LEFT type: " + string(type));
            }
          } else if (op == "RIGHT") {
            if (j + 1 >= tokens.size()) {
              return EGLYF_ERROR_WHAT("Invalid RIGHT format");
            }

            auto type = tokens[j++];
            auto name = tokens[j++];

            if (type == "GLYPH") {
              auto glyph = editor->getGlyphByName(string(unquote(name)));
              context.right.push_back(glyph);
            } else if (type == "GROUP") {
              auto group = editor->getGroupByName(string(unquote(name)));
              context.right.push_back(group);
            } else {
              return EGLYF_ERROR_WHAT("Invalid RIGHT type: " + string(type));
            }
          } else {
            return EGLYF_ERROR_WHAT("Invalid context operation: " + string(op));
          }
        }

        i++;
      }
    }

    index = lines.size();
    return Status::Ok();
  }

  Status parseSubstitution(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup &lookup) {
    using namespace std;

    auto first = trim(lines[index]);
    if (first != "AS_SUBSTITUTION") {
      return EGLYF_ERROR_WHAT("Expected AS_SUBSTITUTION");
    }

    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l == "END_SUBSTITUTION") {
        index = i + 1;
        return Status::Ok();
      } else if (l.find("SUB ") == 0) {
        auto subst = make_shared<Editor::Lookup::Substitution>();
        auto status = parseSub(lines, i, *subst);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        lookup.substitutions.push_back(subst);
      } else {
        return EGLYF_ERROR_WHAT("Invalid substitution line: " + string(l));
      }
    }

    index = lines.size();
    return Status::Ok();
  }

  Status parseSub(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup::Substitution &subst) {
    using namespace std;

    auto first = trim(lines[index]);
    auto tokens = splitString(first);

    if (tokens.empty() || tokens[0] != "SUB") {
      return EGLYF_ERROR_WHAT("Expected SUB");
    }

    tokens.pop_front(); // Remove "SUB"

    // Parse input glyphs/groups
    for (size_t i = 0; i < tokens.size(); i += 2) {
      if (i + 1 >= tokens.size()) {
        return EGLYF_ERROR_WHAT("Invalid SUB format");
      }

      auto type = tokens[i];
      auto name = tokens[i + 1];

      if (type == "GLYPH") {
        auto glyph = editor->getGlyphByName(string(unquote(name)));
        subst.input.push_back(glyph);
      } else if (type == "GROUP") {
        auto group = editor->getGroupByName(string(unquote(name)));
        subst.input.push_back(group);
      } else {
        return EGLYF_ERROR_WHAT("Invalid SUB type: " + string(type));
      }
    }

    // Parse WITH line
    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l.find("WITH") == 0) {
        auto tokens = splitString(l);
        tokens.pop_front(); // Remove "WITH"

        for (size_t j = 0; j < tokens.size(); j += 2) {
          if (j + 1 >= tokens.size()) {
            return EGLYF_ERROR_WHAT("Invalid WITH format");
          }

          auto type = tokens[j];
          auto name = tokens[j + 1];

          if (type == "GLYPH") {
            auto glyph = editor->getGlyphByName(string(unquote(name)));
            subst.output.push_back(glyph);
          } else if (type == "GROUP") {
            auto group = editor->getGroupByName(string(unquote(name)));
            subst.output.push_back(group);
          } else {
            return EGLYF_ERROR_WHAT("Invalid WITH type: " + string(type));
          }
        }

        i++;
      } else if (l == "END_SUB") {
        index = i + 1;
        return Status::Ok();
      } else {
        return EGLYF_ERROR_WHAT("Invalid SUB line: " + string(l));
      }
    }

    index = lines.size();
    return EGLYF_ERROR_WHAT("Missing END_SUB");
  }

  Status parseAttach(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup::Attach &attach) {
    using namespace std;

    auto first = trim(lines[index]);
    auto tokens = splitString(first);

    if (tokens.empty() || tokens[0] != "ATTACH") {
      return EGLYF_ERROR_WHAT("Expected ATTACH");
    }
    tokens.pop_front();

    if (tokens.size() < 2) {
      return EGLYF_ERROR_WHAT("Invalid ATTACH format");
    }

    while (tokens.size() > 1) {
      auto type = tokens.front();
      tokens.pop_front();
      auto name = tokens.front();
      tokens.pop_front();

      if (type == "GLYPH") {
        auto glyph = editor->getGlyphByName(string(unquote(name)));
        attach.receptors.push_back(glyph);
      } else if (type == "GROUP") {
        auto group = editor->getGroupByName(string(unquote(name)));
        attach.receptors.push_back(group);
      } else {
        return EGLYF_ERROR_WHAT("Invalid ATTACH type: " + string(type));
      }
    }
    if (!tokens.empty()) {
      return EGLYF_ERROR;
    }

    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l == "END_ATTACH") {
        index = i + 1;
        return Status::Ok();
      } else if (l.find("TO GROUP") == 0 || l.find("TO GLYPH") == 0 || l.find("GROUP") == 0 || l.find("GLYPH") == 0) {
        auto tokens = splitString(l);

        size_t j = 0;
        if (tokens[0] == "TO") {
          j = 1;
        }

        auto type = tokens[j++];
        auto name = tokens[j++];

        if (j + 2 >= tokens.size() || tokens[j] != "AT" || tokens[j + 1] != "ANCHOR") {
          return EGLYF_ERROR_WHAT("Invalid ATTACH target format");
        }

        auto anchorName = tokens[j + 2];

        variant<shared_ptr<Editor::Glyph>, shared_ptr<Editor::Group>> target;

        if (type == "GLYPH") {
          target = editor->getGlyphByName(string(unquote(name)));
        } else if (type == "GROUP") {
          target = editor->getGroupByName(string(unquote(name)));
        } else {
          return EGLYF_ERROR_WHAT("Invalid ATTACH target type: " + string(type));
        }

        auto anchor = editor->getAnchorByName(string(unquote(anchorName)));
        attach.ligands.push_back(Editor::Lookup::AttachLigand(target, anchor));

        i++;
      } else {
        return EGLYF_ERROR_WHAT("Invalid ATTACH line: " + string(l));
      }
    }

    index = lines.size();
    return EGLYF_ERROR_WHAT("Missing END_ATTACH");
  }

  Status parseAdjustSingle(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup::AdjustSingle &adjustSingle) {
    using namespace std;

    auto first = trim(lines[index]);
    auto tokens = splitString(first);

    if (tokens.empty() || tokens[0] != "ADJUST_SINGLE") {
      return EGLYF_ERROR_WHAT("Expected ADJUST_SINGLE");
    }

    tokens.pop_front(); // Remove "ADJUST_SINGLE"

    while (!tokens.empty()) {
      if (tokens.size() < 5) {
        return EGLYF_ERROR_WHAT("Invalid ADJUST_SINGLE format");
      }

      auto what = tokens[0];
      auto name = tokens[1];

      if (tokens[2] != "BY" || tokens[3] != "POS") {
        return EGLYF_ERROR_WHAT("Invalid ADJUST_SINGLE format, expected BY POS");
      }

      optional<int16_t> dx;
      optional<int16_t> dy;

      size_t i = 4;
      while (i < tokens.size()) {
        auto type = tokens[i++];

        if (type == "END_POS") {
          break;
        }

        if (i >= tokens.size()) {
          return EGLYF_ERROR_WHAT("Invalid ADJUST_SINGLE format, missing value");
        }

        auto value = tokens[i++];

        if (type == "DX") {
          dx = static_cast<int16_t>(stoi(string(value)));
        } else if (type == "DY") {
          dy = static_cast<int16_t>(stoi(string(value)));
        } else {
          return EGLYF_ERROR_WHAT("Invalid ADJUST_SINGLE type: " + string(type));
        }
      }

      if (what == "GLYPH") {
        auto glyph = editor->getGlyphByName(string(unquote(name)));
        adjustSingle.glyphs.push_back(Editor::Lookup::AdjustGlyph(glyph, dx, dy));
      } else {
        return EGLYF_ERROR_WHAT("Invalid ADJUST_SINGLE what: " + string(what));
      }

      // Process remaining tokens
      if (i < tokens.size()) {
        tokens.erase(tokens.begin(), tokens.begin() + i);
      } else {
        tokens.clear();
      }
    }

    auto end = trim(lines[index + 1]);
    if (end != "END_ADJUST") {
      return EGLYF_ERROR_WHAT("Expected END_ADJUST");
    }

    index = index + 2;
    return Status::Ok();
  }

  Status parseGroup(std::vector<std::string_view> const &lines, size_t &index) {
    using namespace std;

    // Line 1: DEF_GROUP "name"
    auto first = lines[index];
    if (!first.starts_with("DEF_GROUP")) {
      return EGLYF_ERROR_WHAT("Expected DEF_GROUP");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 2) {
      return EGLYF_ERROR_WHAT("Invalid DEF_GROUP format");
    }

    // Parse tokens
    tokens.pop_front(); // Remove "DEF_GROUP"

    auto name = unquote(tokens[0]);

    // Define the group
    auto groupBuilder = defineGroup(string(name));

    // Line 2: ENUM GROUP "group1" GLYPH "glyph1" ... END_ENUM
    if (index + 1 >= lines.size()) {
      return EGLYF_ERROR_WHAT("Missing ENUM line in DEF_GROUP");
    }

    auto enumLine = trim(lines[index + 1]);
    auto enumTokens = splitString(enumLine);

    if (enumTokens.empty() || enumTokens[0] != "ENUM") {
      return EGLYF_ERROR_WHAT("Expected ENUM in DEF_GROUP");
    }

    // Begin ENUM processing
    groupBuilder->beginEnum();

    // Process ENUM content
    for (size_t j = 1; j < enumTokens.size(); j++) {
      if (enumTokens[j] == "END_ENUM") {
        // End ENUM processing
        groupBuilder->endEnum();
        break;
      } else if (j + 1 < enumTokens.size()) {
        if (enumTokens[j] == "GROUP" || enumTokens[j] == "GLYPH") {
          auto type = enumTokens[j];
          auto itemName = unquote(enumTokens[j + 1]);
          j++; // Skip name

          if (type == "GROUP") {
            groupBuilder->addGroup(string(itemName));
          } else if (type == "GLYPH") {
            groupBuilder->addGlyph(string(itemName));
          }
        }
      }
    }

    // Line 3: END_GROUP
    if (index + 2 >= lines.size()) {
      return EGLYF_ERROR_WHAT("Missing END_GROUP");
    }

    auto endLine = trim(lines[index + 2]);
    if (endLine != "END_GROUP") {
      return EGLYF_ERROR_WHAT("Expected END_GROUP");
    }

    // Processing complete
    index = index + 3;
    return Status::Ok();
  }

  Status parseAnchor(std::string_view line) {
    using namespace std;

    if (!line.starts_with("DEF_ANCHOR") || !line.ends_with("END_ANCHOR")) {
      return EGLYF_ERROR_WHAT("Expected DEF_ANCHOR");
    }

    auto tokens = splitString(trim(line));
    if (tokens.size() < 10) { // Minimum tokens needed for a valid DEF_ANCHOR
      return EGLYF_ERROR_WHAT("Invalid DEF_ANCHOR format");
    }

    // Parse tokens
    // DEF_ANCHOR "a1" ON 469 GLYPH QB1 COMPONENT 1 AT POS DX 105 DY 1860 END_POS END_ANCHOR

    // Get anchor name
    auto name = unquote(tokens[1]);

    // Find GLYPH token
    size_t glyphIndex = 0;
    for (size_t i = 2; i < tokens.size(); i++) {
      if (tokens[i] == "GLYPH") {
        glyphIndex = i;
        break;
      }
    }

    if (glyphIndex == 0 || glyphIndex + 1 >= tokens.size()) {
      return EGLYF_ERROR_WHAT("Missing GLYPH in DEF_ANCHOR");
    }

    auto glyphName = unquote(tokens[glyphIndex + 1]);

    // Find POS token
    size_t posIndex = 0;
    for (size_t i = glyphIndex + 2; i < tokens.size(); i++) {
      if (tokens[i] == "POS") {
        posIndex = i;
        break;
      }
    }

    if (posIndex == 0 || posIndex + 1 >= tokens.size()) {
      return EGLYF_ERROR_WHAT("Missing POS in DEF_ANCHOR");
    }

    // Parse DX and DY values
    optional<int16_t> dx = nullopt;
    optional<int16_t> dy = nullopt;

    for (size_t i = posIndex + 1; i < tokens.size(); i++) {
      if (tokens[i] == "END_POS") {
        break;
      }

      if (tokens[i] == "DX" && i + 1 < tokens.size()) {
        dx = static_cast<int16_t>(stoi(string(tokens[i + 1])));
        i++; // Skip value
      } else if (tokens[i] == "DY" && i + 1 < tokens.size()) {
        dy = static_cast<int16_t>(stoi(string(tokens[i + 1])));
        i++; // Skip value
      }
    }

    // Define the anchor
    defineAnchor(string(name), string(glyphName), dx, dy);

    return Status::Ok();
  }

  Status parseGlyph(std::string_view first) {
    using namespace std;

    if (!first.starts_with("DEF_GLYPH") || !first.ends_with("END_GLYPH")) {
      return EGLYF_ERROR_WHAT("Expected DEF_GLYPH");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 5) {
      return EGLYF_ERROR_WHAT("Invalid DEF_GLYPH format");
    }

    // Parse tokens
    tokens.pop_front(); // Remove "DEF_GLYPH"

    auto name = unquote(tokens[0]);

    // Skip "ID" token
    if (tokens[1] != "ID") {
      return EGLYF_ERROR_WHAT("Expected ID in DEF_GLYPH");
    }

    // Parse ID (not used directly, but skip it)
    tokens.erase(tokens.begin(), tokens.begin() + 3); // Remove name, "ID", and ID value

    optional<uint32_t> unicode;
    GlyphDefinitionTable::Class classDef;

    // Check if UNICODE is present
    if (tokens[0] == "UNICODE") {
      if (tokens.size() < 3) {
        return EGLYF_ERROR_WHAT("Invalid UNICODE format in DEF_GLYPH");
      }

      unicode = static_cast<uint32_t>(stoul(string(tokens[1])));
      tokens.erase(tokens.begin(), tokens.begin() + 2); // Remove "UNICODE" and value
    }

    // Parse TYPE
    if (tokens[0] != "TYPE") {
      return EGLYF_ERROR_WHAT("Expected TYPE in DEF_GLYPH");
    }

    if (tokens.size() < 2) {
      return EGLYF_ERROR_WHAT("Missing type value in DEF_GLYPH");
    }

    auto type = tokens[1];
    if (type == "MARK") {
      classDef = GlyphDefinitionTable::Class::Mark;
    } else if (type == "BASE") {
      classDef = GlyphDefinitionTable::Class::Base;
    } else {
      return EGLYF_ERROR_WHAT("Invalid type in DEF_GLYPH: " + string(type));
    }

    // Define the glyph
    if (auto st = defineGlyph(string(name), unicode, classDef); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    return Status::Ok();
  }

  Status parsePosition(std::vector<std::string_view> const &lines, size_t &index, Editor::Lookup &lookup) {
    using namespace std;

    auto first = trim(lines[index]);
    if (first != "AS_POSITION") {
      return EGLYF_ERROR_WHAT("Expected AS_POSITION");
    }

    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l == "END_POSITION") {
        index = i + 1;
        return Status::Ok();
      } else if (l.find("ATTACH") == 0) {
        auto attach = make_shared<Editor::Lookup::Attach>();
        auto status = parseAttach(lines, i, *attach);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        lookup.attach = attach;
      } else if (l.find("ADJUST_SINGLE") == 0) {
        auto adjustSingle = make_shared<Editor::Lookup::AdjustSingle>();
        auto status = parseAdjustSingle(lines, i, *adjustSingle);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
        lookup.adjustSingle = adjustSingle;
      } else {
        return EGLYF_ERROR_WHAT("Invalid position line: " + string(l));
      }
    }

    index = lines.size();
    return Status::Ok();
  }

  // Utility functions
  std::string_view trim(std::string_view s) {
    using namespace std;
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start])) {
      start++;
    }

    size_t end = s.size();
    while (end > start && std::isspace(s[end - 1])) {
      end--;
    }

    return s.substr(start, end - start);
  }

  std::string_view unquote(std::string_view s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
      return s.substr(1, s.size() - 2);
    }
    return s;
  }

  std::deque<std::string_view> splitString(std::string_view s, char delimiter = ' ') {
    using namespace std;
    deque<string_view> tokens;

    size_t start = 0;
    bool inQuotes = false;

    for (size_t i = 0; i < s.size(); i++) {
      // Toggle quote state
      if (s[i] == '"') {
        inQuotes = !inQuotes;
      }

      // If we find a delimiter (only when not inside quotes)
      if (s[i] == delimiter && !inQuotes) {
        if (i > start) {
          tokens.push_back(s.substr(start, i - start));
        }
        start = i + 1;
      }
    }

    // Add the last token
    if (start < s.size()) {
      tokens.push_back(s.substr(start));
    }

    return tokens;
  }

  Status parseScript(std::vector<std::string_view> const &lines, size_t &index) {
    using namespace std;

    auto first = lines[index];
    if (!first.starts_with("DEF_SCRIPT")) {
      return EGLYF_ERROR_WHAT("Expected DEF_SCRIPT");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 4) {
      return EGLYF_ERROR_WHAT("Invalid DEF_SCRIPT format");
    }

    // Parse tokens
    if (tokens[1] != "NAME") {
      return EGLYF_ERROR_WHAT("Expected NAME in DEF_SCRIPT");
    }

    auto name = unquote(tokens[2]);

    if (tokens[3] != "TAG") {
      return EGLYF_ERROR_WHAT("Expected TAG in DEF_SCRIPT");
    }

    auto tagString = unquote(tokens[4]);
    if (tagString.size() != 4) {
      return EGLYF_ERROR_WHAT("Invalid string length for tag");
    }
    Tag tag;
    ranges::copy(tagString, tag.begin());

    // Define the script
    auto script = defineScript(string(name), tag);

    // Parse subsequent lines until END_SCRIPT
    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l.starts_with("DEF_LANGSYS")) {
        auto status = parseLangsys(lines, i, script);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
      } else if (l == "END_SCRIPT") {
        index = i + 1;
        return Status::Ok();
      } else {
        return EGLYF_ERROR_WHAT("Unexpected line in DEF_SCRIPT: " + string(l));
      }
    }

    return EGLYF_ERROR_WHAT("Missing END_SCRIPT");
  }

  Status parseLangsys(std::vector<std::string_view> const &lines, size_t &index, std::shared_ptr<Editor::Script> script) {
    using namespace std;

    auto first = lines[index];
    if (!first.starts_with("DEF_LANGSYS")) {
      return EGLYF_ERROR_WHAT("Expected DEF_LANGSYS");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 4) {
      return EGLYF_ERROR_WHAT("Invalid DEF_LANGSYS format");
    }

    // Parse tokens
    // DEF_LANGSYS NAME "Default" TAG "dflt"
    if (tokens[1] != "NAME") {
      return EGLYF_ERROR_WHAT("Expected NAME in DEF_LANGSYS");
    }

    auto name = unquote(tokens[2]);

    if (tokens[3] != "TAG") {
      return EGLYF_ERROR_WHAT("Expected TAG in DEF_LANGSYS");
    }

    auto tagString = unquote(tokens[4]);
    if (tagString.size() != 4) {
      return EGLYF_ERROR_WHAT("Invalid string length for tag");
    }
    Tag tag;
    ranges::copy(tagString, tag.begin());

    // Define the langsys
    auto langsys = defineLangSys(script, string(name), tag);

    // Parse subsequent lines until END_LANGSYS
    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l.starts_with("DEF_FEATURE")) {
        auto status = parseFeature(lines, i, langsys);
        if (!status.ok()) {
          return EGLYF_STATUS_PUSH(status);
        }
      } else if (l == "END_LANGSYS") {
        index = i + 1;
        return Status::Ok();
      } else if (l.empty()) {
        i++;
      } else {
        return EGLYF_ERROR_WHAT("Unexpected line in DEF_LANGSYS: " + string(l));
      }
    }

    return EGLYF_ERROR_WHAT("Missing END_LANGSYS");
  }

  Status parseFeature(std::vector<std::string_view> const &lines, size_t &index, std::shared_ptr<Editor::LangSys> langsys) {
    using namespace std;

    auto first = lines[index];
    if (!first.starts_with("DEF_FEATURE")) {
      return EGLYF_ERROR_WHAT("Expected DEF_FEATURE");
    }

    auto tokens = splitString(trim(first));
    if (tokens.size() < 4) {
      return EGLYF_ERROR_WHAT("Invalid DEF_FEATURE format");
    }

    // Parse tokens
    // DEF_FEATURE NAME "Above-base Substitutions" TAG "abvs"
    if (tokens[1] != "NAME") {
      return EGLYF_ERROR_WHAT("Expected NAME in DEF_FEATURE");
    }

    auto name = unquote(tokens[2]);

    if (tokens[3] != "TAG") {
      return EGLYF_ERROR_WHAT("Expected TAG in DEF_FEATURE");
    }

    auto tagString = unquote(tokens[4]);
    if (tagString.size() != 4) {
      return EGLYF_ERROR_WHAT("Invalid string length for tag");
    }
    Tag tag;
    ranges::copy(tagString, tag.begin());

    // Define the feature
    auto feature = defineFeature(langsys, string(name), tag);

    // Parse subsequent lines until END_FEATURE
    for (size_t i = index + 1; i < lines.size();) {
      auto l = trim(lines[i]);

      if (l == "END_FEATURE") {
        index = i + 1;
        return Status::Ok();
      } else if (!l.empty()) {
        // Parse LOOKUP references
        auto lookupTokens = splitString(l);
        for (size_t j = 0; j < lookupTokens.size(); j++) {
          if (lookupTokens[j] == "LOOKUP" && j + 1 < lookupTokens.size()) {
            auto lookupName = unquote(lookupTokens[j + 1]);
            auto lookup = editor->getLookupByName(string(lookupName));
            feature->lookups.push_back(lookup);
            j++; // Skip lookup name
          }
        }
        i++;
      } else {
        i++;
      }
    }

    return EGLYF_ERROR_WHAT("Missing END_FEATURE");
  }

private:
  std::shared_ptr<Editor> editor;
};

} // namespace eglyf
