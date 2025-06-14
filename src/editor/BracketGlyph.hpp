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
    return Status::Ok();
  }

private:
  Status createTcab() {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;
    using GlyphRecord = glyf::GlyphDataTable::CompositeGlyph::GlyphRecord;

    Contour t;
    t.add(0, 626);
    t.add(0, 665);
    t.add(70, 1576);
    t.add(120, 1576);
    t.add(50, 646);
    t.add(120, -284);
    t.add(70, -284);
    auto bounds = t.boundingBox();
    for (int i = 1; i <= vhu; i++) {
      double scale = i * vfu / (double)bounds.height();
      double tx = 0 - bounds.xMin * scale;
      double ty = base - bounds.yMin * scale;
      Transform<double> mtx(scale, 0, 0, scale, tx, ty);
      auto c = t.transformed(mtx);
      auto name = format("tcab0_{}", i);
      auto tcab0 = font->replaceSimpleGlyphByName(name, Class::Mark, {c}, 0, 0, 0, 0);
      if (!tcab0) {
        return EGLYF_STATUS_PUSH(tcab0.status());
      }

      for (int j = 1; j <= 2; j++) {
        auto n = format("tcab{}_{}", j, i);
        auto cp = font->replaceCompositeGlyphByName(n, Class::Mark, GlyphRecord::New(*tcab0), 0, 0, 0, 0);
        if (!cp) {
          return EGLYF_STATUS_PUSH(cp.status());
        }
      }
      for (int j = 0; j <= 2; j++) {
        auto n = format("tcae{}_{}", j, i);
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
