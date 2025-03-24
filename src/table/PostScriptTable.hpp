#pragma once

namespace eglyf {

// 'post'
class PostScriptTable : public Table {
public:
  static bool InvalidNameCharacter(char ch) {
    return !(('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ('0' <= ch && ch <= '9') || ch == '.' || ch == '_');
  }

  struct ReadonlyVersion2Data {
  public:
    static Optional<ReadonlyVersion2Data> Read(InputStream &in) {
      using namespace std;
      ReadonlyVersion2Data r;
      uint16_t numGlyphs;
      if (!in.u16(&numGlyphs)) {
        return EGLYF_NULLOPT;
      }
      if (!in.u16a(r.glyphNameIndex, numGlyphs)) {
        return EGLYF_NULLOPT;
      }
      while (true) {
        uint8_t bytes;
        if (!in.u8(&bytes)) {
          break;
        }
        string s;
        s.resize(bytes);
        if (in.read(s.data(), bytes) != bytes) {
          return EGLYF_NULLOPT;
        }
        r.nameStrings.push_back(s);
      }
      return r;
    }

    Status encode(OutputStream &out) const {
      using namespace std;
      if (!out.sizeU16(glyphNameIndex.size())) {
        return EGLYF_ERROR;
      }
      if (!out.u16a(glyphNameIndex)) {
        return EGLYF_ERROR;
      }
      for (auto const &name : nameStrings) {
        if (!out.sizeU8(name.size())) {
          return EGLYF_ERROR;
        }
        if (!out.write(name.data(), name.size())) {
          return EGLYF_ERROR;
        }
      }
      return Status::Ok();
    }

    Optional<std::string> getName(uint16_t glyphId) const {
      if (glyphId >= glyphNameIndex.size()) {
        return EGLYF_NULLOPT;
      }
      uint16_t index = glyphNameIndex[glyphId];
      if (index < 258) {
        auto const &table = PostScriptTable::OSXPostScriptNames();
        if (index >= table.size()) {
          return EGLYF_NULLOPT;
        }
        return table[index];
      } else {
        auto i = index - 258;
        if (i >= nameStrings.size()) {
          return EGLYF_NULLOPT;
        }
        return nameStrings[i];
      }
    }

    Optional<uint16_t> getGlyphId(std::string const &name) const {
      using namespace std;
      auto const &table = PostScriptTable::OSXPostScriptNameMap();
      if (auto found = table.find(name); found != table.end()) {
        auto index = found->second;
        auto f = ranges::find_if(glyphNameIndex, [=](uint16_t idx) {
          return index == idx;
        });
        if (f == glyphNameIndex.end()) {
          return EGLYF_NULLOPT;
        } else {
          return distance(glyphNameIndex.begin(), f);
        }
      } else {
        auto f = ranges::find_if(glyphNameIndex, [=, this](uint16_t idx) {
          if (idx < 258) {
            return false;
          }
          if (idx - 258 >= nameStrings.size()) {
            return false;
          }
          return nameStrings[idx - 258] == name;
        });
        if (f == glyphNameIndex.end()) {
          return EGLYF_NULLOPT;
        } else {
          return distance(glyphNameIndex.begin(), f);
        }
      }
    }

  public:
    std::vector<uint16_t> glyphNameIndex;
    std::vector<std::string> nameStrings;
  };

  struct Version2Data {
    static Optional<Version2Data> Migrate(ReadonlyVersion2Data const &in) {
      Version2Data r;
      for (uint16_t index : in.glyphNameIndex) {
        if (index < 258) {
          r.names.push_back(index);
        } else if (index - 258 < in.nameStrings.size()) {
          r.names.push_back(in.nameStrings[index - 258]);
        } else {
          return EGLYF_NULLOPT;
        }
      }
      return r;
    }

    Status encode(OutputStream &out) const {
      using namespace std;
      if (!out.sizeU16(names.size())) {
        return EGLYF_ERROR;
      }
      ByteOutputStream strings;
      uint16_t count = 0;
      for (auto const &name : names) {
        if (holds_alternative<string>(name)) {
          auto const &n = get<string>(name);
          if (!out.u16(count + 258)) {
            return EGLYF_ERROR;
          }
          uint8_t bytes = n.size();
          if (!strings.u8(bytes)) {
            return EGLYF_ERROR;
          }
          if (!strings.write(n.data(), bytes)) {
            return EGLYF_ERROR;
          }
          count++;
        } else if (holds_alternative<uint16_t>(name)) {
          auto n = get<uint16_t>(name);
          if (n > 257) {
            return EGLYF_ERROR;
          }
          if (!out.u16(n)) {
            return EGLYF_ERROR;
          }
        }
      }
      string data = strings.data();
      if (out.write(data.data(), data.size())) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR;
      }
    }

    Optional<std::string> getName(uint16_t glyphId) const {
      using namespace std;
      if (glyphId >= names.size()) {
        return EGLYF_NULLOPT;
      }
      auto entry = names[glyphId];
      if (holds_alternative<string>(entry)) {
        return get<string>(entry);
      } else {
        uint16_t index = get<uint16_t>(entry);
        auto const &table = PostScriptTable::OSXPostScriptNames();
        if (index < table.size()) {
          return table[index];
        } else {
          return EGLYF_NULLOPT;
        }
      }
    }

    Status setName(uint16_t glyphId, std::string const &name) {
      using namespace std;
      if (glyphId >= names.size()) {
        return EGLYF_ERROR;
      }
      auto const &table = PostScriptTable::OSXPostScriptNameMap();
      if (auto found = table.find(name); found == table.end()) {
        names[glyphId] = name;
      } else {
        names[glyphId] = found->second;
      }
      return Status::Ok();
    }

    Optional<uint16_t> getGlyphId(std::string const &name) const {
      using namespace std;
      auto const &table = PostScriptTable::OSXPostScriptNameMap();
      if (auto found = table.find(name); found != table.end()) {
        auto index = found->second;
        auto f = ranges::find_if(names, [=](auto const &entry) {
          if (holds_alternative<uint16_t>(entry)) {
            return get<uint16_t>(entry) == index;
          } else {
            return false;
          }
        });
        if (f == names.end()) {
          return EGLYF_NULLOPT;
        } else {
          return distance(names.begin(), f);
        }
      } else {
        auto f = ranges::find_if(names, [=](auto const &entry) {
          if (holds_alternative<string>(entry)) {
            return get<string>(entry) == name;
          } else {
            return false;
          }
        });
        if (f == names.end()) {
          return EGLYF_NULLOPT;
        } else {
          return distance(names.begin(), f);
        }
      }
    }

    std::vector<std::variant<uint16_t, std::string>> names;
  };

private:
  static std::vector<std::string> *CreateOSXPostScriptNames() {
    using namespace std;
    auto ret = new vector<string>({
        // clang-format off
        ".notdef", ".null", "nonmarkingreturn", "space", "exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand",
        "quotesingle", "parenleft", "parenright", "asterisk", "plus", "comma", "hyphen", "period", "slash", "zero",
        "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "colon",
        "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C", "D",
        "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
        "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X",
        "Y", "Z", "bracketleft", "backslash", "bracketright", "asciicircum", "underscore", "grave", "a", "b",
        "c", "d", "e", "f", "g", "h", "i", "j", "k", "l",
        "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
        "w", "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Adieresis", "Aring",
        "Ccedilla", "Eacute", "Ntilde", "Odieresis", "Udieresis", "aacute", "agrave", "acircumflex", "adieresis", "atilde",
        "aring", "ccedilla", "eacute", "egrave", "ecircumflex", "edieresis", "iacute", "igrave", "icircumflex", "idieresis",
        "ntilde", "oacute", "ograve", "ocircumflex", "odieresis", "otilde", "uacute", "ugrave", "ucircumflex", "udieresis",
        "dagger", "degree", "cent", "sterling", "section", "bullet", "paragraph", "germandbls", "registered", "copyright",
        "trademark", "acute", "dieresis", "notequal", "AE", "Oslash", "infinity", "plusminus", "lessequal", "greaterequal",
        "yen", "mu", "partialdiff", "summation", "product", "pi", "integral", "ordfeminine", "ordmasculine", "Omega",
        "ae", "oslash", "questiondown", "exclamdown", "logicalnot", "radical", "florin", "approxequal", "Delta", "guillemotleft",
        "guillemotright", "ellipsis", "nonbreakingspace", "Agrave", "Atilde", "Otilde", "OE", "oe", "endash", "emdash",
        "quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide", "lozenge", "ydieresis", "Ydieresis", "fraction", "currency",
        "guilsinglleft", "guilsinglright", "fi", "fl", "daggerdbl", "periodcentered", "quotesinglbase", "quotedblbase", "perthousand", "Acircumflex",
        "Ecircumflex", "Aacute", "Edieresis", "Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex",
        "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave", "dotlessi", "circumflex", "tilde", "macron", "breve",
        "dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek", "caron", "Lslash", "lslash", "Scaron", "scaron",
        "Zcaron", "zcaron", "brokenbar", "Eth", "eth", "Yacute", "yacute", "Thorn", "thorn", "minus",
        "multiply", "onesuperior", "twosuperior", "threesuperior", "onehalf", "onequarter", "threequarters", "franc", "Gbreve", "gbreve",
        "Idotaccent", "Scedilla", "scedilla", "Cacute", "cacute", "Ccaron", "ccaron", "dcroat",
        // clang-format on
    });
    return ret;
  }

  static std::vector<std::string> const &OSXPostScriptNames() {
    using namespace std;
    static unique_ptr<vector<string> const> const sTable(CreateOSXPostScriptNames());
    return *sTable;
  }

  static std::unordered_map<std::string, uint16_t> *CreateOSXPostScriptNameMap() {
    using namespace std;
    auto ret = new std::unordered_map<std::string, uint16_t>();
    auto const &table = OSXPostScriptNames();
    for (size_t i = 0; i < table.size(); i++) {
      (*ret)[table[i]] = i;
    }
    return ret;
  }

  static std::unordered_map<std::string, uint16_t> const &OSXPostScriptNameMap() {
    using namespace std;
    static unique_ptr<unordered_map<string, uint16_t> const> const sMap(CreateOSXPostScriptNameMap());
    return *sMap;
  }

public:
  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version.major)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(version.minor)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(italicAngle.value)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(underlinePosition)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(underlineThickness)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(isFixedPitch)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(minMemType42)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(maxMemType42)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(minMemType1)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(maxMemType1)) {
      return EGLYF_NULLOPT;
    }
    if (version.major == 0x0002 && version.minor == 0x0000) {
      if (holds_alternative<Version2Data>(data)) {
        auto const &d = get<Version2Data>(data);
        if (auto st = d.encode(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      } else if (holds_alternative<ReadonlyVersion2Data>(data)) {
        auto const &d = get<ReadonlyVersion2Data>(data);
        if (auto st = d.encode(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      } else {
        return EGLYF_NULLOPT;
      }
      return EncodeResult(out.data());
    } else {
      if (!holds_alternative<string>(data)) {
        return EGLYF_NULLOPT;
      }
      auto d = get<string>(data);
      if (!out.write(d.data(), d.size())) {
        return EGLYF_NULLOPT;
      }
      return EncodeResult(out.data());
    }
  }

  static Status Read(InputStream &in, std::shared_ptr<PostScriptTable> &out) {
    using namespace std;
    auto r = make_unique<PostScriptTable>();
    if (!in.u16(&r->version.major)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->version.minor)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->italicAngle.value)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->underlinePosition)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->underlineThickness)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->isFixedPitch)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->minMemType42)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->maxMemType42)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->minMemType1)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->maxMemType1)) {
      return EGLYF_ERROR;
    }
    if (r->version.major == 0x0002 && r->version.minor == 0x0000) {
      if (auto data = ReadonlyVersion2Data::Read(in); data) {
        r->data = *data;
      } else {
        return EGLYF_STATUS_PUSH(data.status());
      }
    } else {
      r->data = in.readUntilEos();
    }
    out.reset(r.release());
    return Status::Ok();
  }

  // Returns glyphId
  Optional<uint16_t> addName(std::string const &name) {
    using namespace std;
    if (holds_alternative<string>(data)) {
      return EGLYF_NULLOPT;
    }
    if (ranges::any_of(name, InvalidNameCharacter)) {
      return EGLYF_NULLOPT;
    }
    Version2Data d;
    if (holds_alternative<ReadonlyVersion2Data>(data)) {
      auto migrated = Version2Data::Migrate(get<ReadonlyVersion2Data>(data));
      if (!migrated) {
        return EGLYF_NULLOPT_PUSH(migrated.status());
      }
      d = *migrated;
    } else {
      d = get<Version2Data>(data);
    }
    auto const &ps = OSXPostScriptNameMap();
    size_t gid = d.names.size();
    if (gid > numeric_limits<uint16_t>::max()) {
      return EGLYF_NULLOPT;
    }
    if (auto found = ps.find(name); found == ps.end()) {
      d.names.push_back(name);
    } else {
      d.names.push_back(found->second);
    }
    data = d;
    return static_cast<uint16_t>(gid);
  }

  Status clone(std::shared_ptr<PostScriptTable> &out) const {
    return EGLYF_STATUS_PUSH(defaultClone<PostScriptTable>(out));
  }

  Optional<std::string> getName(uint16_t glyphId) const {
    using namespace std;
    if (holds_alternative<Version2Data>(data)) {
      auto const &d = get<Version2Data>(data);
      if (auto name = d.getName(glyphId); name) {
        return *name;
      } else {
        return EGLYF_NULLOPT_PUSH(name.status());
      }
    } else if (holds_alternative<ReadonlyVersion2Data>(data)) {
      auto const &d = get<ReadonlyVersion2Data>(data);
      if (auto name = d.getName(glyphId); name) {
        return *name;
      } else {
        return EGLYF_NULLOPT_PUSH(name.status());
      }
    } else {
      return EGLYF_NULLOPT;
    }
  }

  Status setName(uint16_t glyphId, std::string const &name) {
    using namespace std;
    Version2Data d;
    if (holds_alternative<ReadonlyVersion2Data>(data)) {
      auto migrated = Version2Data::Migrate(get<ReadonlyVersion2Data>(data));
      if (!migrated) {
        return EGLYF_STATUS_PUSH(migrated.status());
      }
      d = *migrated;
    } else if (holds_alternative<string>(data)) {
      return EGLYF_ERROR;
    } else {
      d = get<Version2Data>(data);
    }
    if (auto st = d.setName(glyphId, name); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    data = d;
    return Status::Ok();
  }

  Optional<uint16_t> getGlyphId(std::string const &name) const {
    using namespace std;
    if (holds_alternative<Version2Data>(data)) {
      auto const &d = get<Version2Data>(data);
      if (auto gid = d.getGlyphId(name); gid) {
        return *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    } else if (holds_alternative<ReadonlyVersion2Data>(data)) {
      auto const &d = get<ReadonlyVersion2Data>(data);
      if (auto gid = d.getGlyphId(name); gid) {
        return *gid;
      } else {
        return EGLYF_NULLOPT_PUSH(gid.status());
      }
    } else {
      return EGLYF_NULLOPT;
    }
  }

public:
  Version16Dot16 version;
  Fixed italicAngle;
  FWORD underlinePosition;
  FWORD underlineThickness;
  uint32_t isFixedPitch;
  uint32_t minMemType42;
  uint32_t maxMemType42;
  uint32_t minMemType1;
  uint32_t maxMemType1;

  std::variant<Version2Data, ReadonlyVersion2Data, std::string> data;
};

} // namespace eglyf
