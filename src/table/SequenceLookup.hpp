#pragma once

namespace eglyf {

struct SequenceLookup {
  uint16_t sequenceIndex;
  uint16_t lookupListIndex;

  static Optional<SequenceLookup> Read(InputStream &in) {
    using namespace std;
    SequenceLookup r;
    if (!in.u16(&r.sequenceIndex)) {
      return EGLYF_NULLOPT;
    }
    if (!in.u16(&r.lookupListIndex)) {
      return EGLYF_NULLOPT;
    }
    return r;
  }

  Status write(OutputStream &out) const {
    if (!out.u16(sequenceIndex)) {
      return EGLYF_ERROR;
    }
    if (out.u16(lookupListIndex)) {
      return Status::Ok();
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
