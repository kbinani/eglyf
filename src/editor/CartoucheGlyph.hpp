#pragma once

namespace eglyf {

class CartoucheGlyph {
  CartoucheGlyph() = delete;

  struct Param {
    int16_t scale;
    int16_t sideBearing;
    int16_t lineWidth;
    int16_t approachLength;
    int16_t jointLength;
  };

  static Status Create_cb(FontFile &font, Param const &p, int16_t width, int16_t height, int16_t bottom, std::string const &nameL, std::string const &nameR) {
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height, width);
    auto [in4, in3, in2, in1] = QuadraticBezier::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height - 2 * p.lineWidth, width - p.lineWidth);
    Contour c;
    c.points.emplace_back(out1.p0.x + p.approachLength + p.jointLength, out1.p0.y);

    c.points.emplace_back(out1.p0.x, out1.p0.y);
    c.points.emplace_back(out1.p1.x, out1.p1.y, true);
    c.points.emplace_back(out1.p2.x, out1.p2.y);
    c.points.emplace_back(out2.p1.x, out2.p1.y, true);
    c.points.emplace_back(out2.p2.x, out2.p2.y);
    c.points.emplace_back(out3.p1.x, out3.p1.y, true);
    c.points.emplace_back(out3.p2.x, out3.p2.y);
    c.points.emplace_back(out4.p1.x, out4.p1.y, true);
    c.points.emplace_back(out4.p2.x, out4.p2.y);

    c.points.emplace_back(out4.p2.x + p.approachLength + p.jointLength, out4.p2.y);
    c.points.emplace_back(in1.p2.x + p.approachLength + p.jointLength, in1.p2.y);

    c.points.emplace_back(in1.p2.x, in1.p2.y);
    c.points.emplace_back(in1.p1.x, in1.p1.y, true);
    c.points.emplace_back(in1.p0.x, in1.p0.y);
    c.points.emplace_back(in2.p1.x, in2.p1.y, true);
    c.points.emplace_back(in2.p0.x, in2.p0.y);
    c.points.emplace_back(in3.p1.x, in3.p1.y, true);
    c.points.emplace_back(in3.p0.x, in3.p0.y);
    c.points.emplace_back(in4.p1.x, in4.p1.y, true);
    c.points.emplace_back(in4.p0.x, in4.p0.y);

    c.points.emplace_back(in4.p0.x + p.approachLength + p.jointLength, in4.p0.y);

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto cbL = font.addSimpleGlyph(nameL, Class::Base, {c}, advanceWidth, p.sideBearing);
    if (!cbL) {
      return EGLYF_STATUS_PUSH(cbL.status());
    }

    auto cbR = font.addCompositeGlyph(nameR, Class::Base, GlyphRecord::New(*cbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength);
    if (!cbR) {
      return EGLYF_STATUS_PUSH(cbR.status());
    }
    return Status::Ok();
  }

