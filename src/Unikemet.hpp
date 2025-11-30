#pragma once

namespace eglyf {

class Unikemet {
  Unikemet() = delete;

public:
  static Status EnumerateAltSeq(std::function<Status(uint32_t, std::vector<uint32_t> const &)> cb) {
    using namespace std;
    static auto create = []() -> unordered_map<uint32_t, vector<uint32_t>> * {
      auto p = new unordered_map<uint32_t, vector<uint32_t>>();
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
