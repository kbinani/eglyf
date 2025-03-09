#pragma once

namespace eglyf {

// 'post'
class PostScriptTable : public Table {
public:
  struct Version2Data {
    static bool InvalidNameCharacter(char ch) {
      return !(('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ('0' <= ch && ch <= '9') || ch == '.' || ch == '_');
    }

    static Optional<Version2Data> Read(InputStream &in) {
      using namespace std;
      Version2Data r;
      if (!in.u16(&r.numGlyphs)) {
        return EGLYF_NULLOPT;
      }
      vector<uint16_t> nameIndex;
      if (!in.u16a(nameIndex, r.numGlyphs)) {
        return EGLYF_NULLOPT;
      }
      vector<string> names;
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
        if (ranges::any_of(s, InvalidNameCharacter)) {
          return EGLYF_NULLOPT;
        }
        names.push_back(s);
      }
      for (auto index : nameIndex) {
        if (index <= 257) {
          r.names.push_back(index);
        } else {
          auto offset = index - 258;
          if (offset >= names.size()) {
            return EGLYF_NULLOPT;
          }
          r.names.push_back(names[offset]);
        }
      }
      return r;
    }

    Status encode(OutputStream &out) const {
      using namespace std;
      if (!out.u16(numGlyphs)) {
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
          if (ranges::any_of(n, InvalidNameCharacter)) {
            return EGLYF_ERROR;
          }
          uint8_t bytes = n.size();
          if (!strings.u8(bytes)) {
            return EGLYF_ERROR;
          }
          if (!strings.write((void *)n.data(), bytes)) {
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

    uint16_t numGlyphs;
    std::vector<std::variant<uint16_t, std::string>> names;
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
    d.numGlyphs++;
    d.names.push_back(name);
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
