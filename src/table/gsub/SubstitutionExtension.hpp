#pragma once

namespace eglyf::gsub {

class SubstitutionExtension : public Subtable {
public:
  static std::shared_ptr<Subtable> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    uint16_t format;
    if (!in.u16(&format)) {
      return nullptr;
    }
    if (format != 1) {
      return nullptr;
    }
    auto r = make_shared<SubstitutionExtension>();
    if (!in.u16(&r->extensionLookupType)) {
      return nullptr;
    }
    if (r->extensionLookupType == 7) {
      return nullptr;
    }
    Offset32 extensionOffset;
    if (!in.o32(&extensionOffset)) {
      return nullptr;
    }
    if (!in.seek(extensionOffset)) {
      return nullptr;
    }
    OffsetInputStream sub(in);
    if (r->extensionLookupType == 1) {
      // Single
      if (auto t = gsub::Single::Read(sub); t) {
        r->extension = t;
      } else {
        return nullptr;
      }
    } else if (r->extensionLookupType == 2) {
      // Multiple
      if (auto t = gsub::Multiple::Read(sub); t) {
        r->extension = t;
      } else {
        return nullptr;
      }
    } else if (r->extensionLookupType == 3) {
      // Alternate
      return nullptr;
    } else if (r->extensionLookupType == 4) {
      // Ligature
      if (auto t = Ligature::Read(sub); t) {
        r->extension = t;
      } else {
        return nullptr;
      }
    } else if (r->extensionLookupType == 5) {
      // Contextual substitution
      return nullptr;
    } else if (r->extensionLookupType == 6) {
      // Chained contexts substitution
      if (auto t = ChainedContextsSubstitution::Read(sub); t) {
        r->extension = t;
      } else {
        return nullptr;
      }
    } else if (r->extensionLookupType == 8) {
      // Reverse chaining context single
      return nullptr;
    } else {
      return nullptr;
    }
    return r;
  }

  bool write(OutputStream &out) override {
    // TODO:
    return true;
  }

public:
  uint16_t extensionLookupType;
  std::shared_ptr<Subtable> extension;
};

} // namespace eglyf::gsub