  static Status Create_crb(FontFile &font, Param const &p, int16_t width, int16_t height, int16_t bottom,
                           std::string const &name_crbL, std::string const &name_ceR, std::optional<std::string> name_crbR, std::string const &name_ceL) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height, width);
    Contour c;
    vector<float> t;
    float cutX = p.sideBearing + p.lineWidth - p.lineWidth * 9 / 32;
    out1.getTWhenX(cutX, t);
    if (t.empty()) {
      out2.getTWhenX(cutX, t);
      if (t.empty()) {
        return EGLYF_ERROR;
      }
      auto [out2_, _] = out2.cut(t[0]);
      auto joint = out2_.p2;

      c.points.emplace_back(out1.p0.x + p.approachLength + p.jointLength, out1.p0.y);

      c.points.emplace_back(out1.p0.x, out1.p0.y);
      c.points.emplace_back(out1.p1.x, out1.p1.y, true);
      c.points.emplace_back(out1.p2.x, out1.p2.y);
      c.points.emplace_back(out2_.p1.x, out2_.p1.y, true);
      c.points.emplace_back(out2_.p2.x, out2_.p2.y);
      c.points.emplace_back(joint.x, joint.y);

      c.points.emplace_back(joint.x, bottom + height);
      c.points.emplace_back(joint.x - p.lineWidth, bottom + height);
      c.points.emplace_back(joint.x - p.lineWidth, bottom);
      c.points.emplace_back(joint.x, bottom);
    } else {
      auto [out1_, _] = out1.cut(t[0]);
      auto joint = out1_.p2;

      c.points.emplace_back(out1_.p0.x + p.approachLength + p.jointLength, out1_.p0.y);

      c.points.emplace_back(out1_.p0.x, out1_.p0.y);
      c.points.emplace_back(out1_.p1.x, out1_.p1.y, true);
      c.points.emplace_back(joint.x, joint.y);

      c.points.emplace_back(joint.x, bottom + height);
      c.points.emplace_back(joint.x - p.lineWidth, bottom + height);
      c.points.emplace_back(joint.x - p.lineWidth, bottom);
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
      c.points.emplace_back(out4_.p2.x + p.approachLength + p.jointLength, out4_.p2.y);
    } else {
      auto [_, out3_] = out3.cut(t[0]);
      c.points.emplace_back(out3_.p0.x, out3_.p0.y);
      c.points.emplace_back(out3_.p1.x, out3_.p1.y, true);
      c.points.emplace_back(out3_.p2.x, out3_.p2.y);
      c.points.emplace_back(out4.p1.x, out4.p1.y, true);
      c.points.emplace_back(out4.p2.x, out4.p2.y);

      c.points.emplace_back(out4.p2.x + p.approachLength + p.jointLength, out4.p2.y);
    }
    auto [in4, in3, in2, in1] = QuadraticBezier::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height - 2 * p.lineWidth, width - p.lineWidth);
    c.points.emplace_back(in1.p2.x + p.approachLength + p.jointLength, in1.p2.y);

    c.points.emplace_back(in1.p2.x, in1.p2.y);
    c.points.emplace_back(in1.p1.x, in1.p1.y, true);
    c.points.emplace_back(in1.p0.x, in1.p0.y);
    c.points.emplace_back(in2.p1.x, in2.p1.y, true);
    c.points.emplace_back(in2.p0.x, in2.p0.y);
    c.points.emplace_back(in3.p1.x, in3.p1.y, true);
    c.points.emplace_back(in3.p0.x, in3.p0.y);
    c.points.emplace_back(in4.p1.x, in4.p1.y, true);
    c.points.emplace_back(in4.p0.x, in4.p0.y);

    c.points.emplace_back(in4.p0.x + p.approachLength + p.jointLength, in4.p0.y);

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto crbL = font.addSimpleGlyph(name_crbL, Class::Base, {c}, advanceWidth, cutX - p.lineWidth);
    if (!crbL) {
      return EGLYF_STATUS_PUSH(crbL.status());
    }

    auto ceR = font.addCompositeGlyph(name_ceR, Class::Base, GlyphRecord::New(*crbL), advanceWidth, cutX - p.lineWidth);
    if (!ceR) {
      return EGLYF_STATUS_PUSH(ceR.status());
    }
    auto ceL = font.addCompositeGlyph(name_ceL, Class::Base, GlyphRecord::New(*crbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength);
    if (!ceL) {
      return EGLYF_STATUS_PUSH(ceL.status());
    }
    if (name_crbR) {
      auto crbR = font.addCompositeGlyph(*name_crbR, Class::Base, GlyphRecord::New(*ceL), advanceWidth, -p.jointLength);
      if (!crbR) {
        return EGLYF_STATUS_PUSH(crbR.status());
      }
    }
    return Status::Ok();
  }

  static Status Create_O33aeL(FontFile &font, Param const &p, int16_t top, int16_t height) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;

    int16_t h1 = (int16_t)round(p.scale * 0.093333f);
    int16_t h2 = (int16_t)round(p.scale * 0.089118f);
    int16_t h3 = (int16_t)round(p.scale * 0.483114f);
    int16_t vBase = (int16_t)floor((height - p.lineWidth * 7) / 6.0f);
    int16_t remainingV = height - p.lineWidth * 7 - vBase * 6;
    array<int16_t, 6> v;
    ranges::fill(v, vBase);
    while (remainingV > 0) {
      for (size_t i = 0; i < v.size() && remainingV > 0; i++) {
        v[i] += 1;
        remainingV--;
      }
    }
    // https://gyazo.com/7e1c9dd2fcdf73ffe1a34125c32db697
    array<int16_t, 10> x;
    array<int16_t, 14> y;
    x[0] = -p.jointLength;
    x[1] = 0;
    x[2] = x[1] + p.lineWidth;
    x[3] = x[2] + h1;
    x[4] = x[3] + p.lineWidth;
    x[5] = x[4] + h2;
    x[6] = x[5] + p.lineWidth;
    x[7] = x[6] + h2;
    x[8] = x[7] + p.lineWidth;
    x[9] = x[8] + h3;
    y[0] = top;
    y[1] = y[0] - p.lineWidth;
    y[2] = y[1] - v[0];
    y[3] = y[2] - p.lineWidth;
    y[4] = y[3] - v[1];
    y[5] = y[4] - p.lineWidth;
    y[6] = y[5] - v[2];
    y[7] = y[6] - p.lineWidth;
    y[8] = y[7] - v[3];
    y[9] = y[8] - p.lineWidth;
    y[10] = y[9] - v[4];
    y[11] = y[10] - p.lineWidth;
    y[12] = y[11] - v[5];
    y[13] = y[12] - p.lineWidth;
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
    auto gid = font.addSimpleGlyph("O33aeL", Class::Base, {c0, c1, c2, c3, c4, c5, c6, c7}, x[9] + p.sideBearing, -p.jointLength);
    if (!gid) {
      return EGLYF_STATUS_PUSH(gid.status());
    }
    return Status::Ok();
  }

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
    int16_t approachLength = lineWidth / 2;
    int16_t sideBearing = 2 * lineWidth;
    int16_t jointLength = max(1, lineWidth / 2);

    Param p;
    p.scale = scale;
    p.lineWidth = lineWidth;
    p.approachLength = approachLength;
    p.sideBearing = sideBearing;
    p.jointLength = jointLength;

    int16_t top = base + vfu * vhu + vSpace + lineWidth;
    int16_t bottom = base - vSpace - lineWidth;
    int16_t height = top - bottom;
    int16_t width = height * 5 / 16;

    int16_t otop = top + vSpace + lineWidth;
    int16_t obottom = bottom - vSpace - lineWidth;
    int16_t oheight = otop - obottom;
    int16_t owidth = oheight * 5 / 16;

    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;

    if (auto st = Create_cb(font, p, width, height, bottom, "cbL", "cbR"); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_crb(font, p, width, height, bottom, "crbL", "ceR", "crbR", "ceL"); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_cb(font, p, owidth, oheight, obottom, "cobL", "cobR"); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_crb(font, p, owidth, oheight, obottom, "corbL", "coeR", nullopt, "coeL"); !st.ok()) {
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
    if (auto st = Create_O33aeL(font, p, top, height); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QC{}", s);
      int16_t w = sb + s * hfu + sb;
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
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QO{}", s);
      int16_t w = sb + s * hfu + sb;
      Contour c0;
      c0.points.emplace_back(-jointLength, otop);
      c0.points.emplace_back(w + jointLength, otop);
      c0.points.emplace_back(w + jointLength, otop - lineWidth);
      c0.points.emplace_back(-jointLength, otop - lineWidth);

      Contour c1;
      c1.points.emplace_back(-jointLength, obottom + lineWidth);
      c1.points.emplace_back(w + jointLength, obottom + lineWidth);
      c1.points.emplace_back(w + jointLength, obottom);
      c1.points.emplace_back(-jointLength, obottom);

      auto gid = font.addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Base, {c0, c1}, w, -jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QD{}", s);
      int16_t w = sb + s * hfu + sb;
      auto qo = font.post->getGlyphID(format("QO{}", s));
      if (!qo) {
        return EGLYF_ERROR;
      }
      auto qc = font.post->getGlyphID(format("QC{}", s));
      if (!qc) {
        return EGLYF_ERROR;
      }
      vector<GlyphRecord> children;
      children.push_back(GlyphRecord::New(*qo));
      children.push_back(GlyphRecord::New(*qc));
      auto gid = font.addCompositeGlyph(name, Class::Base, children, w, -p.jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
