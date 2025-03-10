#pragma once

namespace eglyf {

class GlyphSubstitutionTable : public SubtableCollection<gsub::Subtable> {
public:
  Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<gsub::Subtable> &out) override {
    using namespace std;
    if (lookupType == 1) {
      return EGLYF_STATUS_PUSH(gsub::Single::Read(in, out));
    } else if (lookupType == 2) {
      return EGLYF_STATUS_PUSH(gsub::Multiple::Read(in, out));
    } else if (lookupType == 3) {
      return EGLYF_STATUS_PUSH(gsub::Alternate::Read(in, out));
    } else if (lookupType == 4) {
      return EGLYF_STATUS_PUSH(gsub::Ligature::Read(in, out));
    } else if (lookupType == 5) {
      return EGLYF_STATUS_PUSH(gsub::ContextualReader::Read(in, out));
    } else if (lookupType == 6) {
      return EGLYF_STATUS_PUSH(gsub::ChainedContextsSubstitution::Read(in, out));
    } else if (lookupType == 7) {
      return EGLYF_STATUS_PUSH(gsub::SubstitutionExtension::Read(in, out));
    } else if (lookupType == 8) {
      // Reverse chaining context single
      return EGLYF_ERROR;
    } else {
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
