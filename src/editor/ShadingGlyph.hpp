#pragma once

namespace eglyf {

class ShadingGlyph {
  struct Polyline {
    Polyline(std::initializer_list<Line<int16_t>> init) : lines(init.begin(), init.end()) {}

    std::vector<Line<int16_t>> lines;
  };

  static Polyline MakeRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    return Polyline({Line(x0, y0, x0, y1), Line(x0, y1, x1, y1), Line(x1, y1, x1, y0), Line(x1, y0, x0, y0)});
  }

  static Status ForEachPolyline(int16_t x, int16_t y, int16_t w, int16_t h, std::function<Status(std::set<int> const &member, Polyline const &)> cb) {
    // https://gyazo.com/d4b0feac3ef961076114f594eedf3219
    int16_t x0 = x;
    int16_t x1 = x + w / 2;
    int16_t x2 = x + w;
    int16_t y0 = y + h;
    int16_t y1 = y + h / 2;
    int16_t y2 = y;
    if (auto st = cb({1}, MakeRect(x0, y0, x1, y1)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({2}, MakeRect(x0, y1, x1, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({3}, MakeRect(x1, y0, x2, y1)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({4}, MakeRect(x1, y1, x2, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 2}, MakeRect(x0, y0, x1, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 3}, MakeRect(x0, y0, x2, y1)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 4}, Polyline({Line(x0, y0, x0, y1),
                                       Line(x0, y1, x1, y1),
                                       Line(x1, y1, x1, y2),
                                       Line(x1, y2, x2, y2),
                                       Line(x2, y2, x2, y1),
                                       Line(x2, y1, x1, y1),
                                       Line(x1, y1, x1, y0),
                                       Line(x1, y0, x0, y0)}));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({2, 3}, Polyline({
                                 Line(x1, y1, x0, y1),
                                 Line(x0, y1, x0, y2),
                                 Line(x0, y2, x1, y2),
                                 Line(x1, y2, x1, y1),
                                 Line(x1, y1, x2, y1),
                                 Line(x2, y1, x2, y0),
                                 Line(x2, y0, x1, y0),
                                 Line(x1, y0, x1, y1),
                             }));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({2, 4}, MakeRect(x0, y1, x2, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({3, 4}, MakeRect(x1, y0, x2, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 2, 3}, Polyline({
                                    Line(x2, y0, x0, y0),
                                    Line(x0, y0, x0, y2),
                                    Line(x0, y2, x1, y2),
                                    Line(x1, y2, x1, y1),
                                    Line(x1, y1, x2, y1),
                                    Line(x2, y1, x2, y0),
                                }));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 2, 4}, Polyline({
                                    Line(x1, y0, x0, y0),
                                    Line(x0, y0, x0, y2),
                                    Line(x0, y2, x2, y2),
                                    Line(x2, y2, x2, y1),
                                    Line(x2, y1, x1, y1),
                                    Line(x1, y1, x1, y0),
                                }));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 3, 4}, Polyline({
                                    Line(x2, y0, x0, y0),
                                    Line(x0, y0, x0, y1),
                                    Line(x0, y1, x1, y1),
                                    Line(x1, y1, x1, y2),
                                    Line(x1, y2, x2, y2),
                                    Line(x2, y2, x2, y0),
                                }));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({2, 3, 4}, Polyline({
                                    Line(x2, y0, x1, y0),
                                    Line(x1, y0, x1, y1),
                                    Line(x1, y1, x0, y1),
                                    Line(x0, y1, x0, y2),
                                    Line(x0, y2, x2, y2),
                                    Line(x2, y2, x2, y0),
                                }));
        !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    if (auto st = cb({1, 2, 3, 4}, MakeRect(x0, y0, x2, y2)); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return Status::Ok();
  }

  static void RepeatDiagonalLines(int ih, int chu, int vhu, int16_t hfu, int16_t vfu, int16_t base, int16_t margin,
                                  std::function<void(Line<double> const &a, Line<double> const &b)> cb) {
    using namespace std;
    int scale = min(hfu * chu, vfu * vhu);
    int lineWidth = max(1, (int)round(scale * 0.006));
    int tdistance = (int)round(scale * 0.067);
    int div = (int)round(hfu / (double)tdistance);
    int distance = hfu / (double)div;
    double a = vfu / (double)hfu;
    int k = (int)round(lineWidth * sqrt(1 / (a * a) + 1));
    int dy = vfu + vhu * vfu + vfu;
    int dx = (int)round(dy / a);
    int x0 = -ih * hfu / 2;
    int y0 = base - margin;
    for (int i = 1;; i++) {
      int x00 = x0 - i * hfu;
      if (x00 < x0 - (vhu + 1) * vfu / a) {
        break;
      }
      for (int j = 0; j < div; j++) {
        int x = x00 + j * distance;
        Line<double> la(x - dx + 1, y0 - dy, x + dx + 1, y0 + dy);
        Line<double> lb(x + dx + k + 1, y0 + dy, x - dx + k + 1, y0 - dy);
        cb(la, lb);
      }
    }
    for (int i = 0;; i++) {
      int x00 = x0 + i * hfu;
      if (x0 + ih * hfu < x00) {
        break;
      }
      for (int j = 0; j < div; j++) {
        int x = x00 + j * distance;
        Line<double> la(x - dx + 1, y0 - dy, x + dx + 1, y0 + dy);
        Line<double> lb(x + dx + k + 1, y0 + dy, x - dx + k + 1, y0 - dy);
        cb(la, lb);
      }
    }
  }

public:
  static Status Create(FontFile &font, int hhu, int chu, int vhu, int16_t hfu, int16_t vfu, int16_t base, int16_t lineWidth) {
    using namespace std;
    using L = Line<double>;
    using Contour = glyf::GlyphDataTable::Contour;
    using Class = gdef::GlyphDefinitionTable::Class;

    int16_t margin = lineWidth / 2;
    int16_t const y0 = base - margin;
    for (int ih = 1; ih <= hhu; ih++) {
      int16_t w = hfu * ih;
      for (int iv = 1; iv <= vhu; iv++) {
        int16_t h = vfu * iv;
        int16_t x0 = -w / 2;
        auto st = ForEachPolyline(x0, y0, w, h, [&](set<int> const &member, Polyline const &pl) -> Status {
          string name = "dq";
          for (int m : member) {
            name += format("{}", m);
          }
          name += format("_{}{}", ih, iv);
          // https://gyazo.com/5e7a1afb76a703797ec8131a542f518f
          struct Cross {
            int member;
            double t;
            bool used = false;
          };
          vector<Contour> contours;
          RepeatDiagonalLines(ih, chu, vhu, hfu, vfu, base, margin, [&](Line<double> const &a, Line<double> const &b) {
            deque<Cross> aCross;
            deque<Cross> bCross;
            L aL(a.x0, a.y0, a.x1, a.y1);
            L bL(b.x0, b.y0, b.x1, b.y1);
            for (int m = 0; m < pl.lines.size(); m++) {
              auto const &l = pl.lines[m];
              L lL(l.x0, l.y0, l.x1, l.y1);
              {
                auto [t, u] = L::Intersection(aL, lL);
                if (t && u && 0 <= *t && *t <= 1 && 0 <= *u && *u <= 1) {
                  Cross c;
                  c.member = m;
                  c.t = *t;
                  aCross.push_back(c);
                }
              }
              {
                auto [t, u] = L::Intersection(bL, lL);
                if (t && u && 0 <= *t && *t <= 1 && 0 <= *u && *u <= 1) {
                  Cross c;
                  c.member = m;
                  c.t = *t;
                  bCross.push_back(c);
                }
              }
            }
            if (aCross.empty() && bCross.empty()) {
              return;
            }
            ranges::sort(aCross, [](auto const &a, auto const &b) { return a.t < b.t; });
            ranges::sort(bCross, [](auto const &a, auto const &b) { return a.t < b.t; });

            size_t aUnused = aCross.size();
            size_t bUnused = bCross.size();

            while (aUnused > 1) {
              auto c0i = ranges::find_if(aCross, [](Cross const &it) { return !it.used; });
              if (c0i == aCross.end()) {
                break;
              }
              Cross &c0 = *c0i;
              auto c1i = find_if(c0i + 1, aCross.end(), [](Cross const &it) { return !it.used; });
              if (c1i == aCross.end()) {
                break;
              }
              Cross &c1 = *c1i;
              c0.used = true;
              c1.used = true;
              aUnused -= 2;
              auto c2i = ranges::find_if(bCross, [&](Cross const &it) { return !it.used && it.member == c1.member; });
              if (c2i != bCross.end() && bUnused > 1) {
                Cross &c2 = *c2i;
                size_t idx = distance(c2i, bCross.end());
                Cross &c3 = bCross[(idx + 1) % bCross.size()];
                c2.used = true;
                c3.used = true;
                bUnused -= 2;
                Vec<double> p0 = aL.get(c0.t);
                Vec<double> p1 = aL.get(c1.t);
                Vec<double> p2 = bL.get(c2.t);
                Vec<double> p3 = bL.get(c3.t);
                Contour c;
                c.points.emplace_back((int16_t)round(p0.x), (int16_t)round(p0.y));
                c.points.emplace_back((int16_t)round(p1.x), (int16_t)round(p1.y));
                c.points.emplace_back((int16_t)round(p2.x), (int16_t)round(p2.y));
                c.points.emplace_back((int16_t)round(p3.x), (int16_t)round(p3.y));
                contours.push_back(c);
              } else {
                Vec<double> p0 = aL.get(c0.t);
                Vec<double> p1 = aL.get(c1.t);
                Vec<double> p2 = aL.get(1.0);
                Contour c;
                c.points.emplace_back((int16_t)round(p0.x), (int16_t)round(p0.y));
                c.points.emplace_back((int16_t)round(p1.x), (int16_t)round(p1.y));
                c.points.emplace_back((int16_t)round(p2.x), (int16_t)round(p2.y));
                contours.push_back(c);
              }
            }
          });
          auto gid = font.replaceSimpleGlyphByName(name, Class::Mark, contours, 0, x0);
          if (!gid) {
            return EGLYF_STATUS_PUSH(gid.status());
          }
          return Status::Ok();
        });
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
