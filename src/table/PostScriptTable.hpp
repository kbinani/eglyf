#pragma once

namespace eglyf {

// 'post'
class PostScriptTable : public Table {
public:
  struct Version2Data {
  public:
    static bool InvalidNameCharacter(char ch) {
      return !(('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ('0' <= ch && ch <= '9') || ch == '.' || ch == '_');
    }

    static Optional<Version2Data> Read(InputStream &in) {
      using namespace std;
      Version2Data r;
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

  public:
    std::vector<uint16_t> glyphNameIndex;
    std::vector<std::string> nameStrings;
  };

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
      if (!holds_alternative<Version2Data>(data)) {
        return EGLYF_NULLOPT;
      }
      auto d = get<Version2Data>(data);
      if (auto st = d.encode(out); !st.ok()) {
        return EGLYF_NULLOPT_PUSH(st);
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
      if (auto data = Version2Data::Read(in); data) {
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

  Status addName(std::string const &name) {
    using namespace std;
    if (!holds_alternative<Version2Data>(data)) {
      return EGLYF_ERROR;
    }
    if (ranges::any_of(name, Version2Data::InvalidNameCharacter)) {
      return EGLYF_ERROR;
    }
    auto &d = get<Version2Data>(data);
    size_t index = d.glyphNameIndex.size() + 258;
    if (index >= (size_t)numeric_limits<uint16_t>::max()) {
      return EGLYF_ERROR;
    }
    d.glyphNameIndex.push_back(index);
    d.nameStrings.push_back(name);
    return Status::Ok();
  }

  Status clone(std::shared_ptr<PostScriptTable> &out) const {
    return EGLYF_STATUS_PUSH(defaultClone<PostScriptTable>(out));
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

  std::variant<Version2Data, std::string> data;
};

} // namespace eglyf
