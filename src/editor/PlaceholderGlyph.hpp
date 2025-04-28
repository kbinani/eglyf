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
    int t;       // (line width) / 2
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
    CommonParam c = CommonParameter(chu, hfu, vhu, vfu);
    Param x = Parameter(h, c.segment, hfu);
    Param y = Parameter(v, c.segment, vfu);
    return Rect<int16_t>(-x.s - c.t, base - c.t, x.s + c.t, base + 2 * y.s + c.t);
  }

  static Status Create(FontFile &font, int16_t base, int16_t hfu, int16_t sb, int chu, int16_t vfu, int vhu) {
    using namespace std;
    using SimpleGlyph = glyf::GlyphDataTable::SimpleGlyph;

    auto &outlines = font.outlines;
    if (!holds_alternative<FontFile::TrueTypeOutlines>(outlines)) {
      return EGLYF_ERROR;
    }
    auto &glyf = get<FontFile::TrueTypeOutlines>(outlines).glyf;
    CommonParam p = CommonParameter(chu, hfu, vhu, vfu);

    for (int xLevel = 1; xLevel <= chu; xLevel++) {
      Param x = Parameter(xLevel, p.segment, hfu);
      for (int yLevel = 1; yLevel <= vhu; yLevel++) {
        Param y = Parameter(yLevel, p.segment, vfu);
        auto name = format("GB1_{}{}", xLevel, yLevel);
        vector<Contour> half;
        for (int i = x.n / 2; i < x.n - 1; i++) {
          Contour c;
          c.points.emplace_back(-x.s + (x.e + x.d) * i, y.s + p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i + x.e, y.s + p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i + x.e, y.s - p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i, y.s - p.t);
          half.push_back(c);
        }
        {
          // top right corner
          Contour c;
          c.points.emplace_back(x.s - x.e, y.s + p.t);
          c.points.emplace_back(x.s + p.t, y.s + p.t);
          c.points.emplace_back(x.s + p.t, y.s - y.e);
          c.points.emplace_back(x.s - p.t, y.s - y.e);
          c.points.emplace_back(x.s - p.t, y.s - p.t);
          c.points.emplace_back(x.s - x.e, y.s - p.t);
          half.push_back(c);
        }
        for (int i = 1; i < y.n - 1; i++) {
          Contour c;
          c.points.emplace_back(x.s + p.t, y.s - (y.e + y.d) * i);
          c.points.emplace_back(x.s + p.t, y.s - (y.e + y.d) * i - y.e);
          c.points.emplace_back(x.s - p.t, y.s - (y.e + y.d) * i - y.e);
          c.points.emplace_back(x.s - p.t, y.s - (y.e + y.d) * i);
          half.push_back(c);
        }
        {
          // bottom right corner
          Contour c;
          c.points.emplace_back(x.s + p.t, -y.s + y.e);
          c.points.emplace_back(x.s + p.t, -y.s - p.t);
          c.points.emplace_back(x.s - x.e, -y.s - p.t);
          c.points.emplace_back(x.s - x.e, -y.s + p.t);
          c.points.emplace_back(x.s - p.t, -y.s + p.t);
          c.points.emplace_back(x.s - p.t, -y.s + y.e);
          half.push_back(c);
        }
        for (int i = x.n - 2; i >= (x.n + 1) / 2; i--) {
          Contour c;
          c.points.emplace_back(-x.s + (x.e + x.d) * i, -y.s + p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i + x.e, -y.s + p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i + x.e, -y.s - p.t);
          c.points.emplace_back(-x.s + (x.e + x.d) * i, -y.s - p.t);
          half.push_back(c);
        }
        int x0 = 0;
        int y0 = base + y.s;
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
        auto reticle = p.segment * 3 / (5 * 2);
        int xInnerGap = 2 * x.s - 2 * p.t;
        int yInnerGap = 2 * y.s - 2 * p.t;
        if (reticle * 2 < xInnerGap - 2 * p.t && reticle * 2 < yInnerGap - 2 * p.t) {
          Contour c;
          c.points.emplace_back(x0 - reticle, y0 + p.t);
          c.points.emplace_back(x0 - p.t, y0 + p.t);
          c.points.emplace_back(x0 - p.t, y0 + reticle);
          c.points.emplace_back(x0 + p.t, y0 + reticle);
          c.points.emplace_back(x0 + p.t, y0 + p.t);
          c.points.emplace_back(x0 + reticle, y0 + p.t);
          c.points.emplace_back(x0 + reticle, y0 - p.t);
          c.points.emplace_back(x0 + p.t, y0 - p.t);
          c.points.emplace_back(x0 + p.t, y0 - reticle);
          c.points.emplace_back(x0 - p.t, y0 - reticle);
          c.points.emplace_back(x0 - p.t, y0 - p.t);
          c.points.emplace_back(x0 - reticle, y0 - p.t);
          contours.push_back(c);
        }
        auto gid = font.addSimpleGlyph(name, gdef::GlyphDefinitionTable::Class::Mark, contours, 0, -(x.s + p.t));
        if (!gid) {
          return EGLYF_STATUS_PUSH(gid.status());
        }
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
