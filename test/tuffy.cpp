// clang-format off
#include "doctest.h"
#include "eglyf.hpp"
#include <fstream>
// clang-format on

using namespace std;
using namespace eglyf;

#if 1

static Status Expand(glyf::GlyphDataTable const &glyf,
                     glyf::GlyphDataTable::CompositeGlyph const &cg,
                     Transform<int16_t> mtx,
                     std::vector<glyf::GlyphDataTable::Contour> &contours) {
  using namespace std;
  using namespace eglyf;
  for (auto const &r : cg.records) {
    auto txm = Transform<int16_t>::Concat(mtx, r.transform<int16_t>());
    auto g = glyf.glyphs[r.glyphIndex];
    if (holds_alternative<glyf::GlyphDataTable::EmptyGlyph>(g)) {
      continue;
    } else if (holds_alternative<glyf::GlyphDataTable::ReadonlyGlyph>(g)) {
      auto rg = get<glyf::GlyphDataTable::ReadonlyGlyph>(g);
      auto sg = rg.toSimpleGlyph();
      if (!sg) {
        return EGLYF_STATUS_PUSH(sg.status());
      }
      for (auto const &c : sg->contours) {
        auto tc = c.transformed(txm);
        contours.push_back(tc);
      }
    } else if (holds_alternative<glyf::GlyphDataTable::SimpleGlyph>(g)) {
      return EGLYF_ERROR;
    } else if (holds_alternative<glyf::GlyphDataTable::CompositeGlyph>(g)) {
      auto cg = get<glyf::GlyphDataTable::CompositeGlyph>(g);
      auto st = Expand(glyf, cg, txm, contours);
      if (!st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else {
      return EGLYF_ERROR;
    }
  }
  return Status::Ok();
}

static void WriteGlyph(std::string const &data, std::string const &outputFilePath, std::string const &varname, uint16_t advanceWidth) {
  using namespace std;
  using namespace eglyf;
  ofstream out(outputFilePath);
  REQUIRE(out);
  out << "#pragma once" << endl;
  out << "// clang-format off" << endl;
  out << "namespace eglyf::res::tuffy {" << endl;
  out << endl;
  if (data.empty()) {
    out << "inline std::string_view const " << varname << ";" << endl;
  } else {
    out << "namespace detail {" << endl;
    out << "inline constexpr unsigned char " << varname << "_raw[] = {" << endl;
    for (size_t i = 0; i < data.size(); i++) {
      uint8_t c = *((uint8_t *)data.data() + i);
      out << (int)c << ",";
    }
    out << "};" << endl;
    out << "}" << endl;
    out << endl;
    out << "inline std::string_view const " << varname << "{(char const*)detail::" << varname << "_raw, " << data.size() << "};" << endl;
  }
  out << "inline uint16_t constexpr " << varname << "_advanceWidth = " << advanceWidth << ";" << endl;
  out << endl;
  out << "} // namespace eglyf::res::tuff" << endl;
}

TEST_CASE("tuffy") {
  using namespace std;
  using namespace eglyf;
  namespace fs = std::filesystem;
  FileInputStream fis(fs::path("Tuffy.ttf"));
  shared_ptr<Font> font;
  REQUIRE(Font::Read(fis, font).ok());
  REQUIRE(holds_alternative<Font::TrueTypeOutlines>(font->outlines));
  auto const &outline = get<Font::TrueTypeOutlines>(font->outlines);
  auto const &glyf = outline.glyf;

  {
    ofstream out("src/glyph/hhea.hpp");
    out << "#pragma once" << endl;
    out << "// clang-format off" << endl;
    out << "namespace eglyf::res::tuffy {" << endl;
    out << endl;
    out << "inline constexpr int kAscender = " << font->hhea->ascender << ";" << endl;
    out << "inline constexpr int kDescender = " << font->hhea->descender << ";" << endl;
    out << endl;
    out << "} // namespace eglyf::res::tuffy" << endl;
  }

  for (uint32_t code = BasicGlyphs::kMinCodepoint; code <= BasicGlyphs::kMaxCodepoint; code++) {
    auto gid = font->getGlyphID(code);
    if (!gid) {
      continue;
    }
    auto name = format("code{}", code);
    auto advanceWidth = font->hmtx->getAdvanceWidth(*gid);
    REQUIRE(advanceWidth);
    auto const &glyph = glyf->glyphs[*gid];
    if (holds_alternative<glyf::GlyphDataTable::EmptyGlyph>(glyph)) {
      WriteGlyph("", format("src/glyph/{}.hpp", code), name, *advanceWidth);
    } else if (holds_alternative<glyf::GlyphDataTable::ReadonlyGlyph>(glyph)) {
      auto const &rg = get<glyf::GlyphDataTable::ReadonlyGlyph>(glyph);
      auto sg = rg.toSimpleGlyph();
      REQUIRE(sg);
      ByteOutputStream out;
      REQUIRE(sg->encode(out).ok());

      string data = out.data();
      WriteGlyph(data, format("src/glyph/{}.hpp", code), name, *advanceWidth);
    } else if (holds_alternative<glyf::GlyphDataTable::CompositeGlyph>(glyph)) {
      auto cg = get<glyf::GlyphDataTable::CompositeGlyph>(glyph);
      glyf::GlyphDataTable::SimpleGlyph sg;
      auto st = Expand(*glyf, cg, Transform<int16_t>(), sg.contours);
      REQUIRE(st.ok());
      REQUIRE(sg.contours.size() <= numeric_limits<int16_t>::max());
      sg.header.numberOfContours = sg.contours.size();
      optional<Rect<int16_t>> bounds;
      size_t numPoints = 0;
      for (auto const &c : sg.contours) {
        auto b = c.boundingBox();
        if (bounds) {
          bounds->updateBound(b.xMin, b.yMin, b.xMax, b.yMax);
        } else {
          bounds = b;
        }
        numPoints += c.points.size();
      }
      REQUIRE(bounds);
      sg.header.xMin = bounds->xMin;
      sg.header.yMin = bounds->yMin;
      sg.header.xMax = bounds->xMax;
      sg.header.yMax = bounds->yMax;
      REQUIRE(numPoints <= numeric_limits<uint16_t>::max());
      sg.numPoints = numPoints;
      ByteOutputStream out;
      REQUIRE(sg.encode(out).ok());
      string data = out.data();
      WriteGlyph(data, format("src/glyph/{}.hpp", code), name, *advanceWidth);
    } else {
      REQUIRE(false);
    }
  }
}

#endif
