#pragma once

namespace eglyf::gpos {

class PositioningExtension : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<PositioningExtension>();
    if (!in.u16(&ret->extensionLookupType)) {
      return EGLYF_ERROR;
    }
    Offset32 extensionOffset;
    if (!in.o32(&extensionOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(extensionOffset)) {
      return EGLYF_ERROR;
    }
    switch (ret->extensionLookupType) {
    case 1:
      if (auto st = SingleAdjustment::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 2:
      if (auto st = PairAdjustmentPositioning::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 3:
      return EGLYF_ERROR;
    case 4:
      if (auto st = MarkToBaseAttachment::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 5:
      if (auto st = MarkToLigatureAttachmentPositioning::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 6:
      if (auto st = MarkToMarkAttachmentPositioning::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 7:
      if (auto st = Contextual::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 8:
      if (auto st = ChainedContexts::Read(in, ret->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
      break;
    case 9:
      return EGLYF_ERROR;
    default:
      return EGLYF_ERROR;
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out, std::map<std::shared_ptr<Subtable>, std::pair<std::shared_ptr<OffsetWriter>, OffsetWriter::Handle32>> &extensions) override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(1)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(extensionLookupType)) {
      return EGLYF_ERROR;
    }
    auto extensionOffset = writer->o32();
    if (!extensionOffset) {
      return EGLYF_ERROR;
    }
    extensions[extension] = make_pair(writer, extensionOffset);
    return Status::Ok();
  }

  size_t size() const override {
    return sizeof(uint16_t) * 2 + sizeof(Offset32);
  }

public:
  uint16_t extensionLookupType;
  std::shared_ptr<Subtable> extension;
};

} // namespace eglyf::gpos
