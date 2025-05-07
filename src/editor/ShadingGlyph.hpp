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
