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
    int16_t walledLineWidth;
    int16_t wallHeight;
  };

  static glyf::GlyphDataTable::Contour CreateContour_cb(Param const &p, int16_t width, int16_t height, int16_t bottom) {
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;
    using Contour = glyf::GlyphDataTable::Contour;

    auto [out1, out2, out3, out4] = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height, width);
    auto [in4, in3, in2, in1] = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height - 2 * p.lineWidth, width - p.lineWidth);
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
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;
    using Contour = glyf::GlyphDataTable::Contour;

    auto [out1, out2, out3, out4] = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height, width);
    array<double, 2> t;
    cutX = p.sideBearing + p.lineWidth - p.lineWidth * 9 / 32;
    size_t num = out1.getTWhenX(cutX, t);
    if (num == 0) {
      num = out2.getTWhenX(cutX, t);
      if (num == 0) {
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
    num = out3.getTWhenX(cutX, t);
    if (num == 0) {
      num = out4.getTWhenX(cutX, t);
      if (num == 0) {
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
    auto [in4, in3, in2, in1] = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(p.sideBearing + width, bottom + height / 2), height - 2 * p.lineWidth, width - p.lineWidth);
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

  static Status Create_cb(Font &font, Param const &p, int16_t width, int16_t height, int16_t bottom,
                          std::string const &name, std::vector<std::string> copy, std::vector<std::string> mirror) {
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto c = CreateContour_cb(p, width, height, bottom);

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto cbL = font.replaceSimpleGlyphByName(name, Class::Base, {c}, advanceWidth, p.sideBearing, 0, 0);
    if (!cbL) {
      return EGLYF_STATUS_PUSH(cbL.status());
    }

    for (auto const &c : copy) {
      auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*cbL), advanceWidth, -p.jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (auto const &m : mirror) {
      auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*cbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    return Status::Ok();
  }

  static Status Create_crb(Font &font, Param const &p, int16_t width, int16_t height, int16_t bottom,
                           std::string const &name, std::vector<std::string> copy, std::vector<std::string> mirror) {
    using namespace std;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;
    using Contour = glyf::GlyphDataTable::Contour;

    Contour c;
    float cutX;
    if (auto st = CreateContour_crb(p, width, height, bottom, c, cutX); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }

    int16_t advanceWidth = p.sideBearing + width + p.approachLength;
    auto crbL = font.replaceSimpleGlyphByName(name, Class::Base, {c}, advanceWidth, cutX - p.lineWidth, 0, 0);
    if (!crbL) {
      return EGLYF_STATUS_PUSH(crbL.status());
    }
    for (auto const &c : copy) {
      auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*crbL), advanceWidth, -p.jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (auto const &m : mirror) {
      auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*crbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
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
    using Contour = glyf::GlyphDataTable::Contour;

    int16_t width = (int16_t)round(p.scale * 0.315197f);
    int16_t h = (int16_t)round(p.scale * 0.375235f);
    int16_t w = (int16_t)round(p.scale * 0.147280f);

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

  static void CreateContour_cwbL(Param const &p, int16_t hfu, int16_t width, int16_t bottom, int16_t height, glyf::GlyphDataTable::Contour &c) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;

    auto sideBearing = p.sideBearing;
    auto jointLength = p.jointLength;
    auto approachLength = p.approachLength;
    auto lineWidth = p.lineWidth;
    auto walledLineWidth = p.walledLineWidth;
    auto wallHeight = p.wallHeight;

    Vec<int16_t> center(Vec<int16_t>(p.sideBearing + width, bottom + height / 2));
    array<QuadraticBezier<int16_t>, 4> out = QuadraticBezier<int16_t>::LeftCartouche(center, height - 2 * lineWidth + 2 * walledLineWidth, width - lineWidth + walledLineWidth);
    auto [out1, out2, out3, out4] = out;
    auto [in4, in3, in2, in1] = QuadraticBezier<int16_t>::LeftCartouche(center, height - 2 * lineWidth, width - lineWidth);

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
    c.points.emplace_back(in4.p0.x + p.approachLength + p.jointLength, in4.p0.y + walledLineWidth);

    double const length = out1.length() + out2.length() + out3.length() + out4.length();
    int const numSections = (int)round(length / hfu);
    double const sectionLength = length / numSections;
    double currentLength = 0;
    int curveIndex = 0;
    double curveOffset = 0;

    c.points.emplace_back(out1.p0.x, out1.p0.y);
    QuadraticBezier<int16_t> current = out1;

    for (int i = 0; i < numSections; i++) {
      // up
      double nextLength = sectionLength * i + sectionLength / 5;
      double curveLength = current.length();
      double s0 = (nextLength - curveOffset) / curveLength;
      while (s0 > 1) {
        c.points.emplace_back(current.p1.x, current.p1.y, true);
        c.points.emplace_back(current.p2.x, current.p2.y);
        curveIndex++;
        if (curveIndex >= out.size()) {
          break;
        }
        curveOffset += curveLength;
        currentLength += curveLength;
        current = out[curveIndex];
        curveLength = current.length();
        s0 = (nextLength - curveOffset) / curveLength;
      }
      double t0 = current.inverseArcLength(s0);
      if (curveIndex >= out.size()) {
        break;
      }
      auto upSub = current.cut(0, t0);
      c.points.emplace_back(upSub.p1.x, upSub.p1.y, true);
      c.points.emplace_back(upSub.p2.x, upSub.p2.y);
      Vec<int16_t> upBase(upSub.p2.x, upSub.p2.y);
      auto upNorm = current.normal(t0, center);
      Vec<int16_t> upTip(upBase.x + (int16_t)round(upNorm.x * wallHeight), upBase.y + (int16_t)round(upNorm.y * wallHeight));
      c.points.emplace_back(upTip.x, upTip.y);
      auto nx = current.cut(t0, 1);
      double upSubLength = upSub.length();
      curveOffset += upSubLength;
      currentLength += upSubLength;
      current = nx;
      curveLength = nx.length();

      // down
      nextLength = sectionLength * i + sectionLength * 4 / 5;
      double s1 = (nextLength - curveOffset) / curveLength;
      while (s1 > 1) {
        curveIndex++;
        if (curveIndex >= out.size()) {
          break;
        }
        curveOffset += curveLength;
        currentLength += curveLength;
        current = out[curveIndex];
        curveLength = current.length();
        s1 = (nextLength - curveOffset) / curveLength;
      }
      double t1 = current.inverseArcLength(s1);
      if (curveIndex >= out.size()) {
        break;
      }
      auto downSub = current.cut(0, t1);
      Vec<int16_t> downBase = current.get(t1);
      auto downNorm = current.normal(t1, center);
      Vec<int16_t> downTip(downBase.x + (int16_t)round(downNorm.x * wallHeight), downBase.y + (int16_t)round(downNorm.y * wallHeight));
      c.points.emplace_back(downTip.x, downTip.y);
      c.points.emplace_back(downBase.x, downBase.y);

      auto next = current.cut(t1, 1);
      double downSubLength = downSub.length();
      curveOffset += downSubLength;
      currentLength += downSubLength;
      current = next;
    }

    c.points.emplace_back(current.p1.x, current.p1.y, true);
    c.points.emplace_back(current.p2.x, current.p2.y);
  }

public:
  static Status Create(Font &font, int16_t base, int16_t hfu, int16_t sb, int hhu, int chu, int16_t vfu, int vhu, int lineWidth) {
    using namespace std;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;
    using Contour = glyf::GlyphDataTable::Contour;

    int16_t scale = min(chu * hfu, vhu * vfu);
    int16_t vSpace = lineWidth;
    int16_t approachLength = lineWidth / 2;
    int16_t sideBearing = 2 * lineWidth;
    int16_t jointLength = max(1, lineWidth / 2);
    int16_t walledLineWidth = max(1, (int)round(lineWidth * 0.59375));
    int16_t wallHeight = max(1, (int)round(lineWidth * 0.75));

    Param p;
    p.scale = scale;
    p.lineWidth = lineWidth;
    p.approachLength = approachLength;
    p.sideBearing = sideBearing;
    p.jointLength = jointLength;
    p.walledLineWidth = walledLineWidth;
    p.wallHeight = wallHeight;

    int16_t top = base + vfu * vhu + vSpace + lineWidth;
    int16_t bottom = base - vSpace - lineWidth;
    int16_t height = top - bottom;
    int16_t width = height * 5 / 16;

    int16_t margin = lineWidth / 2;

    int16_t otop = top + vSpace + lineWidth;
    int16_t obottom = bottom - vSpace - lineWidth;
    int16_t oheight = otop - obottom;
    int16_t owidth = oheight * 5 / 16;

    // https://gyazo.com/ed6c12cb6881845d80380a52fa57597a
    int16_t vtop = base - margin + vhu * vfu;
    int16_t vbottom = base - margin;
    int16_t vwidth;
    {
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t vleft = center - hhu * hfu / 2 - 2 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 2 * lineWidth;
      vwidth = vright - vleft;
    }
    int16_t vheight = vwidth * 5 / 16;

    int16_t vowidth;
    {
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t vleft = center - hhu * hfu / 2 - 4 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 4 * lineWidth;
      vowidth = vright - vleft;
    }
    int16_t voheight = vowidth * 5 / 16;

    auto &outlines = font.outlines;
    if (!holds_alternative<Font::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<Font::TrueTypeOutlines>(outlines).glyf;

    if (auto st = Create_cb(font, p, width, height, bottom, "cbL", {"creR"}, {"cbR", "creL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    Contour cbT;
    {
      // cbT, creB
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t vleft = center - hhu * hfu / 2 - 2 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 2 * lineWidth;
      auto out = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vwidth, vheight - approachLength);
      auto in = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vwidth - 2 * lineWidth, vheight - approachLength - lineWidth);
      auto out1 = out[0].rotatedCW90().translated(center, 0);
      auto out2 = out[1].rotatedCW90().translated(center, 0);
      auto out3 = out[2].rotatedCW90().translated(center, 0);
      auto out4 = out[3].rotatedCW90().translated(center, 0);
      auto in1 = in[0].rotatedCW90().translated(center, 0);
      auto in2 = in[1].rotatedCW90().translated(center, 0);
      auto in3 = in[2].rotatedCW90().translated(center, 0);
      auto in4 = in[3].rotatedCW90().translated(center, 0);
      cbT.add(vright, vbottom - jointLength);
      cbT.add(vright, vbottom + approachLength);
      cbT.add(out1.p0);
      cbT.add(out1.p1, true);
      cbT.add(out1.p2);
      cbT.add(out2.p1, true);
      cbT.add(out2.p2);
      cbT.add(out3.p1, true);
      cbT.add(out3.p2);
      cbT.add(out4.p1, true);
      cbT.add(out4.p2);
      cbT.add(vleft, vbottom + approachLength);
      cbT.add(vleft, vbottom - jointLength);
      cbT.add(vleft + lineWidth, vbottom - jointLength);
      cbT.add(vleft + lineWidth, vbottom + approachLength);
      cbT.add(in4.p1, true);
      cbT.add(in4.p0);
      cbT.add(in3.p1, true);
      cbT.add(in3.p0);
      cbT.add(in2.p1, true);
      cbT.add(in2.p0);
      cbT.add(in1.p1, true);
      cbT.add(in1.p0);
      cbT.add(vright - lineWidth, vbottom - jointLength);
      auto gcbT = font.replaceSimpleGlyphByName("cbT", Class::Base, {cbT}, w, vleft, sideBearing + vheight, sideBearing);
      if (!gcbT) {
        return EGLYF_STATUS_PUSH(gcbT.status());
      }

      Contour creB;
      for (auto it = cbT.points.rbegin(); it != cbT.points.rend(); it++) {
        Point const &point = *it;
        // -(vbottom + vheight) + K => vbottom, K = 2 * vbottom + vheight
        creB.add(point.x, 2 * vbottom + vheight - point.y, point.control);
      }
      auto gcreB = font.replaceSimpleGlyphByName("creB", Class::Base, {creB}, w, vleft, sideBearing + vheight, -jointLength);
      if (!gcreB) {
        return EGLYF_STATUS_PUSH(gcreB.status());
      }
    }
    if (auto st = Create_crb(font, p, width, height, bottom, "crbL", {"ceR"}, {"crbR", "ceL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    Contour crbT;
    {
      // crbT, ceB
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t vleft = center - hhu * hfu / 2 - 2 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 2 * lineWidth;
      auto out = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vwidth, vheight - approachLength);
      auto in = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vwidth - 2 * lineWidth, vheight - approachLength - lineWidth);
      auto out1 = out[0].rotatedCW90().translated(center, 0);
      auto out2 = out[1].rotatedCW90().translated(center, 0);
      auto out3 = out[2].rotatedCW90().translated(center, 0);
      auto out4 = out[3].rotatedCW90().translated(center, 0);
      auto in1 = in[0].rotatedCW90().translated(center, 0);
      auto in2 = in[1].rotatedCW90().translated(center, 0);
      auto in3 = in[2].rotatedCW90().translated(center, 0);
      auto in4 = in[3].rotatedCW90().translated(center, 0);

      int16_t cutY = vbottom + vheight - lineWidth + lineWidth * 9 / 32;
      int16_t vrtop = cutY + lineWidth;
      crbT.add(vright, vrtop);
      crbT.add(vright, vrtop - lineWidth);
      array<double, 2> t;
      size_t num = out1.getTWhenY(cutY, t);
      if (num != 1) {
        num = out2.getTWhenY(cutY, t);
        if (num != 1) {
          return EGLYF_ERROR;
        }
        auto out2_ = out2.cut(0, t[0]);
        crbT.add(out2_.p2);
        crbT.add(out2_.p1, true);
        crbT.add(out2_.p0);

        crbT.add(out1.p1, true);
        crbT.add(out1.p0);
      } else {
        auto out1_ = out1.cut(0, t[0]);
        crbT.add(out1_.p2);
        crbT.add(out1_.p1, true);
        crbT.add(out1_.p0);
      }
      crbT.add(vright, vbottom + approachLength);
      crbT.add(vright, vbottom - jointLength);
      crbT.add(vright - lineWidth, vbottom - jointLength);
      crbT.add(vright - lineWidth, vbottom + approachLength);

      crbT.add(in1.p1, true);
      crbT.add(in1.p2);
      crbT.add(in2.p1, true);
      crbT.add(in2.p2);
      crbT.add(in3.p1, true);
      crbT.add(in3.p2);
      crbT.add(in4.p1, true);
      crbT.add(in4.p2);

      crbT.add(vleft + lineWidth, vbottom + approachLength);
      crbT.add(vleft + lineWidth, vbottom - jointLength);
      crbT.add(vleft, vbottom - jointLength);
      crbT.add(vleft, vbottom + approachLength);
      num = out4.getTWhenY(cutY, t);
      if (num != 1) {
        num = out3.getTWhenY(cutY, t);
        if (num != 1) {
          return EGLYF_ERROR;
        }
        auto out3_ = out3.cut(t[0], 1);
        crbT.add(out4.p1, true);
        crbT.add(out4.p0);
        crbT.add(out3_.p1, true);
        crbT.add(out3_.p0);
      } else {
        auto out4_ = out4.cut(t[0], 1);
        crbT.add(out4_.p1, true);
        crbT.add(out4_.p0);
      }
      crbT.add(vleft, vrtop - lineWidth);
      crbT.add(vleft, vrtop);

      auto gcrbT = font.replaceSimpleGlyphByName("crbT", Class::Base, {crbT}, w, vleft, sideBearing + vheight, sideBearing - lineWidth * 9 / 32);
      if (!gcrbT) {
        return EGLYF_STATUS_PUSH(gcrbT.status());
      }

      Contour ceB;
      for (auto it = crbT.points.rbegin(); it != crbT.points.rend(); it++) {
        Point const &point = *it;
        ceB.add(point.x, -point.y, point.control);
      }
      auto gceB = font.replaceSimpleGlyphByName("ceB", Class::Base, {ceB}, w, vleft, sideBearing + vheight, -jointLength);
      if (!gceB) {
        return EGLYF_STATUS_PUSH(gceB.status());
      }
    }
    if (auto st = Create_cb(font, p, owidth, oheight, obottom, "cobL", {"coreR"}, {"cobR", "coreL"}); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    {
      // cobT
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t voleft = center - hhu * hfu / 2 - 4 * lineWidth;
      int16_t voright = center + hhu * hfu / 2 + 4 * lineWidth;
      auto out = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vowidth, voheight - approachLength);
      auto in = QuadraticBezier<int16_t>::LeftCartouche(Vec<int16_t>(-vbottom - approachLength, 0), vowidth - 2 * lineWidth, voheight - approachLength - lineWidth);
      auto out1 = out[0].rotatedCW90().translated(center, 0);
      auto out2 = out[1].rotatedCW90().translated(center, 0);
      auto out3 = out[2].rotatedCW90().translated(center, 0);
      auto out4 = out[3].rotatedCW90().translated(center, 0);
      auto in1 = in[0].rotatedCW90().translated(center, 0);
      auto in2 = in[1].rotatedCW90().translated(center, 0);
      auto in3 = in[2].rotatedCW90().translated(center, 0);
      auto in4 = in[3].rotatedCW90().translated(center, 0);
      Contour cobT;
      cobT.add(voright, vbottom - jointLength);
      cobT.add(voright, vbottom + approachLength);
      cobT.add(out1.p0);
      cobT.add(out1.p1, true);
      cobT.add(out1.p2);
      cobT.add(out2.p1, true);
      cobT.add(out2.p2);
      cobT.add(out3.p1, true);
      cobT.add(out3.p2);
      cobT.add(out4.p1, true);
      cobT.add(out4.p2);
      cobT.add(voleft, vbottom + approachLength);
      cobT.add(voleft, vbottom - jointLength);
      cobT.add(voleft + lineWidth, vbottom - jointLength);
      cobT.add(voleft + lineWidth, vbottom + approachLength);
      cobT.add(in4.p1, true);
      cobT.add(in4.p0);
      cobT.add(in3.p1, true);
      cobT.add(in3.p0);
      cobT.add(in2.p1, true);
      cobT.add(in2.p0);
      cobT.add(in1.p1, true);
      cobT.add(in1.p0);
      cobT.add(voright - lineWidth, vbottom - jointLength);
      auto gcobT = font.replaceSimpleGlyphByName("cobT", Class::Base, {cobT}, w, voleft, sideBearing + voheight, sideBearing);
      if (!gcobT) {
        return EGLYF_STATUS_PUSH(gcobT.status());
      }
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
      auto hwtbL = font.replaceSimpleGlyphByName("hwtbL", Class::Base, {c}, advanceWidth, sideBearing, 0, 0);
      if (!hwtbL) {
        return EGLYF_STATUS_PUSH(hwtbL.status());
      }
      auto hwteR = font.replaceCompositeGlyphByName("hwteR", Class::Base, GlyphRecord::New(*hwtbL), advanceWidth, sideBearing, 0, 0);
      if (!hwteR) {
        return EGLYF_STATUS_PUSH(hwteR.status());
      }

      for (auto const &n : {"hwtbR", "hwteL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
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
      auto hwtobL = font.replaceSimpleGlyphByName("hwtobL", Class::Base, {c}, advanceWidth, sideBearing, 0, 0);
      if (!hwtobL) {
        return EGLYF_STATUS_PUSH(hwtobL.status());
      }
      auto hwtoeR = font.replaceCompositeGlyphByName("hwtoeR", Class::Base, GlyphRecord::New(*hwtobL), advanceWidth, sideBearing, 0, 0);
      if (!hwtoeR) {
        return EGLYF_STATUS_PUSH(hwtoeR.status());
      }

      for (auto const &n : {"hwtobR", "hwtoeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtobL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      vector<Contour> contours;
      int16_t advanceWidth;
      int16_t advanceHeight;
      CreateContour_hwttb(p, top, height, contours, advanceWidth, advanceHeight);
      auto hwttbL = font.replaceSimpleGlyphByName("hwttbL", Class::Base, contours, advanceWidth, p.sideBearing, 0, 0);
      if (!hwttbL) {
        return EGLYF_STATUS_PUSH(hwttbL.status());
      }

      for (auto const &n : {"hwttbR", "hwtteL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwttbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      auto hwtteR = font.replaceCompositeGlyphByName("hwtteR", Class::Base, GlyphRecord::New(*hwttbL), advanceWidth, p.sideBearing, 0, 0);
      if (!hwtteR) {
        return EGLYF_STATUS_PUSH(hwtteR.status());
      }
      for (auto const &n : {"hwtbbL", "hwtbeR"}) {
        auto gid = font.replaceCompositeGlyphByName("hwtbbL", Class::Base, GlyphRecord::New(*hwttbL, 0, advanceHeight, Vec<float>(1, -1)), advanceWidth, p.sideBearing, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      for (auto const &n : {"hwtbbR", "hwtbeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwttbL, advanceWidth, advanceHeight, Vec<float>(-1, -1)), advanceWidth, -jointLength, 0, 0);
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

      auto hwtdbL = font.replaceSimpleGlyphByName("hwtdbL", Class::Base, {c, c0, c1}, advanceWidth, -jointLength, 0, 0);
      if (!hwtdbL) {
        return EGLYF_STATUS_PUSH(hwtdbL.status());
      }
      auto hwtdeR = font.replaceCompositeGlyphByName("hwtdeR", Class::Base, GlyphRecord::New(*hwtdbL), advanceWidth, -jointLength, 0, 0);
      if (!hwtdeR) {
        return EGLYF_STATUS_PUSH(hwtdeR.status());
      }
      for (auto const &n : {"hwtdbR", "hwtdeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtdbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      vector<Contour> contours;
      int16_t advanceWidth;
      int16_t advanceHeight;
      CreateContour_hwttb(p, otop, oheight, contours, advanceWidth, advanceHeight);
      auto hwtotbL = font.replaceSimpleGlyphByName("hwtotbL", Class::Base, contours, advanceWidth, p.sideBearing, 0, 0);
      if (!hwtotbL) {
        return EGLYF_STATUS_PUSH(hwtotbL.status());
      }
      auto hwtoteR = font.replaceCompositeGlyphByName("hwtoteR", Class::Base, GlyphRecord::New(*hwtotbL), advanceWidth, sideBearing, 0, 0);
      if (!hwtoteR) {
        return EGLYF_STATUS_PUSH(hwtoteR.status());
      }
      for (auto const &n : {"hwtotbR", "hwtoteL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtotbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      for (auto const &n : {"hwtobbL", "hwtobeR"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtotbL, 0, advanceHeight, Vec<float>(1, -1)), advanceWidth, p.sideBearing, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      for (auto const &n : {"hwtobbR", "hwtobeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtotbL, advanceWidth, advanceHeight, Vec<float>(-1, -1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      vector<Contour> contours;
      int16_t advanceWidth;
      int16_t advanceHeight;
      CreateContour_hwttb(p, top, height, contours, advanceWidth, advanceHeight);

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

      auto hwtdtbL = font.replaceSimpleGlyphByName("hwtdtbL", Class::Base, contours, advanceWidth, -p.jointLength, 0, 0);
      if (!hwtdtbL) {
        return EGLYF_STATUS_PUSH(hwtdtbL.status());
      }
      for (auto const &n : {"hwtdbbL", "hwtdteR", "hwtdbeR"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtdtbL, 0, advanceHeight, Vec<float>(1, -1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      for (auto const &n : {"hwtdteL", "hwtdtbR"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtdtbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      for (auto const &n : {"hwtdbbR", "hwtdbeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*hwtdtbL, advanceWidth, advanceHeight, Vec<float>(-1, -1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      // O33aeL
      vector<Contour> contours;
      int16_t advanceWidth;
      CreateContour_O33aeL(p, top, height, contours, advanceWidth);
      auto O33aeL = font.replaceSimpleGlyphByName("O33aeL", Class::Base, contours, advanceWidth, -p.jointLength, 0, 0);
      if (!O33aeL) {
        return EGLYF_STATUS_PUSH(O33aeL.status());
      }
      auto O33aeR = font.replaceCompositeGlyphByName("O33aeR", Class::Base, GlyphRecord::New(*O33aeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
      if (!O33aeR) {
        return EGLYF_STATUS_PUSH(O33aeR.status());
      }
    }
    {
      // O33aoeL
      vector<Contour> contours;
      int16_t advanceWidth;
      CreateContour_O33aeL(p, otop, oheight, contours, advanceWidth);
      auto O33aoeL = font.replaceSimpleGlyphByName("O33aoeL", Class::Base, contours, advanceWidth, -p.jointLength, 0, 0);
      if (!O33aoeL) {
        return EGLYF_STATUS_PUSH(O33aoeL.status());
      }
      auto O33aoeR = font.replaceCompositeGlyphByName("O33aoeR", Class::Base, GlyphRecord::New(*O33aoeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
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

      auto O33adeL = font.replaceSimpleGlyphByName("O33adeL", Class::Base, contours, advanceWidth, -p.jointLength, 0, 0);
      if (!O33adeL) {
        return EGLYF_STATUS_PUSH(O33adeL.status());
      }
      auto O33adeR = font.replaceCompositeGlyphByName("O33adeR", Class::Base, GlyphRecord::New(*O33adeL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -jointLength, 0, 0);
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

      auto gid = font.replaceSimpleGlyphByName(name, Class::Base, {c0, c1}, w, -jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QC{}V", s);
      int16_t w = sb + s * hfu + sb;
      int16_t center = w / 2;
      int16_t v = vhu * vfu;
      int16_t vleft = center - hhu * hfu / 2 - 2 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 2 * lineWidth;
      Contour c0;
      c0.points.emplace_back(vleft, vbottom - jointLength);
      c0.points.emplace_back(vleft, vtop + jointLength);
      c0.points.emplace_back(vleft + lineWidth, vtop + jointLength);
      c0.points.emplace_back(vleft + lineWidth, vbottom - jointLength);
      Contour c1;
      c1.points.emplace_back(vright - lineWidth, vbottom - jointLength);
      c1.points.emplace_back(vright - lineWidth, vtop + jointLength);
      c1.points.emplace_back(vright, vtop + jointLength);
      c1.points.emplace_back(vright, vbottom - jointLength);
      auto gid = font.replaceSimpleGlyphByName(name, Class::Base, {c0, c1}, w, vleft, v, -jointLength);
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

      auto gid = font.replaceSimpleGlyphByName(name, gdef::GlyphDefinitionTable::Class::Base, {c0, c1}, w, -jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QO{}V", s);
      int16_t w = sb + s * hfu + sb;
      int16_t center = w / 2;
      int16_t v = vhu * vfu;
      int16_t vleft = center - hhu * hfu / 2 - 4 * lineWidth;
      int16_t vright = center + hhu * hfu / 2 + 4 * lineWidth;
      Contour c0;
      c0.points.emplace_back(vleft, vbottom - jointLength);
      c0.points.emplace_back(vleft, vtop + jointLength);
      c0.points.emplace_back(vleft + lineWidth, vtop + jointLength);
      c0.points.emplace_back(vleft + lineWidth, vbottom - jointLength);
      Contour c1;
      c1.points.emplace_back(vright - lineWidth, vbottom - jointLength);
      c1.points.emplace_back(vright - lineWidth, vtop + jointLength);
      c1.points.emplace_back(vright, vtop + jointLength);
      c1.points.emplace_back(vright, vbottom - jointLength);
      auto gid = font.replaceSimpleGlyphByName(name, Class::Base, {c0, c1}, w, vleft, v, -jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QD{}", s);
      int16_t w = sb + s * hfu + sb;
      auto qo = font.postGetGlyphID(format("QO{}", s));
      if (!qo) {
        return EGLYF_ERROR;
      }
      auto qc = font.postGetGlyphID(format("QC{}", s));
      if (!qc) {
        return EGLYF_ERROR;
      }
      vector<GlyphRecord> children;
      children.push_back(GlyphRecord::New(*qo));
      children.push_back(GlyphRecord::New(*qc));
      auto gid = font.replaceCompositeGlyphByName(name, Class::Base, children, w, -p.jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QD{}V", s);
      int16_t w = sb + s * hfu + sb;
      int16_t v = vhu * vfu;
      int16_t center = w / 2;
      int16_t vleft = center - hhu * hfu / 2 - 4 * lineWidth;
      auto qo = font.postGetGlyphID(format("QO{}V", s));
      if (!qo) {
        return EGLYF_ERROR;
      }
      auto qc = font.postGetGlyphID(format("QC{}V", s));
      if (!qc) {
        return EGLYF_ERROR;
      }
      vector<GlyphRecord> children;
      children.push_back(GlyphRecord::New(*qo));
      children.push_back(GlyphRecord::New(*qc));
      auto gid = font.replaceCompositeGlyphByName(name, Class::Base, children, w, vleft, v, -jointLength);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QW{}", s);
      // int16_t w = sb + s * hfu + sb;
      int16_t w = s * hfu;
      // int n = s;// (int)round(w / (float)(sb + hfu + sb));
      int16_t wallLength = hfu * 3 / 5; // w * 3 / (n * 5);
      int16_t hollowLength = (s * hfu - s * wallLength) / (s * 2);
      Contour up;
      up.points.emplace_back(-jointLength, top - lineWidth);
      up.points.emplace_back(-jointLength, top - lineWidth + walledLineWidth);
      for (int i = 0; i < s; i++) {
        int16_t x0 = (2 * hollowLength + wallLength) * i + hollowLength;
        up.points.emplace_back(x0, top - lineWidth + walledLineWidth);
        up.points.emplace_back(x0, top - lineWidth + walledLineWidth + wallHeight);
        up.points.emplace_back(x0 + wallLength, top - lineWidth + walledLineWidth + wallHeight);
        up.points.emplace_back(x0 + wallLength, top - lineWidth + walledLineWidth);
      }
      up.points.emplace_back(w + jointLength, top - lineWidth + walledLineWidth);
      up.points.emplace_back(w + jointLength, top - lineWidth);

      Contour down;
      down.points.emplace_back(-jointLength, bottom + lineWidth - walledLineWidth);
      down.points.emplace_back(-jointLength, bottom + lineWidth);
      down.points.emplace_back(w + jointLength, bottom + lineWidth);
      down.points.emplace_back(w + jointLength, bottom + lineWidth - walledLineWidth);
      for (int i = s - 1; i >= 0; i--) {
        int16_t x0 = (2 * hollowLength + wallLength) * i + hollowLength + wallLength;
        down.points.emplace_back(x0, bottom + lineWidth - walledLineWidth);
        down.points.emplace_back(x0, bottom + lineWidth - walledLineWidth - wallHeight);
        down.points.emplace_back(x0 - wallLength, bottom + lineWidth - walledLineWidth - wallHeight);
        down.points.emplace_back(x0 - wallLength, bottom + lineWidth - walledLineWidth);
      }

      auto gid = font.replaceSimpleGlyphByName(name, Class::Base, {up, down}, w, -jointLength, 0, 0);
      if (!gid) {
        return EGLYF_STATUS_PUSH(gid.status());
      }
    }
    for (int s = 1; s <= hhu; s++) {
      auto name = format("QF{}", s);
      int16_t w = sb + s * hfu + sb;
      auto qo = font.postGetGlyphID(format("QO{}", s));
      if (!qo) {
        return EGLYF_ERROR;
      }
      auto qw = font.postGetGlyphID(format("QW{}", s));
      if (!qw) {
        return EGLYF_ERROR;
      }
      vector<GlyphRecord> children;
      children.push_back(GlyphRecord::New(*qo));
      children.push_back(GlyphRecord::New(*qw));
      auto gid = font.replaceCompositeGlyphByName(name, Class::Base, children, w, -p.jointLength, 0, 0);
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

      auto cdbL = font.replaceSimpleGlyphByName("cdbL", Class::Base, {c0, c1, c}, w, -jointLength, 0, 0);
      if (!cdbL) {
        return EGLYF_STATUS_PUSH(cdbL.status());
      }
      for (auto const &n : {"cdbR", "cdreL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*cdbL, w, 0, Vec<float>(-1, 1)), w, -jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      auto cdreR = font.replaceCompositeGlyphByName("cdreR", Class::Base, GlyphRecord::New(*cdbL), w, -jointLength, 0, 0);
      if (!cdreR) {
        return EGLYF_STATUS_PUSH(cdreR.status());
      }
    }
    {
      // cdbT
      int16_t w = sb + hhu * hfu + sb;
      int16_t center = w / 2;
      int16_t voleft = center - hhu * hfu / 2 - 4 * lineWidth;
      int16_t voright = center + hhu * hfu / 2 + 4 * lineWidth;
      Contour c0;
      c0.add(voleft, vbottom - jointLength);
      c0.add(voleft, vbottom + voheight + sideBearing + jointLength);
      c0.add(voleft + lineWidth, vbottom + voheight + sideBearing + jointLength);
      c0.add(voleft + lineWidth, vbottom - jointLength);
      Contour c1;
      c1.add(voright - lineWidth, vbottom - jointLength);
      c1.add(voright - lineWidth, vbottom + voheight + sideBearing + jointLength);
      c1.add(voright, vbottom + voheight + sideBearing + jointLength);
      c1.add(voright, vbottom - jointLength);
      auto cdbT = font.replaceSimpleGlyphByName("cdbT", Class::Base, {c0, c1, cbT}, w, voleft, voheight + sideBearing, -jointLength);
      if (!cdbT) {
        return EGLYF_STATUS_PUSH(cdbT.status());
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

      auto cdrbL = font.replaceSimpleGlyphByName("cdrbL", Class::Base, {c0, c1, c}, w, -jointLength, 0, 0);
      if (!cdrbL) {
        return EGLYF_STATUS_PUSH(cdrbL.status());
      }

      for (auto const &n : {"cdrbR", "cdeL"}) {
        auto gid = font.replaceCompositeGlyphByName(n, Class::Base, GlyphRecord::New(*cdrbL, w, 0, Vec<float>(-1, 1)), w, -jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }

      auto cdeR = font.replaceCompositeGlyphByName("cdeR", Class::Base, GlyphRecord::New(*cdrbL), w, -jointLength, 0, 0);
      if (!cdeR) {
        return EGLYF_STATUS_PUSH(cdeR.status());
      }
    }
    {
      // cwbL
      Contour c;
      CreateContour_cwbL(p, hfu, width, bottom, height, c);

      int16_t advanceWidth = sideBearing + width + approachLength;
      auto cwbL = font.replaceSimpleGlyphByName("cwbL", Class::Base, {c}, advanceWidth, sideBearing + lineWidth - walledLineWidth - wallHeight, 0, 0);
      if (!cwbL) {
        return EGLYF_STATUS_PUSH(cwbL.status());
      }
      // copy
      for (string const &c : {"cweR"}) {
        auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*cwbL), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // mirror
      for (string const &m : {"cwbR", "cweL"}) {
        auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*cwbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // cfbL
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
      auto cfbL = font.replaceSimpleGlyphByName("cfbL", Class::Base, {c, c0, c1}, advanceWidth, -jointLength, 0, 0);
      if (!cfbL) {
        return EGLYF_STATUS_PUSH(cfbL.status());
      }
      // copy
      for (string const &c : {"cfeR"}) {
        auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*cfbL), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // mirror
      for (string const &m : {"cfbR", "cfeL"}) {
        auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*cfbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    {
      // hwbL
      double a = hfu / 5.0;
      double h = height - 2 * lineWidth + 2 * walledLineWidth + 2 * wallHeight;
      int hDiv = (int)round((h - 2 * a) / (5 * a));
      int16_t ha = (int16_t)round(h / (5 * hDiv - 2));

      int16_t wallLength = hfu * 3 / 5;
      int16_t hollowLength = (hfu - wallLength) / 2;

      vector<int16_t> wallX;
      int x = sideBearing + width - hollowLength;
      while (x > sideBearing + hfu) {
        wallX.push_back(x);
        x -= hfu;
      }

      Contour c;
      Contour::Drawer d(&c, sideBearing + lineWidth - walledLineWidth - wallHeight, bottom + lineWidth - walledLineWidth - wallHeight);
      for (int i = 1; i < hDiv; i++) {
        d.north(3 * ha);
        d.east(wallHeight);
        d.north(2 * ha);
        d.west(wallHeight);
      }
      d.toY(top - lineWidth + walledLineWidth + wallHeight);
      d.toX(wallX.back());
      for (auto it = wallX.rbegin() + 1; it != wallX.rend(); it++) {
        int16_t wx = *it;
        d.south(wallHeight);
        d.east(2 * hollowLength);
        d.north(wallHeight);
        d.toX(wx);
      }
      d.south(wallHeight);
      d.toX(sideBearing + width + jointLength);
      d.south(walledLineWidth);
      d.toX(sideBearing + lineWidth);
      d.toY(bottom + lineWidth);
      d.toX(sideBearing + width + jointLength);
      d.south(walledLineWidth);
      d.toX(wallX.front());
      for (auto it = wallX.begin(); it + 1 != wallX.end(); it++) {
        int16_t wx = *it;
        d.south(wallHeight);
        d.west(wallLength);
        d.north(wallHeight);
        d.west(2 * hollowLength);
      }
      d.south(wallHeight);
      int16_t advanceWidth = sideBearing + width;
      int16_t sideB = sideBearing + lineWidth - walledLineWidth - wallHeight;
      auto hwbL = font.replaceSimpleGlyphByName("hwbL", Class::Base, {c}, advanceWidth, sideB, 0, 0);
      if (!hwbL) {
        return EGLYF_STATUS_PUSH(hwbL.status());
      }
      // copy
      for (string const &c : {"hweR"}) {
        auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*hwbL), advanceWidth, sideB, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // mirror
      for (string const &m : {"hwbR", "hweL"}) {
        auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*hwbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // hfbL
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
      auto cfbL = font.replaceSimpleGlyphByName("hfbL", Class::Base, {c, c0, c1}, advanceWidth, -jointLength, 0, 0);
      if (!cfbL) {
        return EGLYF_STATUS_PUSH(cfbL.status());
      }
      // copy
      for (string const &c : {"hfeR"}) {
        auto gid = font.replaceCompositeGlyphByName(c, Class::Base, GlyphRecord::New(*cfbL), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
      // mirror
      for (string const &m : {"hfbR", "hfeL"}) {
        auto gid = font.replaceCompositeGlyphByName(m, Class::Base, GlyphRecord::New(*cfbL, advanceWidth, 0, Vec<float>(-1, 1)), advanceWidth, -p.jointLength, 0, 0);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
