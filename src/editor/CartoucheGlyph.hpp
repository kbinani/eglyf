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

  static glyf::GlyphDataTable::Contour CreateContour_cb(Param const &p, int16_t width, int16_t height, int16_t bottom) {
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

    return c;
  }

  static Status CreateContour_crb(Param const &p, int16_t width, int16_t height, int16_t bottom, glyf::GlyphDataTable::Contour &c, float &cutX) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto [out1, out2, out3, out4] = QuadraticBezier::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height, width);
    vector<float> t;
    cutX = p.sideBearing + p.lineWidth - p.lineWidth * 9 / 32;
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

    return Status::Ok();
  }

  static Status Create_cb(FontFile &font, Param const &p, int16_t width, int16_t height, int16_t bottom,
                          std::string const &name, std::vector<std::string> copy, std::vector<std::string> mirror) {
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto c = CreateContour_cb(p, width, height, bottom);

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto cbL = font.addSimpleGlyph(name, Class::Base, {c}, advanceWidth, p.sideBearing);
    if (!cbL) {
      return EGLYF_STATUS_PUSH(cbL.status());
    }

    for (auto const &c : copy) {
      auto gid = font.addCompositeGlyph(c, Class::Base, GlyphRecord::New(*cbL), advanceWidth, -p.jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (auto const &m : mirror) {
      auto gid = font.addCompositeGlyph(m, Class::Base, GlyphRecord::New(*cbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }

  static Status Create_crb(FontFile &font, Param const &p, int16_t width, int16_t height, int16_t bottom,
                           std::string const &name, std::vector<std::string> copy, std::vector<std::string> mirror) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    Contour c;
    float cutX;
    if (auto st = CreateContour_crb(p, width, height, bottom, c, cutX); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto crbL = font.addSimpleGlyph(name, Class::Base, {c}, advanceWidth, cutX - p.lineWidth);
    if (!crbL) {
      return EGLYF_STATUS_PUSH(crbL.status());
    }
    for (auto const &c : copy) {
      auto gid = font.addCompositeGlyph(c, Class::Base, GlyphRecord::New(*crbL), advanceWidth, -p.jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (auto const &m : mirror) {
      auto gid = font.addCompositeGlyph(m, Class::Base, GlyphRecord::New(*crbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }

  static void CreateContour_O33aeL(Param const &p, int16_t top, int16_t height, std::vector<glyf::GlyphDataTable::Contour> &contours, int16_t &advanceWidth) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;

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

    contours.push_back(c0);
    contours.push_back(c1);
    contours.push_back(c2);
    contours.push_back(c3);
    contours.push_back(c4);
    contours.push_back(c5);
    contours.push_back(c6);
    contours.push_back(c7);

    advanceWidth = x[9] + p.sideBearing;
  }

  static void CreateContour_hwttb(Param const &p, int16_t top, int16_t height, std::vector<glyf::GlyphDataTable::Contour> &contours, int16_t &advanceWidth, int16_t &advanceHeight) {
    using namespace std;
    int16_t width = (int16_t)round(p.scale * 0.315197f);
    int16_t h = (int16_t)round(p.scale * 0.375235f);
    int16_t w = (int16_t)round(p.scale * 0.147280f);
    using Contour = glyf::GlyphDataTable::Contour;

    Contour c0;
    c0.points.emplace_back(p.sideBearing, top - height);
    c0.points.emplace_back(p.sideBearing, top);
    c0.points.emplace_back(p.sideBearing + width + p.jointLength, top);
    c0.points.emplace_back(p.sideBearing + width + p.jointLength, top - p.lineWidth);
    c0.points.emplace_back(p.sideBearing + p.lineWidth + w + p.lineWidth, top - p.lineWidth);
    c0.points.emplace_back(p.sideBearing + p.lineWidth + w + p.lineWidth, top - p.lineWidth - h - p.lineWidth);
    c0.points.emplace_back(p.sideBearing + p.lineWidth, top - p.lineWidth - h - p.lineWidth);
    c0.points.emplace_back(p.sideBearing + p.lineWidth, top - height + p.lineWidth);
    c0.points.emplace_back(p.sideBearing + width + p.jointLength, top - height + p.lineWidth);
    c0.points.emplace_back(p.sideBearing + width + p.jointLength, top - height);
    contours.push_back(c0);

    Contour c1;
    c1.points.emplace_back(p.sideBearing + p.lineWidth + w, top - p.lineWidth - h);
    c1.points.emplace_back(p.sideBearing + p.lineWidth + w, top - p.lineWidth);
    c1.points.emplace_back(p.sideBearing + p.lineWidth, top - p.lineWidth);
    c1.points.emplace_back(p.sideBearing + p.lineWidth, top - p.lineWidth - h);
    contours.push_back(c1);

    advanceWidth = p.sideBearing + width;
    advanceHeight = 2 * top - height;
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

    if (auto st = Create_cb(font, p, width, height, bottom, "cbL", {"creR"}, {"cbR", "creL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_crb(font, p, width, height, bottom, "crbL", {"ceR"}, {"crbR", "ceL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_cb(font, p, owidth, oheight, obottom, "cobL", {"coreR"}, {"cobR", "coreL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = Create_crb(font, p, owidth, oheight, obottom, "corbL", {"coeR"}, {"corbR", "coeL"}); !st.ok()) {
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
      auto hwtbL = font.addSimpleGlyph("hwtbL", Class::Base, {c}, advanceWidth, sideBearing);
      if (!hwtbL) {
        return EGLYF_STATUS_PUSH(hwtbL.status());
      }

      for (auto const &n : {"hwtbR", "hwteL"}) {
        auto gid = font.addCompositeGlyph(n, Class::Base, GlyphRecord::New(*hwtbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      // hwtobL
      int16_t w = hfu;
      int16_t advanceWidth = sideBearing + w;
      Contour c;
      c.points.emplace_back(sideBearing, otop);
      c.points.emplace_back(sideBearing + w + jointLength, otop);
      c.points.emplace_back(sideBearing + w + jointLength, otop - lineWidth);
      c.points.emplace_back(sideBearing + lineWidth, otop - lineWidth);
      c.points.emplace_back(sideBearing + lineWidth, obottom + lineWidth);
      c.points.emplace_back(sideBearing + w + jointLength, obottom + lineWidth);
      c.points.emplace_back(sideBearing + w + jointLength, obottom);
      c.points.emplace_back(sideBearing, obottom);
      auto hwtobL = font.addSimpleGlyph("hwtobL", Class::Base, {c}, advanceWidth, sideBearing);
      if (!hwtobL) {
        return EGLYF_STATUS_PUSH(hwtobL.status());
      }

      auto hwtobR = font.addCompositeGlyph("hwtobR", Class::Base, GlyphRecord::New(*hwtobL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!hwtobR) {
        return EGLYF_STATUS_PUSH(hwtobR.status());
      }
    }
    {
      vector<Contour> contours;
      int16_t advanceWidth;
      int16_t advanceHeight;
      CreateContour_hwttb(p, top, height, contours, advanceWidth, advanceHeight);
      auto hwttbL = font.addSimpleGlyph("hwttbL", Class::Base, contours, advanceWidth, p.sideBearing);
      if (!hwttbL) {
        return EGLYF_STATUS_PUSH(hwttbL.status());
      }

      for (auto const &n : {"hwttbR", "hwtteL"}) {
        auto gid = font.addCompositeGlyph(n, Class::Base, GlyphRecord::New(*hwttbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      auto hwtteR = font.addCompositeGlyph("hwtteR", Class::Base, GlyphRecord::New(*hwttbL), advanceWidth, p.sideBearing);
      if (!hwtteR) {
        return EGLYF_STATUS_PUSH(hwtteR.status());
      }
      auto hwtbbL = font.addCompositeGlyph("hwtbbL", Class::Base, GlyphRecord::New(*hwttbL, 0, advanceHeight, Vec<float>(1, -1)), advanceWidth, p.sideBearing);
      if (!hwtbbL) {
        return EGLYF_STATUS_PUSH(hwtbbL.status());
      }
      for (auto const &n : {"hwtbbR", "hwtbeL"}) {
        auto gid = font.addCompositeGlyph(n, Class::Base, GlyphRecord::New(*hwtbbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      // hwtdbL, hwtdbR, hwtdeR, hwtdeL
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

      Contour c0;
      c0.points.emplace_back(-jointLength, otop - lineWidth);
      c0.points.emplace_back(-jointLength, otop);
      c0.points.emplace_back(advanceWidth + jointLength, otop);
      c0.points.emplace_back(advanceWidth + jointLength, otop - lineWidth);

      Contour c1;
      c1.points.emplace_back(-jointLength, obottom);
      c1.points.emplace_back(-jointLength, obottom + lineWidth);
      c1.points.emplace_back(advanceWidth + jointLength, obottom + lineWidth);
      c1.points.emplace_back(advanceWidth + jointLength, obottom);

      auto hwtdbL = font.addSimpleGlyph("hwtdbL", Class::Base, {c, c0, c1}, advanceWidth, -jointLength);
      if (!hwtdbL) {
        return EGLYF_STATUS_PUSH(hwtdbL.status());
      }
      auto hwtdbR = font.addCompositeGlyph("hwtdbR", Class::Base, GlyphRecord::New(*hwtdbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!hwtdbR) {
        return EGLYF_STATUS_PUSH(hwtdbR.status());
      }
      auto hwtdeR = font.addCompositeGlyph("hwtdeR", Class::Base, GlyphRecord::New(*hwtdbL), advanceWidth, -jointLength);
      if (!hwtdeR) {
        return EGLYF_STATUS_PUSH(hwtdeR.status());
      }
      auto hwtdeL = font.addCompositeGlyph("hwtdeL", Class::Base, GlyphRecord::New(*hwtdbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!hwtdeL) {
        return EGLYF_STATUS_PUSH(hwtdeL.status());
      }
    }
    {
      // O33aeL
      vector<Contour> contours;
      int16_t advanceWidth;
      CreateContour_O33aeL(p, top, height, contours, advanceWidth);
      auto O33aeL = font.addSimpleGlyph("O33aeL", Class::Base, contours, advanceWidth, -p.jointLength);
      if (!O33aeL) {
        return EGLYF_STATUS_PUSH(O33aeL.status());
      }
      auto O33aeR = font.addCompositeGlyph("O33aeR", Class::Base, GlyphRecord::New(*O33aeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!O33aeR) {
        return EGLYF_STATUS_PUSH(O33aeR.status());
      }
    }
    {
      // O33aoeL
      vector<Contour> contours;
      int16_t advanceWidth;
      CreateContour_O33aeL(p, otop, oheight, contours, advanceWidth);
      auto O33aoeL = font.addSimpleGlyph("O33aoeL", Class::Base, contours, advanceWidth, -p.jointLength);
      if (!O33aoeL) {
        return EGLYF_STATUS_PUSH(O33aoeL.status());
      }
      auto O33aoeR = font.addCompositeGlyph("O33aoeR", Class::Base, GlyphRecord::New(*O33aoeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!O33aoeR) {
        return EGLYF_STATUS_PUSH(O33aoeR.status());
      }
    }
    {
      // O33adeL
      vector<Contour> contours;
      int16_t advanceWidth;
      CreateContour_O33aeL(p, top, height, contours, advanceWidth);

      Contour c0;
      c0.points.emplace_back(-jointLength, otop - lineWidth);
      c0.points.emplace_back(-jointLength, otop);
      c0.points.emplace_back(advanceWidth + jointLength, otop);
      c0.points.emplace_back(advanceWidth + jointLength, otop - lineWidth);
      contours.push_back(c0);

      Contour c1;
      c1.points.emplace_back(-jointLength, obottom);
      c1.points.emplace_back(-jointLength, obottom + lineWidth);
      c1.points.emplace_back(advanceWidth + jointLength, obottom + lineWidth);
      c1.points.emplace_back(advanceWidth + jointLength, obottom);
      contours.push_back(c1);

      auto O33adeL = font.addSimpleGlyph("O33adeL", Class::Base, contours, advanceWidth, -p.jointLength);
      if (!O33adeL) {
        return EGLYF_STATUS_PUSH(O33adeL.status());
      }
      auto O33adeR = font.addCompositeGlyph("O33adeR", Class::Base, GlyphRecord::New(*O33adeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength);
      if (!O33adeR) {
        return EGLYF_STATUS_PUSH(O33adeR.status());
      }
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
    {
      // cdbL, cdbR, cdreL, cdreR
      auto c = CreateContour_cb(p, width, height, bottom);
      int16_t w = p.sideBearing + width + p.approachLength;
      Contour c0;
      c0.points.emplace_back(w + jointLength, otop);
      c0.points.emplace_back(-jointLength, otop);
      c0.points.emplace_back(-jointLength, otop - lineWidth);
      c0.points.emplace_back(w + jointLength, otop - lineWidth);

      Contour c1;
      c1.points.emplace_back(w + jointLength, obottom + lineWidth);
      c1.points.emplace_back(-jointLength, obottom + lineWidth);
      c1.points.emplace_back(-jointLength, obottom);
      c1.points.emplace_back(w + jointLength, obottom);

      auto cdbL = font.addSimpleGlyph("cdbL", Class::Base, {c0, c1, c}, w, -jointLength);
      if (!cdbL) {
        return EGLYF_STATUS_PUSH(cdbL.status());
      }

      auto cdbR = font.addCompositeGlyph("cdbR", Class::Base, GlyphRecord::New(*cdbL, w, 0, Vec<float>(-1, 1)), w, -jointLength);
      if (!cdbR) {
        return EGLYF_STATUS_PUSH(cdbR.status());
      }

      auto cdreL = font.addCompositeGlyph("cdreL", Class::Base, GlyphRecord::New(*cdbL, w, 0, Vec<float>(-1, 1)), w, -jointLength);
      if (!cdreL) {
        return EGLYF_STATUS_PUSH(cdreL.status());
      }

      auto cdreR = font.addCompositeGlyph("cdreR", Class::Base, GlyphRecord::New(*cdbL), w, -jointLength);
      if (!cdreR) {
        return EGLYF_STATUS_PUSH(cdreR.status());
      }
    }
    {
      // cdrbL, cdrbR, cdeL, cdeR
      Contour c;
      float cutX;
      if (auto st = CreateContour_crb(p, width, height, bottom, c, cutX); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      int16_t w = p.sideBearing + width + p.approachLength;
      Contour c0;
      c0.points.emplace_back(w + jointLength, otop);
      c0.points.emplace_back(-jointLength, otop);
      c0.points.emplace_back(-jointLength, otop - lineWidth);
      c0.points.emplace_back(w + jointLength, otop - lineWidth);

      Contour c1;
      c1.points.emplace_back(w + jointLength, obottom + lineWidth);
      c1.points.emplace_back(-jointLength, obottom + lineWidth);
      c1.points.emplace_back(-jointLength, obottom);
      c1.points.emplace_back(w + jointLength, obottom);

      auto cdrbL = font.addSimpleGlyph("cdrbL", Class::Base, {c0, c1, c}, w, -jointLength);
      if (!cdrbL) {
        return EGLYF_STATUS_PUSH(cdrbL.status());
      }

      auto cdrbR = font.addCompositeGlyph("cdrbR", Class::Base, GlyphRecord::New(*cdrbL, w, 0, Vec<float>(-1, 1)), w, -jointLength);
      if (!cdrbR) {
        return EGLYF_STATUS_PUSH(cdrbR.status());
      }

      auto cdeL = font.addCompositeGlyph("cdeL", Class::Base, GlyphRecord::New(*cdrbL, w, 0, Vec<float>(-1, 1)), w, -jointLength);
      if (!cdeL) {
        return EGLYF_STATUS_PUSH(cdeL.status());
      }

      auto cdeR = font.addCompositeGlyph("cdeR", Class::Base, GlyphRecord::New(*cdrbL), w, -jointLength);
      if (!cdeR) {
        return EGLYF_STATUS_PUSH(cdeR.status());
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
