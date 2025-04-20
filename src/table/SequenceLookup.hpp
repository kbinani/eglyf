#pragma once

namespace eglyf {

struct SequenceLookup {
  uint16_t sequenceIndex;
  std::variant<uint16_t, std::shared_ptr<SubtableCollection::Lookup>> lookup;

  static Optional<SequenceLookup> Read(InputStream &in) {
    using namespace std;
    SequenceLookup r;
    if (!in.u16(&r.sequenceIndex)) {
      return EGLYF_NULLOPT_WHAT("Failed to read sequenceIndex");
    }
    uint16_t lookupListIndex;
    if (!in.u16(&lookupListIndex)) {
      return EGLYF_NULLOPT_WHAT("Failed to read lookupListIndex");
    }
    r.lookup = lookupListIndex;
    return r;
  }

  Status write(OutputStream &out) const {
    if (!out.u16(sequenceIndex)) {
      return EGLYF_ERROR_WHAT("Failed to write sequenceIndex");
    }
    if (!holds_alternative<uint16_t>(lookup)) {
      return EGLYF_ERROR_WHAT("Lookup is not a uint16_t");
    }
    if (out.u16(get<uint16_t>(lookup))) {
      return Status::Ok();
    } else {
      return EGLYF_ERROR_WHAT("Failed to write lookup index");
    }
  }
};

} // namespace eglyf
