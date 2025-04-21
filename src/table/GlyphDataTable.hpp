#pragma once

namespace eglyf {

// 'glyf'
class GlyphDataTable : public Table {
public:
  struct Header {
    int16_t numberOfContours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;

    static Optional<Header> Read(InputStream &in) {
      using namespace std;
      Header h;
      if (!in.i16(&h.numberOfContours)) {
        return EGLYF_NULLOPT_WHAT("Failed to read numberOfContours");
      }
      if (!in.i16(&h.xMin)) {
        return EGLYF_NULLOPT_WHAT("Failed to read xMin");
      }
      if (!in.i16(&h.yMin)) {
        return EGLYF_NULLOPT_WHAT("Failed to read yMin");
      }
      if (!in.i16(&h.xMax)) {
        return EGLYF_NULLOPT_WHAT("Failed to read xMax");
      }
      if (!in.i16(&h.yMax)) {
        return EGLYF_NULLOPT_WHAT("Failed to read yMax");
      }
      return h;
    }

    Status encode(OutputStream &out) const {
      if (!out.i16(numberOfContours)) {
        return EGLYF_ERROR_WHAT("Failed to write numberOfContours");
      }
      if (!out.i16(xMin)) {
        return EGLYF_ERROR_WHAT("Failed to write xMin");
      }
      if (!out.i16(yMin)) {
        return EGLYF_ERROR_WHAT("Failed to write yMin");
      }
      if (!out.i16(xMax)) {
        return EGLYF_ERROR_WHAT("Failed to write xMax");
      }
      if (out.i16(yMax)) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR_WHAT("Failed to write yMax");
      }
    }
  };

  struct EmptyGlyph {
  };

  struct Point {
    int16_t x;
    int16_t y;
    bool control;
  };

  struct Contour {
    std::vector<Point> points;
  };

  struct SimpleGlyph {
    enum : uint8_t {
      ON_CURVE = 0x01,
      X_SHORT = 0x02,
      Y_SHORT = 0x04,
      REPEAT = 0x08,
      X_SAME = 0x10,
      Y_SAME = 0x20,
    };

    static Optional<SimpleGlyph> Read(Header header, InputStream &in) {
      using namespace std;
      vector<uint16_t> endPtsOfContours;
      if (!in.u16a(endPtsOfContours, header.numberOfContours)) {
        return EGLYF_NULLOPT;
      }
      uint16_t instructionLength;
      if (!in.u16(&instructionLength)) {
        return EGLYF_NULLOPT;
      }
      SimpleGlyph g;
      g.header = header;
      g.numPoints = 0;
      if (instructionLength > 0) {
        g.instructions.resize(instructionLength);
        if (!in.read(g.instructions.data(), instructionLength)) {
          return EGLYF_NULLOPT;
        }
      }
      if (header.numberOfContours == 0) {
        return g;
      }
      g.contours.resize(header.numberOfContours);
      vector<uint8_t> flags;
      while (flags.size() < endPtsOfContours[header.numberOfContours - 1] + 1) {
        uint8_t flag;
        if (!in.u8(&flag)) {
          return EGLYF_NULLOPT;
        }
        if ((flag & REPEAT) == REPEAT) {
          uint8_t count;
          if (!in.u8(&count)) {
            return EGLYF_NULLOPT;
          }
          for (int j = 0; j < count + 1; j++) {
            flags.push_back(flag);
          }
        } else {
          flags.push_back(flag);
        }
      }
      int16_t x = 0;
      for (uint16_t i = 0; i < header.numberOfContours; i++) {
        uint16_t from;
        if (i == 0) {
          from = 0;
        } else {
          from = endPtsOfContours[i - 1] + 1;
        }
        g.contours[i].points.resize(endPtsOfContours[i] - from + 1);
        for (uint16_t j = from; j <= endPtsOfContours[i]; j++) {
          uint8_t flag = flags[j];
          int16_t dx = 0;
          if ((flag & X_SHORT) == X_SHORT) {
            uint8_t v;
            if (!in.u8(&v)) {
              return EGLYF_NULLOPT;
            }
            if ((flag & X_SAME) == X_SAME) {
              dx = (int16_t)v;
            } else {
              dx = -(int16_t)v;
            }
          } else {
            if ((flag & X_SAME) == X_SAME) {
              dx = 0;
            } else {
              if (!in.i16(&dx)) {
                return EGLYF_NULLOPT;
              }
            }
          }
          x += dx;
          g.contours[i].points[j - from].x = x;
        }
      }
      int16_t y = 0;
      for (uint16_t i = 0; i < header.numberOfContours; i++) {
        uint16_t from;
        if (i == 0) {
          from = 0;
        } else {
          from = endPtsOfContours[i - 1] + 1;
        }
        for (uint16_t j = from; j <= endPtsOfContours[i]; j++) {
          uint8_t flag = flags[j];
          int16_t dy = 0;
          if ((flag & Y_SHORT) == Y_SHORT) {
            uint8_t v;
            if (!in.u8(&v)) {
              return EGLYF_NULLOPT;
            }
            if ((flag & Y_SAME) == Y_SAME) {
              dy = (int16_t)v;
            } else {
              dy = -(int16_t)v;
            }
          } else {
            if ((flag & Y_SAME) == Y_SAME) {
              dy = 0;
            } else {
              if (!in.i16(&dy)) {
                return EGLYF_NULLOPT;
              }
            }
          }
          y += dy;
          g.contours[i].points[j - from].y = y;
          g.contours[i].points[j - from].control = (flag & ON_CURVE) != ON_CURVE;
        }
      }
      return g;
    }

