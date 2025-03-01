#pragma once

namespace eglyf::gsub {

struct SequenceLookup {
  uint16_t sequenceIndex;
  uint16_t lookupListIndex;

  static std::optional<SequenceLookup> Read(InputStream &in) {
    using namespace std;
    SequenceLookup r;
    if (!in.u16(&r.sequenceIndex)) {
      return nullopt;
    }
    if (!in.u16(&r.lookupListIndex)) {
      return nullopt;
    }
    return r;
  }
};

} // namespace eglyf::gsub
