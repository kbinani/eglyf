#pragma once

namespace eglyf {

class GlyphPositioningTable : public SubtableCollection<Subtable> {
public:
  Status readSubtable(InputStream &in, uint16_t lookupType, std::shared_ptr<Subtable> &out) override {
    using namespace std;
    switch (lookupType) {
    case 1:
      return EGLYF_STATUS_PUSH(gpos::SingleAdjustment::Read(in, out));
    case 2:
      return EGLYF_STATUS_PUSH(gpos::PairAdjustmentPositioning::Read(in, out));
    case 3:
      return EGLYF_STATUS_PUSH(gpos::CursiveAttachmentPositioning::Read(in, out));
    case 4:
      return EGLYF_STATUS_PUSH(gpos::MarkToBaseAttachment::Read(in, out));
    case 5:
      return EGLYF_STATUS_PUSH(gpos::MarkToLigatureAttachmentPositioning::Read(in, out));
    case 6:
      return EGLYF_STATUS_PUSH(gpos::MarkToMarkAttachmentPositioning::Read(in, out));
    case 7:
      return EGLYF_STATUS_PUSH(Contextual::Read(in, out));
    case 8:
      return EGLYF_STATUS_PUSH(ChainedContextsReader::Read(in, out));
    case 9:
      return EGLYF_STATUS_PUSH(gpos::PositioningExtension::Read(in, out));
    default:
      return EGLYF_ERROR;
    }
  }
};

} // namespace eglyf