    Status encode(OutputStream &out) const {
      using namespace std;
      vector<uint16_t> endPtsOfContours;
      uint16_t num = 0;
      vector<uint8_t> flags;
      int16_t x = 0;
      int16_t y = 0;
      ByteOutputStream xs;
      ByteOutputStream ys;
      for (auto const &c : contours) {
        if (c.points.empty()) {
          return EGLYF_ERROR;
        }
        if ((size_t)num + c.points.size() > numeric_limits<uint16_t>::max()) [[unlikely]] {
          return EGLYF_ERROR;
        }
        num += c.points.size();
        endPtsOfContours.push_back(num - 1);
        for (auto const &p : c.points) {
          uint8_t flag = p.control ? 0 : ON_CURVE;

          int16_t dx = p.x - x;
          if (dx == 0) {
            flag |= X_SAME;
          } else if (-255 <= dx && dx <= 255) {
            uint8_t v;
            if (dx < 0) {
              flag |= X_SHORT;
              v = (uint8_t)(-dx);
            } else {
              flag |= X_SHORT | X_SAME;
              v = (uint8_t)dx;
            }
            if (!xs.u8(v)) {
              return EGLYF_ERROR;
            }
          } else {
            if (!xs.i16(dx)) {
              return EGLYF_ERROR;
            }
          }
          x = p.x;

          int16_t dy = p.y - y;
          if (dy == 0) {
            flag |= Y_SAME;
          } else if (-255 <= dy && dy <= 255) {
            uint8_t v;
            if (dy < 0) {
              flag |= Y_SHORT;
              v = (uint8_t)(-dy);
            } else {
              flag |= Y_SHORT | Y_SAME;
              v = (uint8_t)dy;
            }
            if (!ys.u8(v)) {
              return EGLYF_ERROR;
            }
          } else {
            if (!ys.i16(dy)) {
              return EGLYF_ERROR;
            }
          }
          y = p.y;

          flags.push_back(flag);
        }
      }
      vector<uint8_t> flagBytes;
      for (int i = 0; i < flags.size();) {
        auto flag = flags[i];
        int count = 0;
        for (int j = i + 1; j < flags.size(); j++) {
          if (flags[j] == flag) {
            count++;
          } else {
            break;
          }
        }
        if (0 < count) {
          auto c = min(count, 255);
          flagBytes.push_back(flag | REPEAT);
          flagBytes.push_back((uint8_t)c);
          i += c + 1;
        } else {
          flagBytes.push_back(flag);
          i++;
        }
      }
      if (auto st = header.encode(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (!out.u16a(endPtsOfContours)) {
        return EGLYF_ERROR;
      }
      if (!out.sizeU16(instructions.size())) {
        return EGLYF_ERROR;
      }
      if (!instructions.empty()) {
        if (!out.write(instructions.data(), instructions.size())) {
          return EGLYF_ERROR;
        }
      }
      assert(!flagBytes.empty());
      if (!out.write(flagBytes.data(), flagBytes.size())) {
        return EGLYF_ERROR;
      }
      auto xsd = xs.data();
      if (!out.write(xsd.data(), xsd.size())) {
        return EGLYF_ERROR;
      }
      auto ysd = ys.data();
      if (!out.write(ysd.data(), ysd.size())) {
        return EGLYF_ERROR;
      }
      return Status::Ok();
    }

    Header header;
    uint16_t numPoints = 0;
    std::vector<Contour> contours;
    std::string instructions;
  };

  struct ReadonlyGlyph {
    static Optional<ReadonlyGlyph> Read(Header header, InputStream &in) {
      using namespace std;
      ReadonlyGlyph r;
      r.header = header;
      r.data = in.readUntilEos();
      r.numPoints = 0;
      if (header.numberOfContours > 0) {
        ByteInputStream in(r.data);
        for (uint16_t i = 0; i < header.numberOfContours; i++) {
          uint16_t index;
          if (!in.u16(&index)) {
            return EGLYF_NULLOPT_WHAT("Failed to read contour index");
          }
          r.numPoints = (std::max)(r.numPoints, (uint16_t)(index + 1));
        }
      }
      return r;
    }

    Status encode(OutputStream &out) const {
      if (auto st = header.encode(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      if (out.write((void *)data.c_str(), data.size())) {
        return Status::Ok();
      } else {
        return EGLYF_ERROR_WHAT("Failed to write data");
      }
    }

    Header header;
    uint16_t numPoints;
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

      static GlyphRecord New(uint16_t glyphIndex, int16_t dx, int16_t dy, std::optional<std::variant<float, Vec<float>>> scale = std::nullopt) {
        GlyphRecord r;
        r.glyphIndex = glyphIndex;
        r.flags = ARGS_ARE_XY_VALUES | ARG_1_AND_2_ARE_WORDS;
        if (scale) {
          if (holds_alternative<float>(*scale)) {
            float fs = get<float>(*scale);
            r.flags |= WE_HAVE_A_SCALE;
            r.scale = F2DOT14::FromFloat(fs);
          } else if (holds_alternative<Vec<float>>(*scale)) {
            auto fvs = get<Vec<float>>(*scale);
            r.flags |= WE_HAVE_AN_X_AND_Y_SCALE;
            r.scale = Vec<F2DOT14>(F2DOT14::FromFloat(fvs.x), F2DOT14::FromFloat(fvs.y));
          }
        }
        r.offset = Vec<int16_t>(dx, dy);
        return r;
      }
    };

    static Optional<CompositeGlyph> Read(Header h, InputStream &in) {
      using namespace std;
      CompositeGlyph ret;
      ret.header = h;
      uint16_t flags;
      do {
        if (!in.u16(&flags)) {
          return EGLYF_NULLOPT_WHAT("Failed to read flags");
        }
        GlyphRecord rec;
        rec.flags = flags;
        if (!in.u16(&rec.glyphIndex)) {
          return EGLYF_NULLOPT_WHAT("Failed to read glyphIndex");
        }
        if (flags & ARG_1_AND_2_ARE_WORDS) {
          if (flags & ARGS_ARE_XY_VALUES) {
            Vec<int16_t> offset;
            if (!in.i16(&offset.x)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.x (i16)");
            }
            if (!in.i16(&offset.y)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.y (i16)");
            }
            rec.offset = offset;
          } else {
            Vec<uint16_t> offset;
            if (!in.u16(&offset.x)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.x (u16)");
            }
            if (!in.u16(&offset.y)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.y (u16)");
            }
            rec.offset = offset;
          }
        } else {
          if (flags & ARGS_ARE_XY_VALUES) {
            Vec<int8_t> offset;
            if (!in.i8(&offset.x)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.x (i8)");
            }
            if (!in.i8(&offset.y)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.y (i8)");
            }
            rec.offset = offset;
          } else {
            Vec<uint8_t> offset;
            if (!in.u8(&offset.x)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.x (u8)");
            }
            if (!in.u8(&offset.y)) {
              return EGLYF_NULLOPT_WHAT("Failed to read offset.y (u8)");
            }
            rec.offset = offset;
          }
        }
        if (flags & WE_HAVE_A_SCALE) {
          F2DOT14 scale;
          if (!in.f2dot14(&scale)) {
            return EGLYF_NULLOPT_WHAT("Failed to read scale (f2dot14)");
          }
          rec.scale = scale;
        } else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
          F2DOT14 xscale;
          F2DOT14 yscale;
          if (!in.f2dot14(&xscale)) {
            return EGLYF_NULLOPT_WHAT("Failed to read xscale (f2dot14)");
          }
          if (!in.f2dot14(&yscale)) {
            return EGLYF_NULLOPT_WHAT("Failed to read yscale (f2dot14)");
          }
          rec.scale = Vec<F2DOT14>(xscale, yscale);
        } else if (flags & WE_HAVE_A_TWO_BY_TWO) {
          F2DOT14 xscale;
          if (!in.f2dot14(&xscale)) {
            return EGLYF_NULLOPT_WHAT("Failed to read xscale (f2dot14) for two-by-two");
          }
          F2DOT14 scale01;
          if (!in.f2dot14(&scale01)) {
            return EGLYF_NULLOPT_WHAT("Failed to read scale01 (f2dot14)");
          }
          F2DOT14 scale10;
          if (!in.f2dot14(&scale10)) {
            return EGLYF_NULLOPT_WHAT("Failed to read scale10 (f2dot14)");
          }
          F2DOT14 yscale;
          if (!in.f2dot14(&yscale)) {
            return EGLYF_NULLOPT_WHAT("Failed to read yscale (f2dot14) for two-by-two");
          }
          rec.scale = Vec<F2DOT14>(xscale, yscale);
          rec.scale2 = Vec<F2DOT14>(scale01, scale10);
        }
        ret.records.push_back(rec);
      } while (flags & MORE_COMPONENTS);
      if (flags & WE_HAVE_INSTRUCTIONS) {
        uint16_t numInstr;
        if (!in.u16(&numInstr)) {
          return EGLYF_NULLOPT_WHAT("Failed to read numInstr");
        }
        string instructions;
        instructions.resize(numInstr);
        if (in.read(instructions.data(), numInstr) != numInstr) {
          return EGLYF_NULLOPT_WHAT("Failed to read instructions data");
        }
        ret.instructions = instructions;
      }
      return ret;
    }

    Status encode(OutputStream &out) const {
      using namespace std;
      if (auto st = header.encode(out); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
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
        } else if (instructions) {
          flg |= WE_HAVE_INSTRUCTIONS;
        }
        if (!out.u16(flg)) {
          return EGLYF_ERROR_WHAT("Failed to write flags");
        }
        if (!out.u16(rec.glyphIndex)) {
          return EGLYF_ERROR_WHAT("Failed to write glyphIndex");
        }
        if (holds_alternative<Vec<int16_t>>(rec.offset)) {
          auto const &o = get<Vec<int16_t>>(rec.offset);
          if (!out.i16(o.x)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.x (i16)");
          }
          if (!out.i16(o.y)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.y (i16)");
          }
        } else if (holds_alternative<Vec<uint16_t>>(rec.offset)) {
          auto const &o = get<Vec<uint16_t>>(rec.offset);
          if (!out.u16(o.x)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.x (u16)");
          }
          if (!out.u16(o.y)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.y (u16)");
          }
        } else if (holds_alternative<Vec<int8_t>>(rec.offset)) {
          auto const &o = get<Vec<int8_t>>(rec.offset);
          if (!out.i8(o.x)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.x (i8)");
          }
          if (!out.i8(o.y)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.y (i8)");
          }
        } else if (holds_alternative<Vec<uint8_t>>(rec.offset)) {
          auto const &o = get<Vec<uint8_t>>(rec.offset);
          if (!out.u8(o.x)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.x (u8)");
          }
          if (!out.u8(o.y)) {
            return EGLYF_ERROR_WHAT("Failed to write offset.y (u8)");
          }
        }
        if (rec.scale) {
          if (holds_alternative<F2DOT14>(*rec.scale)) {
            auto s1 = get<F2DOT14>(*rec.scale);
            if (!out.f2dot14(s1)) {
              return EGLYF_ERROR_WHAT("Failed to write scale (f2dot14)");
            }
          } else if (holds_alternative<Vec<F2DOT14>>(*rec.scale)) {
            auto s1 = get<Vec<F2DOT14>>(*rec.scale);
            if (rec.scale2) {
              if (!out.f2dot14(s1.x)) {
                return EGLYF_ERROR_WHAT("Failed to write scale.x (f2dot14)");
              }
              if (!out.f2dot14(rec.scale2->x)) {
                return EGLYF_ERROR_WHAT("Failed to write scale2.x (f2dot14)");
              }
              if (!out.f2dot14(rec.scale2->y)) {
                return EGLYF_ERROR_WHAT("Failed to write scale2.y (f2dot14)");
              }
              if (!out.f2dot14(s1.y)) {
                return EGLYF_ERROR_WHAT("Failed to write scale.y (f2dot14)");
              }
            } else {
              if (!out.f2dot14(s1.x)) {
                return EGLYF_ERROR_WHAT("Failed to write scale.x (f2dot14)");
              }
              if (!out.f2dot14(s1.y)) {
                return EGLYF_ERROR_WHAT("Failed to write scale.y (f2dot14)");
              }
            }
          }
        }
      }
      if (instructions) {
        if (!out.sizeU16(instructions->size())) {
          return EGLYF_ERROR_WHAT("Failed to write instructions size");
        }
        if (!out.write(instructions->data(), instructions->size())) {
          return EGLYF_ERROR_WHAT("Failed to write instructions data");
        }
      }
      return Status::Ok();
    }

    Header header;
    std::vector<GlyphRecord> records;
    std::optional<std::string> instructions;
  };

