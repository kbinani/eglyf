#pragma once

namespace eglyf {

class GlyphPositioningTable : public SubtableCollection<gpos::Subtable> {
public:
  Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<gpos::Subtable> &out) override {
    using namespace std;
    switch (lookupType) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    default:
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
