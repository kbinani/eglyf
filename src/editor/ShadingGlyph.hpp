#pragma once

namespace eglyf {

class ShadingGlyph {
private:
  static void Combination(int *indexes, int s, int rest, std::function<void(int *)> cb) {
    if (rest == 0) {
      cb(indexes);
    } else {
      if (s < 0) {
        return;
      }
      Combination(indexes, s - 1, rest, cb);
      indexes[rest - 1] = s;
      Combination(indexes, s - 1, rest - 1, cb);
    }
  }

  static void ForEachCombination(int n, int k, std::function<void(int *)> cb) {
    using namespace std;
    auto indexes = make_unique<int[]>(k);
    Combination(indexes.get(), n - 1, k, cb);
  }

  struct Line {
    Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
  };

  struct Polyline {
    explicit Polyline(std::initializer_list<Line> init) : lines(init.begin(), init.end()) {}

    std::vector<Line> lines;
  };

  static Polyline MakeRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    return Polyline({Line(x0, y0, x0, y1), Line(x0, y1, x1, y1), Line(x1, y1, x1, y0), Line(x1, y0, x0, y0)});
  }

  static void ForEachPolyline(int16_t x, int16_t y, int16_t w, int16_t h, std::function<void(std::set<int> member, Polyline const &)> cb) {
    // https://gyazo.com/d4b0feac3ef961076114f594eedf3219
    int16_t x0 = x;
    int16_t x1 = x + w / 2;
    int16_t x2 = x + w;
    int16_t y0 = y + h;
    int16_t y1 = y - h / 2;
    int16_t y2 = y - h;
    cb({1}, MakeRect(x0, y0, x1, y1));
    cb({2}, MakeRect(x0, y1, x1, y2));
    cb({3}, MakeRect(x1, y0, x2, y1));
    cb({4}, MakeRect(x1, y1, x2, y2));
    cb({1, 2}, MakeRect(x0, y0, x1, y2));
    cb({1, 3}, MakeRect(x0, y0, x2, y1));
    cb({1, 4}, Polyline({Line(x0, y0, x0, y1),
                         Line(x0, y1, x1, y1),
                         Line(x1, y1, x1, y2),
                         Line(x1, y2, x2, y2),
                         Line(x2, y2, x2, y1),
                         Line(x2, y1, x1, y1),
                         Line(x1, y1, x1, y0),
                         Line(x1, y0, x0, y0)}));
    cb({2, 3}, Polyline({
                   Line(x1, y1, x0, y1),
                   Line(x0, y1, x0, y2),
                   Line(x0, y2, x1, y2),
                   Line(x1, y2, x1, y1),
                   Line(x1, y1, x2, y1),
                   Line(x2, y1, x2, y0),
                   Line(x2, y0, x1, y0),
                   Line(x1, y0, x1, y1),
               }));
    cb({2, 4}, MakeRect(x0, y1, x2, y2));
    cb({3, 4}, MakeRect(x1, y0, x2, y2));
    cb({1, 2, 3}, Polyline({
                      Line(x2, y0, x0, y0),
                      Line(x0, y0, x0, y2),
                      Line(x0, y2, x1, y2),
                      Line(x1, y2, x1, y1),
                      Line(x1, y1, x2, y1),
                      Line(x2, y1, x2, y0),
                  }));
    cb({1, 2, 4}, Polyline({
                      Line(x1, y0, x0, y0),
                      Line(x0, y0, x0, y2),
                      Line(x0, y2, x2, y2),
                      Line(x2, y2, x2, y1),
                      Line(x2, y1, x1, y1),
                      Line(x1, y1, x1, y0),
                  }));
    cb({2, 3, 4}, Polyline({
                      Line(x2, y0, x1, y0),
                      Line(x1, y0, x1, y1),
                      Line(x1, y1, x0, y1),
                      Line(x0, y1, x0, y2),
                      Line(x0, y2, x2, y2),
                      Line(x2, y2, x2, y0),
                  }));
    cb({1, 2, 3, 4}, MakeRect(x0, y0, x2, y2));
  }

public:
  static Status Create(int hhu, int vhu) {
    using namespace std;
    for (int num = 1; num <= 4; num++) {
      ForEachCombination(4, num, [&](int *indexes) {
        set<int> member;
        for (int i = 0; i < num; i++) {
          member.insert(indexes[i] + 1);
        }
        string name = "dq";
        for (int m : member) {
          name += format("{}", m);
        }
        for (int h = 1; h <= hhu; h++) {
          for (int v = 1; v <= vhu; v++) {
            string n = format("{}_{}{}", name, h, v);
            // TODO: debug
            cout << n << endl;
          }
        }
      });
    }
    return EGLYF_ERROR;
  }
};

} // namespace eglyf
