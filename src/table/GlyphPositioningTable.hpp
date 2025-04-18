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
      return EGLYF_STATUS_PUSH(gpos::PairAdjustment::Read(in, out));
    case 3:
      return EGLYF_STATUS_PUSH(gpos::CursiveAttachment::Read(in, out));
    case 4:
      return EGLYF_STATUS_PUSH(gpos::MarkToBaseAttachment::Read(in, out));
    case 5:
      return EGLYF_STATUS_PUSH(gpos::MarkToLigatureAttachment::Read(in, out));
    case 6:
      return EGLYF_STATUS_PUSH(gpos::MarkToMarkAttachment::Read(in, out));
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

  std::optional<gpos::Attachment> findAttachment(uint16_t lookupType, uint16_t receptorGlyphID, uint16_t ligandGlyphID) const {
    using namespace std;
    for (auto const &lookup : lookups) {
      shared_ptr<Subtable> table;
      if (lookup->data->lookupType != 9 && lookupType != lookup->data->lookupType) {
        continue;
      }
      for (auto const &subtable : lookup->data->subtables) {
        shared_ptr<Subtable> table;
        uint16_t type;
        if (lookup->data->lookupType == 9) {
          auto e = dynamic_pointer_cast<gpos::PositioningExtension>(subtable);
          if (e->extensionLookupType != lookupType) {
            continue;
          }
          table = e->extension;
          type = e->extensionLookupType;
        } else {
          type = lookup->data->lookupType;
          table = subtable;
        }
        if (auto mark = dynamic_pointer_cast<gpos::MarkToBaseAttachment>(table); mark && type == 4) {
          if (auto found = mark->findAttachment(receptorGlyphID, ligandGlyphID); found) {
            return *found;
          }
        } else if (auto mkmk = dynamic_pointer_cast<gpos::MarkToMarkAttachment>(table); mkmk && type == 6) {
          if (auto found = mkmk->findAttachment(receptorGlyphID, ligandGlyphID); found) {
            return *found;
          }
        }
      }
    }
    return nullopt;
  }
};

} // namespace eglyf
