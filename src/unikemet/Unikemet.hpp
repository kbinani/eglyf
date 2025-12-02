#pragma once

namespace eglyf {

class Unikemet {
  Unikemet() = delete;

public:
  static Status EnumerateAltSeq(std::function<Status(uint32_t, std::vector<uint32_t> const &)> cb) {
    using namespace std;
    static auto const create = []() -> unordered_map<uint32_t, vector<uint32_t>> * {
      auto p = new unordered_map<uint32_t, vector<uint32_t>>();
      unordered_map<uint32_t, vector<uint32_t>> &r = *p;
#include "./UnikemetAltSeq.hpp"
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
