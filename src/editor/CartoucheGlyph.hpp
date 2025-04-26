#pragma once

namespace eglyf {

class CartoucheGlyph {
  CartoucheGlyph() = delete;

public:
  static Status Create(FontFile &font, int16_t base, int16_t hfu, int16_t sb, int chu, int16_t vfu, int vhu) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Point = glyf::GlyphDataTable::Point;

    int16_t lineWidth = max(1, min(chu * hfu, vhu * vfu) / 48);
    int16_t vSpace = lineWidth;
    int16_t top = base + vfu * vhu + vSpace + lineWidth;
    int16_t bottom = base - vSpace - lineWidth;
    int16_t height = top - bottom;
    int16_t width = height * 5 / 16;
    int16_t jointLength = lineWidth * 2;

    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;

    {
      // cbL
      int16_t sideBearing = 2 * lineWidth;
      auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height, width);
      auto [in4, in3, in2, in1] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height - 2 * lineWidth, width - lineWidth);
      Contour c;
      c.points.emplace_back(out1.p0.x + jointLength, out1.p0.y);

      c.points.emplace_back(out1.p0.x, out1.p0.y);
      c.points.emplace_back(out1.p1.x, out1.p1.y, true);
      c.points.emplace_back(out1.p2.x, out1.p2.y);
      c.points.emplace_back(out2.p1.x, out2.p1.y, true);
      c.points.emplace_back(out2.p2.x, out2.p2.y);
      c.points.emplace_back(out3.p1.x, out3.p1.y, true);
      c.points.emplace_back(out3.p2.x, out3.p2.y);
      c.points.emplace_back(out4.p1.x, out4.p1.y, true);
      c.points.emplace_back(out4.p2.x, out4.p2.y);

      c.points.emplace_back(out4.p2.x + jointLength, out4.p2.y);
      c.points.emplace_back(in1.p2.x + jointLength, in1.p2.y);

      c.points.emplace_back(in1.p2.x, in1.p2.y);
      c.points.emplace_back(in1.p1.x, in1.p1.y, true);
      c.points.emplace_back(in1.p0.x, in1.p0.y);
      c.points.emplace_back(in2.p1.x, in2.p1.y, true);
      c.points.emplace_back(in2.p0.x, in2.p0.y);
      c.points.emplace_back(in3.p1.x, in3.p1.y, true);
      c.points.emplace_back(in3.p0.x, in3.p0.y);
      c.points.emplace_back(in4.p1.x, in4.p1.y, true);
      c.points.emplace_back(in4.p0.x, in4.p0.y);

      c.points.emplace_back(in4.p0.x + jointLength, in4.p0.y);

      auto gid = font.addSimpleGlyph("cbL", gdef::GlyphDefinitionTable::Class::Base, {c}, sideBearing + width, sideBearing);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    {
      // cbR
      int16_t sideBearing = 2 * lineWidth;
      auto [out1, out2, out3, out4] = QuadraticBezier::RightCartouche(Vec<int16_t>(0, bottom + height / 2), height, width);
      auto [in4, in3, in2, in1] = QuadraticBezier::RightCartouche(Vec<int16_t>(0, bottom + height / 2), height - 2 * lineWidth, width - lineWidth);
      Contour c;
      c.points.emplace_back(out1.p0.x - jointLength, out1.p0.y);

      c.points.emplace_back(out1.p0.x, out1.p0.y);
      c.points.emplace_back(out1.p1.x, out1.p1.y, true);
      c.points.emplace_back(out1.p2.x, out1.p2.y);
      c.points.emplace_back(out2.p1.x, out2.p1.y, true);
      c.points.emplace_back(out2.p2.x, out2.p2.y);
      c.points.emplace_back(out3.p1.x, out3.p1.y, true);
      c.points.emplace_back(out3.p2.x, out3.p2.y);
      c.points.emplace_back(out4.p1.x, out4.p1.y, true);
      c.points.emplace_back(out4.p2.x, out4.p2.y);

      c.points.emplace_back(out4.p2.x - jointLength, out4.p2.y);
      c.points.emplace_back(in1.p2.x - jointLength, in1.p2.y);

      c.points.emplace_back(in1.p2.x, in1.p2.y);
      c.points.emplace_back(in1.p1.x, in1.p1.y, true);
      c.points.emplace_back(in1.p0.x, in1.p0.y);
      c.points.emplace_back(in2.p1.x, in2.p1.y, true);
      c.points.emplace_back(in2.p0.x, in2.p0.y);
      c.points.emplace_back(in3.p1.x, in3.p1.y, true);
      c.points.emplace_back(in3.p0.x, in3.p0.y);
      c.points.emplace_back(in4.p1.x, in4.p1.y, true);
      c.points.emplace_back(in4.p0.x, in4.p0.y);

      c.points.emplace_back(in4.p0.x - jointLength, in4.p0.y);

      auto gid = font.addSimpleGlyph("cbR", gdef::GlyphDefinitionTable::Class::Base, {c}, sideBearing + width, sideBearing);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
