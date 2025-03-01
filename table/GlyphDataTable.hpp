#pragma once

namespace ksesh {

// 'glyf'
class GlyphDataTable : public Table {
public:
  struct Header {
    int16_t numberOfContours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;

    static std::optional<Header> Read(InputStream &in) {
      using namespace std;
      Header h;
      if (!in.i16(&h.numberOfContours)) {
        return nullopt;
      }
      if (!in.i16(&h.xMin)) {
        return nullopt;
      }
      if (!in.i16(&h.yMin)) {
        return nullopt;
      }
      if (!in.i16(&h.xMax)) {
        return nullopt;
      }
      if (!in.i16(&h.yMax)) {
        return nullopt;
      }
      return h;
    }

    bool encode(OutputStream &out) const {
      if (!out.i16(numberOfContours)) {
        return false;
      }
      if (!out.i16(xMin)) {
        return false;
      }
      if (!out.i16(yMin)) {
        return false;
      }
      if (!out.i16(xMax)) {
        return false;
      }
      return out.i16(yMax);
    }
  };

  struct EmptyGlyph {
  };

  struct ReadonlyGlyph {
    static std::optional<ReadonlyGlyph> Read(Header header, InputStream &in) {
      using namespace std;
      ReadonlyGlyph r;
      r.header = header;
      r.data = in.readUntilEos();
      return r;
    }

    bool encode(OutputStream &out) const {
      if (!header.encode(out)) {
        return false;
      }
      return out.write((void *)data.c_str(), data.size());
    }

    Header header;
    std::string data;
  };

  struct CompositeGlyph {
    static std::optional<CompositeGlyph> Read(InputStream &in) {
      using namespace std;
      return nullopt;
    }

    bool encode(OutputStream &out) const {
      return false;
    }

    Header header;
  };

  static std::shared_ptr<GlyphDataTable> Read(InputStream &in, IndexToLocationTable const &loca) {
    using namespace std;
    auto ret = make_shared<GlyphDataTable>();
    for (size_t i = 1; i < loca.offsets.size(); i++) {
      Offset32 offset = loca.offsets[i - 1];
      Offset32 length = loca.offsets[i] - offset;
      if (length < 1) {
        ret->glyphs.push_back(EmptyGlyph());
        continue;
      }
      if (!in.seek(offset)) {
        return nullptr;
      }
      string buffer;
      buffer.resize(length);
      if (length != in.read(buffer.data(), length)) {
        return nullptr;
      }
      ByteInputStream slice(buffer);
      auto header = Header::Read(slice);
      if (!header) {
        return nullptr;
      }
      if (auto g = ReadonlyGlyph::Read(*header, slice); g) {
        ret->glyphs.push_back(*g);
      } else {
        return nullptr;
      }
    }
    return ret;
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    for (auto const &g : glyphs) {
      if (holds_alternative<ReadonlyGlyph>(g)) {
        auto rg = get<ReadonlyGlyph>(g);
        if (!rg.encode(out)) {
          return nullopt;
        }
      } else if (holds_alternative<CompositeGlyph>(g)) {
        auto cg = get<CompositeGlyph>(g);
        if (!cg.encode(out)) {
          return nullopt;
        }
      }
    }
    return EncodeResult(out.data());
  }

public:
  std::vector<std::variant<EmptyGlyph, ReadonlyGlyph, CompositeGlyph>> glyphs;
};

} // namespace ksesh
