#pragma once

namespace eglyf {

class CartoucheGlyph {
  CartoucheGlyph() = delete;

public:
  static Status Create(FontFile &font, int16_t base, int16_t hfu, int16_t sb, int hhu, int chu, int16_t vfu, int vhu) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Point = glyf::GlyphDataTable::Point;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    int16_t scale = min(chu * hfu, vhu * vfu);
    int16_t lineWidth = max(1, scale / 32);
    int16_t vSpace = lineWidth;
    int16_t top = base + vfu * vhu + vSpace + lineWidth;
    int16_t bottom = base - vSpace - lineWidth;
    int16_t height = top - bottom;
    int16_t width = height * 5 / 16;
    int16_t jointLength = max(1, lineWidth / 2);
    int16_t approachLength = lineWidth / 2;
    int16_t sideBearing = 2 * lineWidth;

    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;

    {
      // cbL
      auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height, width);
      auto [in4, in3, in2, in1] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height - 2 * lineWidth, width - lineWidth);
      Contour c;
      c.points.emplace_back(out1.p0.x + approachLength + jointLength, out1.p0.y);

      c.points.emplace_back(out1.p0.x, out1.p0.y);
      c.points.emplace_back(out1.p1.x, out1.p1.y, true);
      c.points.emplace_back(out1.p2.x, out1.p2.y);
      c.points.emplace_back(out2.p1.x, out2.p1.y, true);
      c.points.emplace_back(out2.p2.x, out2.p2.y);
      c.points.emplace_back(out3.p1.x, out3.p1.y, true);
      c.points.emplace_back(out3.p2.x, out3.p2.y);
      c.points.emplace_back(out4.p1.x, out4.p1.y, true);
      c.points.emplace_back(out4.p2.x, out4.p2.y);

      c.points.emplace_back(out4.p2.x + approachLength + jointLength, out4.p2.y);
      c.points.emplace_back(in1.p2.x + approachLength + jointLength, in1.p2.y);

      c.points.emplace_back(in1.p2.x, in1.p2.y);
      c.points.emplace_back(in1.p1.x, in1.p1.y, true);
      c.points.emplace_back(in1.p0.x, in1.p0.y);
      c.points.emplace_back(in2.p1.x, in2.p1.y, true);
      c.points.emplace_back(in2.p0.x, in2.p0.y);
      c.points.emplace_back(in3.p1.x, in3.p1.y, true);
      c.points.emplace_back(in3.p0.x, in3.p0.y);
      c.points.emplace_back(in4.p1.x, in4.p1.y, true);
      c.points.emplace_back(in4.p0.x, in4.p0.y);

      c.points.emplace_back(in4.p0.x + approachLength + jointLength, in4.p0.y);

      int16_t advanceWidth = sideBearing + width + approachLength;
      auto cbL = font.addSimpleGlyph("cbL", Class::Base, {c}, advanceWidth, sideBearing);
      if (!cbL) {
        return EGLYF_STATUS_PUSH(cbL.status());
      }

      auto cbR = font.addCompositeGlyph("cbR", Class::Base, GlyphRecord::New(*cbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!cbR) {
        return EGLYF_STATUS_PUSH(cbR.status());
      }
    }
    Status st;
    st = [&]() -> Status {
      // crbL
      int16_t sideBearing = 2 * lineWidth;
      auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height, width);
      Contour c;
      vector<float> t;
      float cutX = sideBearing + lineWidth - lineWidth * 9 / 32;
      out1.getTWhenX(cutX, t);
      if (t.empty()) {
        out2.getTWhenX(cutX, t);
        if (t.empty()) {
          return EGLYF_ERROR;
        }
        auto [out2_, _] = out2.cut(t[0]);
        auto joint = out2_.p2;

        c.points.emplace_back(out1.p0.x + approachLength + jointLength, out1.p0.y);

        c.points.emplace_back(out1.p0.x, out1.p0.y);
        c.points.emplace_back(out1.p1.x, out1.p1.y, true);
        c.points.emplace_back(out1.p2.x, out1.p2.y);
        c.points.emplace_back(out2_.p1.x, out2_.p1.y, true);
        c.points.emplace_back(out2_.p2.x, out2_.p2.y);
        c.points.emplace_back(joint.x, joint.y);

        c.points.emplace_back(joint.x, bottom + height);
        c.points.emplace_back(joint.x - lineWidth, bottom + height);
        c.points.emplace_back(joint.x - lineWidth, bottom);
        c.points.emplace_back(joint.x, bottom);
      } else {
        auto [out1_, _] = out1.cut(t[0]);
        auto joint = out1_.p2;

        c.points.emplace_back(out1_.p0.x + approachLength + jointLength, out1_.p0.y);

        c.points.emplace_back(out1_.p0.x, out1_.p0.y);
        c.points.emplace_back(out1_.p1.x, out1_.p1.y, true);
        c.points.emplace_back(joint.x, joint.y);

        c.points.emplace_back(joint.x, bottom + height);
        c.points.emplace_back(joint.x - lineWidth, bottom + height);
        c.points.emplace_back(joint.x - lineWidth, bottom);
        c.points.emplace_back(joint.x, bottom);
      }
      t.clear();
      out3.getTWhenX(cutX, t);
      if (t.empty()) {
        out4.getTWhenX(cutX, t);
        if (t.empty()) {
          return EGLYF_ERROR;
        }
        auto [_, out4_] = out4.cut(t[0]);
        c.points.emplace_back(out4_.p0.x, out4_.p0.y);
        c.points.emplace_back(out4_.p1.x, out4_.p1.y, true);
        c.points.emplace_back(out4_.p2.x, out4_.p2.y);
        c.points.emplace_back(out4_.p2.x + approachLength + jointLength, out4_.p2.y);
      } else {
        auto [_, out3_] = out3.cut(t[0]);
        c.points.emplace_back(out3_.p0.x, out3_.p0.y);
        c.points.emplace_back(out3_.p1.x, out3_.p1.y, true);
        c.points.emplace_back(out3_.p2.x, out3_.p2.y);
        c.points.emplace_back(out4.p1.x, out4.p1.y, true);
        c.points.emplace_back(out4.p2.x, out4.p2.y);

        c.points.emplace_back(out4.p2.x + approachLength + jointLength, out4.p2.y);
      }
      auto [in4, in3, in2, in1] = QuadraticBezier::LeftCartouche(Vec<int16_t>(sideBearing + width, bottom + height / 2), height - 2 * lineWidth, width - lineWidth);
      c.points.emplace_back(in1.p2.x + approachLength + jointLength, in1.p2.y);

