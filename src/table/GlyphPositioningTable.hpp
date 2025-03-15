#pragma once

namespace eglyf {

class GlyphPositioningTable : public SubtableCollection<gpos::Subtable> {
public:
  Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<gpos::Subtable> &out) override {
    using namespace std;
    switch (lookupType) {
    case 1:
      return EGLYF_ERROR;
    case 2:
      return EGLYF_ERROR;
    case 3:
      return EGLYF_ERROR;
    case 4:
      return EGLYF_STATUS_PUSH(gpos::MarkToBaseAttachment::Read(in, out));
    case 5:
      return EGLYF_ERROR;
    case 6:
      return EGLYF_ERROR;
    case 7:
      return EGLYF_ERROR;
    case 8:
      return EGLYF_ERROR;
    case 9:
      return EGLYF_STATUS_PUSH(gpos::PositioningExtension::Read(in, out));
    default:
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
