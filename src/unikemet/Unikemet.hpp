#pragma once

namespace eglyf {

class Unikemet {
  Unikemet() = delete;

public:
  static Status EnumerateAltSeq(std::function<Status(uint32_t, std::vector<uint32_t> const &)> cb) {
    using namespace std;
    static auto const create = []() -> unordered_map<uint32_t, vector<uint32_t>> * {
      auto p = new unordered_map<uint32_t, vector<uint32_t>>();
      uint32_t D50 = 0x130ad;
      uint32_t hj = 0x13431;
      uint32_t vj = 0x13430;
      unordered_map<uint32_t, vector<uint32_t>> &r = *p;
#include "./UnikemetAltSeq.hpp"
      r[0x130ae] = {D50, hj, D50};                                                                // D50A 𓂮
      r[0x130af] = {D50, hj, D50, hj, D50};                                                       // D50B 𓂯
      r[0x130b0] = {D50, hj, D50, hj, D50, hj, D50};                                              // D50C 𓂰
      r[0x130b1] = {D50, hj, D50, hj, D50, vj, D50, hj, D50};                                     // D50D 𓂱
      r[0x130b2] = {D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50};                            // D50E 𓂲
      r[0x130b3] = {D50, hj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50};                   // D50F 𓂳
      r[0x130b4] = {D50, hj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50, hj, D50};          // D50G 𓂴
      r[0x130b5] = {D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50, vj, D50, hj, D50, hj, D50}; // D50H 𓂵
      r[0x130b6] = {D50, hj, D50, hj, D50, hj, D50, hj, D50};                                     // D50I 𓂶
      return p;
    };
    static unique_ptr<unordered_map<uint32_t, vector<uint32_t>> const> const sTable(create());
    for (auto const &it : *sTable) {
      if (auto st = cb(it.first, it.second); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      uint32_t constexpr overlayMiddle = 0x13436;
      if (it.second.size() == 3 && it.second[1] == overlayMiddle && it.second[0] != it.second[2]) {
        auto copy = it.second;
        swap(copy[0], copy[2]);
        if (auto st = cb(it.first, copy); !st.ok()) {
          return EGLYF_STATUS_PUSH(st);
        }
      }
    }
    return Status::Ok();
  }
};

} // namespace eglyf
