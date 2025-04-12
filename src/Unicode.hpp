#pragma once

namespace eglyf {

class Unicode {
private:
  static std::vector<std::pair<uint32_t, uint32_t>> const &HieroglyphUnicodeRanges() {
    using namespace std;
    static unique_ptr<vector<pair<uint32_t, uint32_t>> const> const sTable([]() {
      auto v = make_unique<vector<pair<uint32_t, uint32_t>>>();
      v->push_back(make_pair(0x13000, 0x13257));
      v->push_back(make_pair(0x1325E, 0x13285));
      v->push_back(make_pair(0x1328A, 0x13378));
      v->push_back(make_pair(0x1337C, 0x1342E));
      v->push_back(make_pair(0x13000, 0x1342f));
      v->push_back(make_pair(0x13460, 0x143fa));
      return std::move(v);
    }());
    return *sTable;
  }

public:
  static void EnumerateHieroglyphUnicode(std::function<void(uint32_t)> cb) {
    for (auto [from, to] : HieroglyphUnicodeRanges()) {
      for (uint32_t cp = from; cp <= to; cp++) {
        cb(cp);
      }
    }
  }

  static bool IsHieroglyph(uint32_t cp) {
    using namespace std;
    auto const &table = HieroglyphUnicodeRanges();
    return ranges::find_if(table, [=](auto it) { return it.first <= cp && cp <= it.second; }) != table.end();
  }
};

} // namespace eglyf
