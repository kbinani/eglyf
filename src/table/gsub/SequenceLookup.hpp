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

  bool write(OutputStream &out) const {
    if (!out.u16(sequenceIndex)) {
      return false;
    }
    return out.u16(lookupListIndex);
  }
};

} // namespace eglyf::gsub
