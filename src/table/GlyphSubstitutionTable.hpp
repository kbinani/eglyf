#pragma once

namespace eglyf {

class GlyphSubstitutionTable : public SubtableCollection {
public:
  Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<Subtable> &out) override {
    using namespace std;
    switch (lookupType) {
    case 1:
      return EGLYF_STATUS_PUSH(gsub::Single::Read(in, out));
    case 2:
      return EGLYF_STATUS_PUSH(gsub::Multiple::Read(in, out));
    case 3:
      return EGLYF_STATUS_PUSH(gsub::Alternate::Read(in, out));
    case 4:
      return EGLYF_STATUS_PUSH(gsub::Ligature::Read(in, out));
    case 5:
      return EGLYF_STATUS_PUSH(Contextual::Read(in, out));
    case 6:
      return EGLYF_STATUS_PUSH(ChainedContextsReader::Read(in, out));
    case 7:
      return EGLYF_STATUS_PUSH(gsub::SubstitutionExtension::Read(in, out));
    case 8:
      return EGLYF_STATUS_PUSH(gsub::ReverseChainedContextsSingle::Read(in, out));
    default:
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
