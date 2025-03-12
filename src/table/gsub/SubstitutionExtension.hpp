#pragma once

namespace eglyf::gsub {

class SubstitutionExtension : public Subtable {
public:
  static Status Read(InputStream &in, std::shared_ptr<Subtable> &out) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return EGLYF_ERROR;
    }
    if (format != 1) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<SubstitutionExtension>();
    if (!in.u16(&r->extensionLookupType)) {
      return EGLYF_ERROR;
    }
    if (r->extensionLookupType == 7) {
      return EGLYF_ERROR;
    }
    Offset32 extensionOffset;
    if (!in.o32(&extensionOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.seek(extensionOffset)) {
      return EGLYF_ERROR;
    }
    OffsetInputStream sub(&in);
    if (r->extensionLookupType == 1) {
      // Single
      if (auto st = gsub::Single::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 2) {
      // Multiple
      if (auto st = gsub::Multiple::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 3) {
      // Alternate
      if (auto st = gsub::Alternate::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 4) {
      // Ligature
      if (auto st = Ligature::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 5) {
      // Contextual substitution
      if (auto st = gsub::ContextualReader::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 6) {
      // Chained contexts substitution
      if (auto st = ChainedContexts::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else if (r->extensionLookupType == 8) {
      // Reverse chaining context single
      if (auto st = ReverseChainedContextsSingle::Read(sub, r->extension); !st.ok()) {
        return EGLYF_STATUS_PUSH(st);
      }
    } else {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
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
    auto offset = writer->o32();
    if (!offset) {
      return EGLYF_ERROR;
    }
    extensions[extension] = make_pair(writer, offset);
    return Status::Ok();
  }

  size_t size() const override {
    return sizeof(uint16_t) * 2 + sizeof(Offset32);
  }

public:
  uint16_t extensionLookupType;
  std::shared_ptr<Subtable> extension;
};

} // namespace eglyf::gsub
