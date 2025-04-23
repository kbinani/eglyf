#pragma once

namespace eglyf {

class PlaceholderGlyph {
  PlaceholderGlyph() = delete;

  struct Param {
    int n; // number of dashes per the side
    int d; // length of the gap between dashes
    int e; // length of a dash
    int s; // (length of the side) / 2
  };

  static Param Parameter(int n, int segment, int16_t fu) {
    using namespace std;
    Param r;
    r.n = max(2, (int)round(n * fu / (float)segment));
    r.d = (n * fu) / (5 * r.n / 2 + 1);
    r.e = r.d * 3 / 2;
    int const _s = r.e * r.n + r.d * (r.n - 1);
    r.s = _s / 2;
    return r;
  }

  struct CommonParam {
    int t;       // line width
    int segment; // base length of (1 dash + 1 gap)
  };

  static CommonParam CommonParameter(int16_t chu, int16_t hfu, int16_t vhu, int16_t vfu) {
    using namespace std;
    CommonParam p;
    int scale = min(chu * hfu, vhu * vfu);
    p.t = max(1, scale / 96);
    p.segment = min(scale / chu, scale / vhu);
    return p;
  }

public:
  static Rect<int16_t> Bounds(int h, int v, int16_t base, int16_t hfu, int chu, int16_t vfu, int vhu) {
    using namespace std;
    CommonParam c = CommonParameter(chu, hfu, vhu, vfu);
    Param x = Parameter(h, c.segment, hfu);
    Param y = Parameter(v, c.segment, vfu);
    return Rect<int16_t>(-x.s - c.t, base - c.t, x.s + c.t, base + 2 * y.s + c.t);
  }

  static Status Create(FontFile &font, int16_t base, int16_t hfu, int16_t sb, int chu, int16_t vfu, int vhu) {
    using namespace std;
    using Contour = glyf::GlyphDataTable::Contour;
    using Point = glyf::GlyphDataTable::Point;
    using SimpleGlyph = glyf::GlyphDataTable::SimpleGlyph;
    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;
    CommonParam p = CommonParameter(chu, hfu, vhu, vfu);
    int const segment = p.segment;
    int const t = p.t;

    for (int xLevel = 1; xLevel <= chu; xLevel++) {
      Param x = Parameter(xLevel, segment, hfu);
      int const w = x.s;
      for (int yLevel = 1; yLevel <= vhu; yLevel++) {
        Param y = Parameter(yLevel, segment, vfu);
        int const h = y.s;
        auto name = format("GB1_{}{}", xLevel, yLevel);
        vector<Contour> half;
        for (int i = x.n / 2; i < x.n - 1; i++) {
          Contour c;
          c.points.emplace_back(-w + (x.e + x.d) * i, h + t);
          c.points.emplace_back(-w + (x.e + x.d) * i + x.e, h + t);
          c.points.emplace_back(-w + (x.e + x.d) * i + x.e, h - t);
          c.points.emplace_back(-w + (x.e + x.d) * i, h - t);
          half.push_back(c);
        }
        {
          // top right corner
          Contour c;
          c.points.emplace_back(w - x.e, h + t);
          c.points.emplace_back(w + t, h + t);
          c.points.emplace_back(w + t, h - y.e);
          c.points.emplace_back(w - t, h - y.e);
          c.points.emplace_back(w - t, h - t);
          c.points.emplace_back(w - x.e, h - t);
          half.push_back(c);
        }
        for (int i = 1; i < y.n - 1; i++) {
          Contour c;
          c.points.emplace_back(w + t, h - (y.e + y.d) * i);
          c.points.emplace_back(w + t, h - (y.e + y.d) * i - y.e);
          c.points.emplace_back(w - t, h - (y.e + y.d) * i - y.e);
          c.points.emplace_back(w - t, h - (y.e + y.d) * i);
          half.push_back(c);
        }
        {
          // bottom right corner
          Contour c;
          c.points.emplace_back(w + t, -h + y.e);
          c.points.emplace_back(w + t, -h - t);
          c.points.emplace_back(w - x.e, -h - t);
          c.points.emplace_back(w - x.e, -h + t);
          c.points.emplace_back(w - t, -h + t);
          c.points.emplace_back(w - t, -h + y.e);
          half.push_back(c);
        }
        for (int i = x.n - 2; i >= (x.n % 2 == 0 ? x.n / 2 : x.n / 2 + 1); i--) {
          Contour c;
          c.points.emplace_back(-w + (x.e + x.d) * i, -h + t);
          c.points.emplace_back(-w + (x.e + x.d) * i + x.e, -h + t);
          c.points.emplace_back(-w + (x.e + x.d) * i + x.e, -h - t);
          c.points.emplace_back(-w + (x.e + x.d) * i, -h - t);
          half.push_back(c);
        }
        int x0 = 0;
        int y0 = base + h;
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
        auto gid = font.addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Mark, contours, 0, -w - t);
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