  using Glyph = std::variant<EmptyGlyph, ReadonlyGlyph, SimpleGlyph, CompositeGlyph>;

  static Status Read(InputStream &in, IndexToLocationTable const &loca, std::shared_ptr<GlyphDataTable> &out) {
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
        return EGLYF_ERROR_WHAT("Failed to seek to glyph offset");
      }
      string buffer;
      buffer.resize(length);
      if (length != in.read(buffer.data(), length)) {
        return EGLYF_ERROR_WHAT("Failed to read glyph data");
      }
      ByteInputStream slice(buffer);
      auto header = Header::Read(slice);
      if (!header) {
        return EGLYF_STATUS_PUSH(header.status());
      }
      if (header->numberOfContours < 0) {
        if (auto cg = CompositeGlyph::Read(*header, slice); cg) {
          ret->glyphs.push_back(*cg);
        } else {
          return EGLYF_STATUS_PUSH(cg.status());
        }
      } else {
        if (auto rg = ReadonlyGlyph::Read(*header, slice); rg) {
          ret->glyphs.push_back(*rg);
        } else {
          return EGLYF_STATUS_PUSH(rg.status());
        }
      }
    }
    out.swap(ret);
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    return EGLYF_NULLOPT_WHAT("Direct encoding of GlyphDataTable is not supported, use encode(offsets, padding) instead");
  }

  Optional<EncodeResult> encode(std::vector<Offset32> &offsets, size_t padding) const {
    using namespace std;

    offsets.clear();

    ByteOutputStream out;
    string pad;
    for (auto const &g : glyphs) {
      offsets.push_back(out.size());
      if (holds_alternative<EmptyGlyph>(g)) {
        // nop
      } else if (holds_alternative<ReadonlyGlyph>(g)) {
        auto rg = get<ReadonlyGlyph>(g);
        if (auto st = rg.encode(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      } else if (holds_alternative<SimpleGlyph>(g)) {
        auto sg = get<SimpleGlyph>(g);
        if (auto st = sg.encode(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      } else if (holds_alternative<CompositeGlyph>(g)) {
        auto cg = get<CompositeGlyph>(g);
        if (auto st = cg.encode(out); !st.ok()) {
          return EGLYF_NULLOPT_PUSH(st);
        }
      }
      if (padding > 1) {
        if (pad.size() != padding) {
          pad = string(padding, '\0');
        }
        auto size = out.size();
        if (size % padding != 0) {
          size_t cnt = padding - size % padding;
          if (!out.write(pad.data(), cnt)) {
            return EGLYF_NULLOPT_WHAT("Failed to write padding");
          }
        }
      }
    }
    offsets.push_back(out.size());
    return EncodeResult(out.data());
  }

  Optional<uint16_t> addEmptyGlyph() {
    EmptyGlyph add;
    uint16_t gid = glyphs.size();
    glyphs.push_back(add);
    return gid;
  }

  Optional<uint16_t> addCompositeGlyph(GlyphDataTable::CompositeGlyph::GlyphRecord child, MaximumProfileTable &maxp) {
    using namespace std;
    if (child.glyphIndex >= glyphs.size()) {
      return EGLYF_NULLOPT_WHAT("Child glyph index out of range");
    }
    auto g = glyphs[child.glyphIndex];
    uint16_t gid = glyphs.size();
    CompositeGlyph add;
    float xscale = 1;
    float yscale = 1;
    float scale01 = 0;
    float scale10 = 0;
    float dx = 0;
    float dy = 0;
    if (child.scale) {
      if (holds_alternative<F2DOT14>(*child.scale)) {
        auto const &scale = get<F2DOT14>(*child.scale);
        xscale = scale.toFloat();
        yscale = scale.toFloat();
      } else if (holds_alternative<Vec<F2DOT14>>(*child.scale)) {
        auto const &scale = get<Vec<F2DOT14>>(*child.scale);
        xscale = scale.x.toFloat();
        yscale = scale.y.toFloat();
      }
    }
    if (child.scale2) {
      scale01 = child.scale2->x.toFloat();
      scale10 = child.scale2->y.toFloat();
    }
    if (holds_alternative<Vec<uint8_t>>(child.offset)) {
      auto const &offset = get<Vec<uint8_t>>(child.offset);
      dx = offset.x;
      dy = offset.y;
    } else if (holds_alternative<Vec<int8_t>>(child.offset)) {
      auto const &offset = get<Vec<int8_t>>(child.offset);
      dx = offset.x;
      dy = offset.y;
    } else if (holds_alternative<Vec<uint16_t>>(child.offset)) {
      auto const &offset = get<Vec<uint16_t>>(child.offset);
      dx = offset.x;
      dy = offset.y;
    } else if (holds_alternative<Vec<int16_t>>(child.offset)) {
      auto const &offset = get<Vec<int16_t>>(child.offset);
      dx = offset.x;
      dy = offset.y;
    }
    if (holds_alternative<ReadonlyGlyph>(g)) {
      auto rg = get<ReadonlyGlyph>(g);
      add.header = rg.header;
      Vec<float> topLeft = Vec<float>(rg.header.xMin, rg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> topRight = Vec<float>(rg.header.xMax, rg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomLeft = Vec<float>(rg.header.xMin, rg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomRight = Vec<float>(rg.header.xMax, rg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      add.header.xMin = (int16_t)floor(min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.xMax = (int16_t)ceil(max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.yMin = (int16_t)floor(min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
      add.header.yMax = (int16_t)ceil(max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
    } else if (holds_alternative<SimpleGlyph>(g)) {
      auto sg = get<SimpleGlyph>(g);
      add.header = sg.header;
      Vec<float> topLeft = Vec<float>(sg.header.xMin, sg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> topRight = Vec<float>(sg.header.xMax, sg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomLeft = Vec<float>(sg.header.xMin, sg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomRight = Vec<float>(sg.header.xMax, sg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      add.header.xMin = (int16_t)floor(min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.xMax = (int16_t)ceil(max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.yMin = (int16_t)floor(min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
      add.header.yMax = (int16_t)ceil(max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
    } else if (holds_alternative<CompositeGlyph>(g)) {
      auto cg = get<CompositeGlyph>(g);
      add.header = cg.header;
      Vec<float> topLeft = Vec<float>(cg.header.xMin, cg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> topRight = Vec<float>(cg.header.xMax, cg.header.yMin).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomLeft = Vec<float>(cg.header.xMin, cg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      Vec<float> bottomRight = Vec<float>(cg.header.xMax, cg.header.yMax).transform(xscale, scale10, scale01, yscale, dx, dy);
      add.header.xMin = (int16_t)floor(min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.xMax = (int16_t)ceil(max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x}));
      add.header.yMin = (int16_t)floor(min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
      add.header.yMax = (int16_t)ceil(max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y}));
    } else {
      return EGLYF_NULLOPT_WHAT("Cannot create composite glyph from empty glyph");
    }
    add.header.numberOfContours = -1;
    add.records.push_back(child);
    glyphs.push_back(add);

    uint16_t depth = 2;
    set<uint16_t> path;
    path.insert(gid);
    uint16_t compositePoints = 0;
    uint16_t compositeContours = 0;
    for (auto const &record : add.records) {
      if (!visit(record, depth, path, maxp, compositePoints, compositeContours)) {
        return EGLYF_NULLOPT_WHAT("Failed to visit composite glyph record");
      }
    }
    maxp.maxComponentElements = (std::max)(maxp.maxComponentElements, (uint16_t)add.records.size());
    maxp.maxCompositePoints = (std::max)(maxp.maxCompositePoints, compositePoints);
    maxp.maxCompositeContours = (std::max)(maxp.maxCompositeContours, compositeContours);

    return gid;
  }

  Status clone(std::shared_ptr<GlyphDataTable> &out) const {
    using namespace std;
    IndexToLocationTable loca(1);
    auto encoded = encode(loca.offsets, 1);
    if (!encoded) {
      return EGLYF_STATUS_PUSH(encoded.status());
    }
    ByteInputStream in(encoded->data);
    shared_ptr<GlyphDataTable> ret;
    if (auto st = Read(in, loca, ret); st.ok()) {
      ret.swap(out);
      return Status::Ok();
    } else {
      return EGLYF_STATUS_PUSH(st);
    }
  }

  static std::optional<Rect<int16_t>> Bounds(Glyph const &glyph) {
    using namespace std;
    if (holds_alternative<GlyphDataTable::ReadonlyGlyph>(glyph)) {
      auto const &r = get<GlyphDataTable::ReadonlyGlyph>(glyph);
      return Rect<int16_t>(r.header.xMin, r.header.yMin, r.header.xMax, r.header.yMax);
    } else if (holds_alternative<GlyphDataTable::SimpleGlyph>(glyph)) {
      auto const &s = get<GlyphDataTable::SimpleGlyph>(glyph);
      return Rect<int16_t>(s.header.xMin, s.header.yMin, s.header.xMax, s.header.yMax);
    } else if (holds_alternative<GlyphDataTable::CompositeGlyph>(glyph)) {
      auto const &c = get<GlyphDataTable::CompositeGlyph>(glyph);
      return Rect<int16_t>(c.header.xMin, c.header.yMin, c.header.xMax, c.header.yMax);
    } else {
      return nullopt;
    }
  }

private:
  bool visit(CompositeGlyph::GlyphRecord const &record, uint16_t depth, std::set<uint16_t> path, MaximumProfileTable &out, uint16_t &compositePoints, uint16_t &compositeContours) const {
    using namespace std;
    if (record.glyphIndex >= glyphs.size()) {
      return false;
    }
    if (path.find(record.glyphIndex) != path.end()) {
      // recursive composition
      return false;
    }
    auto const &child = glyphs[record.glyphIndex];
    if (holds_alternative<CompositeGlyph>(child)) {
      auto const &cchild = get<CompositeGlyph>(child);
      out.maxComponentDepth = (std::max)(out.maxComponentDepth, depth);
      auto next = path;
      next.insert(record.glyphIndex);
      for (auto const &r : cchild.records) {
        if (!visit(r, depth + 1, next, out, compositePoints, compositeContours)) {
          return false;
        }
      }
      return true;
    } else if (holds_alternative<ReadonlyGlyph>(child)) {
      auto const &rchild = get<ReadonlyGlyph>(child);
      compositePoints += rchild.numPoints;
      compositeContours += rchild.header.numberOfContours;
      return true;
    } else if (holds_alternative<SimpleGlyph>(child)) {
      auto const &schild = get<SimpleGlyph>(child);
      compositePoints += schild.numPoints;
      compositeContours += schild.header.numberOfContours;
      return true;
    } else {
      return false;
    }
  }

public:
  std::vector<Glyph> glyphs;
};

} // namespace eglyf
