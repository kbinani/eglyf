#pragma once

namespace eglyf {

class GlyphSubstitutionTable : public SubtableCollection<gsub::Subtable> {
public:
  std::shared_ptr<gsub::Subtable> readSubtable(InputStream &in, uint16_t lookupType) override {
    using namespace std;
    if (lookupType == 1) {
      return gsub::Single::Read(in);
    } else if (lookupType == 2) {
      return gsub::Multiple::Read(in);
    } else if (lookupType == 3) {
      // Alternate
      return nullptr;
    } else if (lookupType == 4) {
      return gsub::Ligature::Read(in);
    } else if (lookupType == 5) {
      // Contextual substitution
      return nullptr;
    } else if (lookupType == 6) {
      return gsub::ChainedContextsSubstitution::Read(in);
    } else if (lookupType == 7) {
      return gsub::SubstitutionExtension::Read(in);
    } else if (lookupType == 8) {
      // Reverse chaining context single
      return nullptr;
    } else {
      return nullptr;
    }
  }
};

} // namespace eglyf
