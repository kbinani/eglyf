#pragma once

namespace ksesh::otf {

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
    enum Flags {
      ARG_1_AND_2_ARE_WORDS = 0x0001,
      ARGS_ARE_XY_VALUES = 0x0002,
      ROUND_XY_TO_GRID = 0x0004,
      WE_HAVE_A_SCALE = 0x0008,
      MORE_COMPONENTS = 0x0020,
      WE_HAVE_AN_X_AND_Y_SCALE = 0x0040,
      WE_HAVE_A_TWO_BY_TWO = 0x0080,
      WE_HAVE_INSTRUCTIONS = 0x0100,
      USE_MY_METRICS = 0x0200,
      OVERLAP_COMPOUND = 0x0400,
      SCALED_COMPONENT_OFFSET = 0x0800,
      UNSCALED_COMPONENT_OFFSET = 0x1000,
    };

    struct GlyphRecord {
      uint16_t flags;
      uint16_t glyphIndex;
      std::variant<Vec<uint8_t>, Vec<int8_t>, Vec<uint16_t>, Vec<int16_t>> offset;
      std::optional<std::variant<F2DOT14, Vec<F2DOT14>>> scale;
      std::optional<Vec<F2DOT14>> scale2;
    };

    static std::optional<CompositeGlyph> Read(Header h, InputStream &in) {
      using namespace std;
      CompositeGlyph ret;
      ret.header = h;
      uint16_t flags;
      do {
        if (!in.u16(&flags)) {
          return nullopt;
        }
        GlyphRecord rec;
        rec.flags = flags;
        if (!in.u16(&rec.glyphIndex)) {
          return nullopt;
        }
        if (flags & ARG_1_AND_2_ARE_WORDS) {
          if (flags & ARGS_ARE_XY_VALUES) {
            Vec<int16_t> offset;
            if (!in.i16(&offset.x)) {
              return nullopt;
            }
            if (!in.i16(&offset.y)) {
              return nullopt;
            }
            rec.offset = offset;
          } else {
            Vec<uint16_t> offset;
            if (!in.u16(&offset.x)) {
              return nullopt;
            }
            if (!in.u16(&offset.y)) {
              return nullopt;
            }
            rec.offset = offset;
          }
        } else {
          if (flags & ARGS_ARE_XY_VALUES) {
            Vec<int8_t> offset;
            if (!in.i8(&offset.x)) {
              return nullopt;
            }
            if (!in.i8(&offset.y)) {
              return nullopt;
            }
            rec.offset = offset;
          } else {
            Vec<uint8_t> offset;
            if (!in.u8(&offset.x)) {
              return nullopt;
            }
            if (!in.u8(&offset.y)) {
              return nullopt;
            }
            rec.offset = offset;
          }
        }
        if (flags & WE_HAVE_A_SCALE) {
          F2DOT14 scale;
          if (!in.u16(&scale.data)) {
            return nullopt;
          }
          rec.scale = scale;
        } else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
          F2DOT14 xscale;
          F2DOT14 yscale;
          if (!in.u16(&xscale.data)) {
            return nullopt;
          }
          if (!in.u16(&yscale.data)) {
            return nullopt;
          }
          rec.scale = Vec<F2DOT14>(xscale, yscale);
        } else if (flags & WE_HAVE_A_TWO_BY_TWO) {
          F2DOT14 xscale;
          if (!in.u16(&xscale.data)) {
            return nullopt;
          }
          F2DOT14 scale01;
          if (!in.u16(&scale01.data)) {
            return nullopt;
          }
          F2DOT14 scale10;
          if (!in.u16(&scale10.data)) {
            return nullopt;
          }
          F2DOT14 yscale;
          if (!in.u16(&yscale.data)) {
            return nullopt;
          }
          rec.scale = Vec<F2DOT14>(xscale, yscale);
          rec.scale2 = Vec<F2DOT14>(scale01, scale10);
        }
        ret.records.push_back(rec);
      } while (flags & MORE_COMPONENTS);
      if (flags & WE_HAVE_INSTRUCTIONS) {
        uint16_t numInstr;
        if (!in.u16(&numInstr)) {
          return nullopt;
        }
        ret.instructions.resize(numInstr);
        if (!in.read(ret.instructions.data(), numInstr)) {
          return nullopt;
        }
      }
      return ret;
    }

    bool encode(OutputStream &out) const {
      using namespace std;
      if (!header.encode(out)) {
        return false;
      }
      for (size_t i = 0; i < records.size(); i++) {
        auto const &rec = records[i];
        uint16_t flg = rec.flags & (~(ARG_1_AND_2_ARE_WORDS | ARGS_ARE_XY_VALUES | WE_HAVE_A_SCALE | WE_HAVE_A_TWO_BY_TWO | WE_HAVE_AN_X_AND_Y_SCALE | MORE_COMPONENTS | WE_HAVE_INSTRUCTIONS));
        if (holds_alternative<Vec<int16_t>>(rec.offset)) {
          flg |= ARG_1_AND_2_ARE_WORDS;
          flg |= ARGS_ARE_XY_VALUES;
        } else if (holds_alternative<Vec<uint16_t>>(rec.offset)) {
          flg |= ARG_1_AND_2_ARE_WORDS;
        } else if (holds_alternative<Vec<int8_t>>(rec.offset)) {
          flg |= ARGS_ARE_XY_VALUES;
        } else if (holds_alternative<Vec<uint8_t>>(rec.offset)) {
          // nop
        }
        if (rec.scale) {
          if (holds_alternative<F2DOT14>(*rec.scale)) {
            flg |= WE_HAVE_A_SCALE;
          } else if (rec.scale2) {
            flg |= WE_HAVE_A_TWO_BY_TWO;
          } else {
            flg |= WE_HAVE_AN_X_AND_Y_SCALE;
          }
        }
        if (i + 1 < records.size()) {
          flg |= MORE_COMPONENTS;
        } else if (instructions.size() > 0) {
          flg |= WE_HAVE_INSTRUCTIONS;
        }
        if (!out.u16(rec.flags)) {
          return false;
        }
        if (!out.u16(rec.glyphIndex)) {
          return false;
        }
        if (holds_alternative<Vec<int16_t>>(rec.offset)) {
          auto const &o = get<Vec<int16_t>>(rec.offset);
          if (!out.i16(o.x)) {
            return false;
          }
          if (!out.i16(o.y)) {
            return false;
          }
        } else if (holds_alternative<Vec<uint16_t>>(rec.offset)) {
          auto const &o = get<Vec<uint16_t>>(rec.offset);
          if (!out.u16(o.x)) {
            return false;
          }
          if (!out.u16(o.y)) {
            return false;
          }
        } else if (holds_alternative<Vec<int8_t>>(rec.offset)) {
          auto const &o = get<Vec<int8_t>>(rec.offset);
          if (!out.i8(o.x)) {
            return false;
          }
          if (!out.i8(o.y)) {
            return false;
          }
        } else if (holds_alternative<Vec<uint8_t>>(rec.offset)) {
          auto const &o = get<Vec<uint8_t>>(rec.offset);
          if (!out.u8(o.x)) {
            return false;
          }
          if (!out.u8(o.y)) {
            return false;
          }
        }
        if (rec.scale) {
          if (holds_alternative<F2DOT14>(*rec.scale)) {
            auto s1 = get<F2DOT14>(*rec.scale);
            if (!out.u16(s1.data)) {
              return false;
            }
          } else if (holds_alternative<Vec<F2DOT14>>(*rec.scale)) {
            auto s1 = get<Vec<F2DOT14>>(*rec.scale);
            if (rec.scale2) {
              if (!out.u16(s1.x.data)) {
                return false;
              }
              if (!out.u16(rec.scale2->x.data)) {
                return false;
              }
              if (!out.u16(rec.scale2->y.data)) {
                return false;
              }
              if (!out.u16(s1.y.data)) {
                return false;
              }
            } else {
              if (!out.u16(s1.x.data)) {
                return false;
              }
              if (!out.u16(s1.y.data)) {
                return false;
              }
            }
          }
        }
      }
      if (instructions.size() > 0) {
        if (instructions.size() > (size_t)numeric_limits<uint16_t>::max()) {
          return false;
        }
        uint16_t numInstr = static_cast<uint16_t>(instructions.size());
        if (!out.u16(numInstr)) {
          return false;
        }
        if (!out.write((void *)instructions.data(), instructions.size())) {
          return false;
        }
      }
      return true;
    }

    Header header;
    std::vector<GlyphRecord> records;
    std::vector<uint8_t> instructions;
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
      if (header->numberOfContours < 0) {
        if (auto cg = CompositeGlyph::Read(*header, slice); cg) {
          ret->glyphs.push_back(*cg);
        } else {
          return nullptr;
        }
      } else {
        if (auto rg = ReadonlyGlyph::Read(*header, slice); rg) {
          ret->glyphs.push_back(*rg);
        } else {
          return nullptr;
        }
      }
    }
    return ret;
  }

  std::optional<EncodeResult> encode() override {
    return std::nullopt;
  }

  std::optional<EncodeResult> encode(IndexToLocationTable &loca) {
    using namespace std;

    loca.offsets.clear();

    ByteOutputStream out;
    for (auto const &g : glyphs) {
      loca.offsets.push_back(out.size());
      if (holds_alternative<EmptyGlyph>(g)) {
        // nop
      } else if (holds_alternative<ReadonlyGlyph>(g)) {
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
      auto size = out.size();
      if (size % 4 != 0) {
        size_t cnt = 4 - size % 4;
        if (!out.write((void *)"\0\0\0\0", cnt)) {
          return nullopt;
        }
      }
    }
    loca.offsets.push_back(out.size());
    return EncodeResult(out.data());
  }

public:
  std::vector<std::variant<EmptyGlyph, ReadonlyGlyph, CompositeGlyph>> glyphs;
};

} // namespace ksesh::otf
