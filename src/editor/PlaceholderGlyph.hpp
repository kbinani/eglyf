#pragma once

namespace eglyf {

class PlaceholderGlyph {
  PlaceholderGlyph() = delete;

public:
  static Status Create(FontFile &font, int16_t base, int16_t hfu, int16_t sb, int chu, int16_t vfu, int16_t tb, int vhu) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Point = glyf::GlyphDataTable::Point;
    using SimpleGlyph = glyf::GlyphDataTable::SimpleGlyph;
    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;
    int const scale = min(chu * hfu, vhu * vfu);
    int const t = max(1, scale / 64);
    for (int x = 1; x <= chu; x++) {
      int xn = max(2, x);
      int xd = (x * hfu) / (5 * xn / 2 + 1);
      int xe = xd * 3 / 2;
      int width = xe * xn + xd * (xn - 1);
      int const w = width / 2;
      for (int y = 1; y <= vhu; y++) {
        int yn = max(2, y);
        int yd = (y * vfu) / (5 * yn / 2 + 1);
        int ye = yd * 3 / 2;
        int height = ye * yn + yd * (yn - 1);
        int const h = height / 2;
        auto name = format("GB1_{}{}", x, y);
        vector<Contour> half;
        for (int i = xn / 2; i < xn - 1; i++) {
          Contour c;
          c.points.emplace_back(-w + (xe + xd) * i, h + t);
          c.points.emplace_back(-w + (xe + xd) * i + xe, h + t);
          c.points.emplace_back(-w + (xe + xd) * i + xe, h - t);
          c.points.emplace_back(-w + (xe + xd) * i, h - t);
          half.push_back(c);
        }
        {
          // top right corner
          Contour c;
          c.points.emplace_back(w - xe, h + t);
          c.points.emplace_back(w + t, h + t);
          c.points.emplace_back(w + t, h - ye);
          c.points.emplace_back(w - t, h - ye);
          c.points.emplace_back(w - t, h - t);
          c.points.emplace_back(w - xe, h - t);
          half.push_back(c);
        }
        for (int i = 1; i < yn - 1; i++) {
          Contour c;
          c.points.emplace_back(w + t, h - (ye + yd) * i);
          c.points.emplace_back(w + t, h - (ye + yd) * i - ye);
          c.points.emplace_back(w - t, h - (ye + yd) * i - ye);
          c.points.emplace_back(w - t, h - (ye + yd) * i);
          half.push_back(c);
        }
        {
          // bottom right corner
          Contour c;
          c.points.emplace_back(w + t, -h + ye);
          c.points.emplace_back(w + t, -h - t);
          c.points.emplace_back(w - xe, -h - t);
          c.points.emplace_back(w - xe, -h + t);
          c.points.emplace_back(w - t, -h + t);
          c.points.emplace_back(w - t, -h + ye);
          half.push_back(c);
        }
        for (int i = xn - 2; i >= (xn % 2 == 0 ? xn / 2 : xn / 2 + 1); i--) {
          Contour c;
          c.points.emplace_back(-w + (xe + xd) * i, -h + t);
          c.points.emplace_back(-w + (xe + xd) * i + xe, -h + t);
          c.points.emplace_back(-w + (xe + xd) * i + xe, -h - t);
          c.points.emplace_back(-w + (xe + xd) * i, -h - t);
          half.push_back(c);
        }
        int x0 = 0;
        int y0 = base + h + t;
        vector<Contour> contours;
        for (auto const &c : half) {
          Contour cp;
          for (auto const &p : c.points) {
            cp.points.emplace_back(p.x + x0, p.y + y0);
          }
          contours.push_back(cp);
        }
        for (auto const &c : half) {
          Contour cp;
          for (auto const &p : c.points) {
            cp.points.emplace_back(-p.x + x0, -p.y + y0);
          }
          contours.push_back(cp);
        }
        auto gid = font.addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Mark, contours, 0, -w);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