      c.points.emplace_back(in1.p2.x, in1.p2.y);
      c.points.emplace_back(in1.p1.x, in1.p1.y, true);
      c.points.emplace_back(in1.p0.x, in1.p0.y);
      c.points.emplace_back(in2.p1.x, in2.p1.y, true);
      c.points.emplace_back(in2.p0.x, in2.p0.y);
      c.points.emplace_back(in3.p1.x, in3.p1.y, true);
      c.points.emplace_back(in3.p0.x, in3.p0.y);
      c.points.emplace_back(in4.p1.x, in4.p1.y, true);
      c.points.emplace_back(in4.p0.x, in4.p0.y);

      c.points.emplace_back(in4.p0.x + approachLength + jointLength, in4.p0.y);

      int16_t advanceWidth = sideBearing + width + approachLength;
      auto crbL = font.addSimpleGlyph("crbL", Class::Base, {c}, advanceWidth, cutX - lineWidth);
      if (!crbL) {
        return EGLYF_STATUS_PUSH(crbL.status());
      }

      auto ceR = font.addCompositeGlyph("ceR", Class::Base, GlyphRecord::New(*crbL), advanceWidth, cutX - lineWidth);
      if (!ceR) {
        return EGLYF_STATUS_PUSH(ceR.status());
      }
      auto crbR = font.addCompositeGlyph("crbR", Class::Base, GlyphRecord::New(*crbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!crbR) {
        return EGLYF_STATUS_PUSH(crbR.status());
      }
      auto ceL = font.addCompositeGlyph("ceL", Class::Base, GlyphRecord::New(*crbR), advanceWidth, -jointLength);
      if (!ceL) {
        return EGLYF_STATUS_PUSH(ceL.status());
      }
      return Status::Ok();
    }();
    if (!st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    {
      // hwtbL
      int16_t w = hfu;
      int16_t advanceWidth = sideBearing + w;
      Contour c;
      c.points.emplace_back(sideBearing, top);
      c.points.emplace_back(sideBearing + w + jointLength, top);
      c.points.emplace_back(sideBearing + w + jointLength, top - lineWidth);
      c.points.emplace_back(sideBearing + lineWidth, top - lineWidth);
      c.points.emplace_back(sideBearing + lineWidth, bottom + lineWidth);
      c.points.emplace_back(sideBearing + w + jointLength, bottom + lineWidth);
      c.points.emplace_back(sideBearing + w + jointLength, bottom);
      c.points.emplace_back(sideBearing, bottom);
      auto gid = font.addSimpleGlyph("hwtbL", Class::Base, {c}, advanceWidth, sideBearing);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    {
      // O33aeL
      int16_t h1 = (int16_t)round(scale * 0.093333f);
      int16_t h2 = (int16_t)round(scale * 0.089118f);
      int16_t h3 = (int16_t)round(scale * 0.483114f);
      int16_t vBase = (int16_t)floor((height - lineWidth * 7) / 6.0f);
      int16_t remainingV = height - lineWidth * 7 - vBase * 6;
      array<int16_t, 6> v;
      ranges::fill(v, vBase);
      while (remainingV > 0) {
        for (size_t i = 0; i < v.size() && remainingV > 0; i++) {
          v[i] += 1;
          remainingV--;
        }
      }
      array<int16_t, 10> x;
      array<int16_t, 14> y;
      x[0] = -jointLength;
      x[1] = 0;
      x[2] = x[1] + lineWidth;
      x[3] = x[2] + h1;
      x[4] = x[3] + lineWidth;
      x[5] = x[4] + h2;
      x[6] = x[5] + lineWidth;
      x[7] = x[6] + h2;
      x[8] = x[7] + lineWidth;
      x[9] = x[8] + h3;
      y[0] = top;
      y[1] = y[0] - lineWidth;
      y[2] = y[1] - v[0];
      y[3] = y[2] - lineWidth;
      y[4] = y[3] - v[1];
      y[5] = y[4] - lineWidth;
      y[6] = y[5] - v[2];
      y[7] = y[6] - lineWidth;
      y[8] = y[7] - v[3];
      y[9] = y[8] - lineWidth;
      y[10] = y[9] - v[4];
      y[11] = y[10] - lineWidth;
      y[12] = y[11] - v[5];
      y[13] = y[12] - lineWidth;
      Contour c0;
      c0.points.emplace_back(x[0], y[0]);
      c0.points.emplace_back(x[9], y[0]);
      c0.points.emplace_back(x[9], y[1]);
      c0.points.emplace_back(x[8], y[1]);
      c0.points.emplace_back(x[8], y[2]);
      c0.points.emplace_back(x[9], y[2]);
      c0.points.emplace_back(x[9], y[3]);
      c0.points.emplace_back(x[8], y[3]);
      c0.points.emplace_back(x[8], y[4]);
      c0.points.emplace_back(x[9], y[4]);
      c0.points.emplace_back(x[9], y[5]);
      c0.points.emplace_back(x[8], y[5]);
      c0.points.emplace_back(x[8], y[6]);
      c0.points.emplace_back(x[9], y[6]);
      c0.points.emplace_back(x[9], y[7]);
      c0.points.emplace_back(x[8], y[7]);
      c0.points.emplace_back(x[8], y[8]);
      c0.points.emplace_back(x[9], y[8]);
      c0.points.emplace_back(x[9], y[9]);
      c0.points.emplace_back(x[8], y[9]);
      c0.points.emplace_back(x[8], y[10]);
      c0.points.emplace_back(x[9], y[10]);
      c0.points.emplace_back(x[9], y[11]);
      c0.points.emplace_back(x[8], y[11]);
      c0.points.emplace_back(x[8], y[12]);
      c0.points.emplace_back(x[9], y[12]);
      c0.points.emplace_back(x[9], y[13]);
      c0.points.emplace_back(x[0], y[13]);
      c0.points.emplace_back(x[0], y[12]);
      c0.points.emplace_back(x[1], y[12]);
      c0.points.emplace_back(x[1], y[1]);
      c0.points.emplace_back(x[0], y[1]);
      Contour c1;
      c1.points.emplace_back(x[3], y[12]);
      c1.points.emplace_back(x[3], y[1]);
      c1.points.emplace_back(x[2], y[1]);
      c1.points.emplace_back(x[2], y[12]);
      Contour c2;
      c2.points.emplace_back(x[4], y[4]);
      c2.points.emplace_back(x[5], y[4]);
      c2.points.emplace_back(x[5], y[1]);
      c2.points.emplace_back(x[4], y[1]);
      Contour c3;
      c3.points.emplace_back(x[6], y[4]);
      c3.points.emplace_back(x[7], y[4]);
      c3.points.emplace_back(x[7], y[1]);
      c3.points.emplace_back(x[6], y[1]);
      Contour c4;
      c4.points.emplace_back(x[4], y[8]);
      c4.points.emplace_back(x[5], y[8]);
      c4.points.emplace_back(x[5], y[5]);
      c4.points.emplace_back(x[4], y[5]);
      Contour c5;
      c5.points.emplace_back(x[6], y[8]);
      c5.points.emplace_back(x[7], y[8]);
      c5.points.emplace_back(x[7], y[5]);
      c5.points.emplace_back(x[6], y[5]);
      Contour c6;
      c6.points.emplace_back(x[4], y[12]);
      c6.points.emplace_back(x[5], y[12]);
      c6.points.emplace_back(x[5], y[9]);
      c6.points.emplace_back(x[4], y[9]);
      Contour c7;
      c7.points.emplace_back(x[7], y[12]);
      c7.points.emplace_back(x[7], y[9]);
      c7.points.emplace_back(x[6], y[9]);
      c7.points.emplace_back(x[6], y[12]);
      auto gid = font.addSimpleGlyph("O33aeL", Class::Base, {c0, c1, c2, c3, c4, c5, c6, c7}, x[9] + sideBearing, -jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QC{}", s);
      int16_t w = s * hfu;
      Contour c0;
      c0.points.emplace_back(-jointLength, top);
      c0.points.emplace_back(w + jointLength, top);
      c0.points.emplace_back(w + jointLength, top - lineWidth);
      c0.points.emplace_back(-jointLength, top - lineWidth);

      Contour c1;
      c1.points.emplace_back(-jointLength, bottom + lineWidth);
      c1.points.emplace_back(w + jointLength, bottom + lineWidth);
      c1.points.emplace_back(w + jointLength, bottom);
      c1.points.emplace_back(-jointLength, bottom);

      auto gid = font.addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Base, {c0, c1}, w, -jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
