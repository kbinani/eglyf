#pragma once

namespace eglyf {

class Unikemet {
  Unikemet() = delete;

public:
  static Status EnumerateAltSeq(std::function<Status(uint32_t, std::vector<uint32_t> const &)> cb) {
    using namespace std;
    static auto create = []() -> unordered_map<uint32_t, vector<uint32_t>> * {
      auto p = new unordered_map<uint32_t, vector<uint32_t>>();
/*
"bs" = 0x13433
"om" = 0x13436
"hj" = 0x13431
"vj" = 0x13430
"ss" = 0x13437

 SUB GLYPH "G43" GLYPH "bs" GLYPH "X1"
 WITH GLYPH "G43a"
r[0x13172] = {0x13171, 0x13433, 0x133cf};

END_SUB
 SUB GLYPH "P8" GLYPH "om" GLYPH "I9"
 WITH GLYPH "P9"
END_SUB
r[0x132a5] = {0x132a4, 0x13436, 0x13191};
r[0x132a5] = {0x13191, 0x13436, 0x132a4};

 SUB GLYPH "S29" GLYPH "om" GLYPH "I9"
 WITH GLYPH "S30"
 r[0x132f5] = {0x132f4, 0x13436, 0x13191};

 SUB GLYPH "V28" GLYPH "hj" GLYPH "W14" GLYPH "vj" GLYPH "O34"
 WITH GLYPH "W14a"
r[0x133c0] = {0x1339b, 0x13431, 0x133bf, 0x13430, 0x13283};

 SUB GLYPH "I10" GLYPH "bs" GLYPH "ss" GLYPH "X1" GLYPH "vj" GLYPH "N17" GLYPH "se"
 WITH GLYPH "I11a"
END_SUB
r[0x13196] = {0x13193, 0x13433, 0x13437, 0x133cf, 0x13430, 0x131ff, 0x13438};

 SUB GLYPH "M23" GLYPH "hj" GLYPH "L2" GLYPH "vj" GLYPH "X1" GLYPH "hj" GLYPH "X1"
 WITH GLYPH "L2a"
END_SUB
r[0x131a5] = {0x131d3, 0x13431, 0x131a4, 0x13430, 0x133cf, 0x13431, 0x133cf};

 SUB GLYPH "O30" GLYPH "hj" GLYPH "O30" GLYPH "hj" GLYPH "O30" GLYPH "hj" GLYPH "O30"
 WITH GLYPH "O30a"
r[0x1327e] = {0x1327d, 0x13431, 0x1327d, 0x13431, 0x1327d, 0x13431, 0x1327d}
*/
      unordered_map<uint32_t, vector<uint32_t>> &r = *p;
#include "./res/UnikemetAltSeq.hpp"
      return p;
    };
    static unique_ptr<unordered_map<uint32_t, vector<uint32_t>> const> const sTable(create());
    for (auto const &it : *sTable) {
      if (auto st = cb(it.first, it.second); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
