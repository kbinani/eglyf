#pragma once

namespace eglyf {

class BracketGlyph {
  BracketGlyph(Font &font, int16_t base, int hhu, int16_t vfu, int vhu) : font(&font), base(base), hhu(hhu), vfu(vfu), vhu(vhu) {}

public:
  static Status Create(Font &font, int16_t base, int hhu, int16_t vfu, int vhu) {
    BracketGlyph bg(font, base, hhu, vfu, vhu);
    if (auto st = bg.createTcab(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = bg.createTcbb(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = bg.createTcpb(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

private:
  Status createTcab() const {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    Contour t;
    t.add(0, 626);
    t.add(0, 665);
    t.add(70, 1576);
    t.add(120, 1576);
    t.add(50, 646);
    t.add(120, -284);
    t.add(70, -284);
    return EGLYF_STATUS_PUSH(createFromTemplate(t, "tcab", "tcae"));
  }

  Status createTcbb() const {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    Contour t;
    t.add(0, -284);
    t.add(0, 1576);
    t.add(110, 1576);
    t.add(110, 1526);
    t.add(50, 1526);
    t.add(50, -234);
    t.add(110, -234);
    t.add(110, -284);
    return EGLYF_STATUS_PUSH(createFromTemplate(t, "tcbb", "tcbe"));
  }

  Status createTcpb() const {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    Contour t;
    t.add(112, -284);
    t.add(72, -265, true);
    t.add(27, -164, true);
    t.add(5, -6, true);
    t.add(0, 206, true);
    t.add(0, 336);
    t.add(0, 956);
    t.add(0, 1086, true);
    t.add(5, 1298, true);
    t.add(27, 1456, true);
    t.add(72, 1557, true);
    t.add(112, 1576);
    t.add(140, 1545);
    t.add(103, 1523, true);
    t.add(66, 1414, true);
    t.add(51, 1259, true);
    t.add(50, 1065, true);
    t.add(50, 957);
    t.add(50, 336);
    t.add(50, 228, true);
    t.add(51, 34, true);
    t.add(66, -122, true);
    t.add(103, -231, true);
    t.add(140, -253);
    return EGLYF_STATUS_PUSH(createFromTemplate(t, "tcpb", "tcpe"));
  }

  Status createFromTemplate(glyf::GlyphDataTable::Contour const &t, std::string const &prefix, std::string const &prefixMirrored) const {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    auto bounds = t.boundingBox();
    for (int i = 1; i <= vhu; i++) {
      double scale = i * vfu / (double)bounds.height();
      double tx = 0 - bounds.xMin * scale;
      double ty = base - bounds.yMin * scale;
      Transform<double> mtx(scale, 0, 0, scale, tx, ty);
      auto c = t.transformed(mtx);
      auto name = format("{}0_{}", prefix, i);
      auto tcab0 = font->replaceSimpleGlyphByName(name, Class::Mark, {c}, 0, 0, 0, 0);
      if (!tcab0) {
        return EGLYF_STATUS_PUSH(tcab0.status());
      }

      for (int j = 1; j <= 2; j++) {
        auto n = format("{}{}_{}", prefix, j, i);
        auto cp = font->replaceCompositeGlyphByName(n, Class::Mark, GlyphRecord::New(*tcab0), 0, 0, 0, 0);
        if (!cp) {
          return EGLYF_STATUS_PUSH(cp.status());
        }
      }
      for (int j = 0; j <= 2; j++) {
        auto n = format("{}{}_{}", prefixMirrored, j, i);
        auto cp = font->replaceCompositeGlyphByName(n, Class::Mark, GlyphRecord::New(*tcab0, 0, 0, Vec<float>(-1, 1)), 0, 0, 0, 0);
        if (!cp) {
          return EGLYF_STATUS_PUSH(cp.status());
        }
      }
    }
    return Status::Ok();
  }

private:
  Font *font;
  int16_t base;
  int hhu;
  int16_t vfu;
  int vhu;
};

} // namespace eglyf
