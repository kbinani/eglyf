#pragma once

namespace ksesh::otf {

// 'post'
class PostScriptTable : public Table {
public:
  struct Version2Data {
    static std::optional<Version2Data> Read(InputStream &in) {
      using namespace std;
      Version2Data r;
      if (!in.u16(&r.numGlyphs)) {
        return nullopt;
      }
      r.glyphNameIndex.reserve(r.numGlyphs);
      for (uint16_t i = 0; i < r.numGlyphs; i++) {
        uint16_t v;
        if (!in.u16(&v)) {
          return nullopt;
        }
        r.glyphNameIndex.push_back(v);
      }
      r.stringData = in.readUntilEos();
      return r;
    }

    bool encode(OutputStream &out) const {
      if (!out.u16(numGlyphs)) {
        return false;
      }
      for (uint16_t i = 0; i < numGlyphs; i++) {
        if (!out.u16(glyphNameIndex[i])) {
          return false;
        }
      }
      return out.write((void *)stringData.c_str(), stringData.size());
    }

    uint16_t numGlyphs;
    std::vector<uint16_t> glyphNameIndex;
    std::string stringData;
  };

public:
  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version.major)) {
      return nullopt;
    }
    if (!out.u16(version.minor)) {
      return nullopt;
    }
    if (!out.u32(italicAngle.value)) {
      return nullopt;
    }
    if (!out.i16(underlinePosition)) {
      return nullopt;
    }
    if (!out.i16(underlineThickness)) {
      return nullopt;
    }
    if (!out.u32(isFixedPitch)) {
      return nullopt;
    }
    if (!out.u32(minMemType42)) {
      return nullopt;
    }
    if (!out.u32(maxMemType42)) {
      return nullopt;
    }
    if (!out.u32(minMemType1)) {
      return nullopt;
    }
    if (!out.u32(maxMemType1)) {
      return nullopt;
    }
    if (version.major == 0x002 && version.minor == 0x0000) {
      if (!holds_alternative<Version2Data>(data)) {
        return nullopt;
      }
      auto d = get<Version2Data>(data);
      if (!d.encode(out)) {
        return nullopt;
      }
      return EncodeResult(out.data());
    } else {
      if (!holds_alternative<string>(data)) {
        return nullopt;
      }
      auto d = get<string>(data);
      if (!out.write(d.data(), d.size())) {
        return nullopt;
      }
      return EncodeResult(out.data());
    }
  }

  static std::shared_ptr<PostScriptTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<PostScriptTable>();
    if (!in.u16(&r->version.major)) {
      return nullptr;
    }
    if (!in.u16(&r->version.minor)) {
      return nullptr;
    }
    if (!in.u32(&r->italicAngle.value)) {
      return nullptr;
    }
    if (!in.i16(&r->underlinePosition)) {
      return nullptr;
    }
    if (!in.i16(&r->underlineThickness)) {
      return nullptr;
    }
    if (!in.u32(&r->isFixedPitch)) {
      return nullptr;
    }
    if (!in.u32(&r->minMemType42)) {
      return nullptr;
    }
    if (!in.u32(&r->maxMemType42)) {
      return nullptr;
    }
    if (!in.u32(&r->minMemType1)) {
      return nullptr;
    }
    if (!in.u32(&r->maxMemType1)) {
      return nullptr;
    }
    if (r->version.major == 0x0002 && r->version.minor == 0x0000) {
      if (auto data = Version2Data::Read(in); data) {
        r->data = *data;
      } else {
        return nullptr;
      }
    } else {
      r->data = in.readUntilEos();
    }
    return r;
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

} // namespace ksesh::otf
