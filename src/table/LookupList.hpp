#pragma once

namespace eglyf {

class LookupList {
public:
  struct Lookup {
    uint16_t lookupType;
    uint16_t lookupFlag;
    std::vector<Offset16> subtableOffsets;
    uint16_t markFilteringSet;
  };

public:
  static std::optional<LookupList> Read(InputStream &in) {
    using namespace std;
    jassert(in.position() == 0);
    LookupList ret;
    uint16_t lookupCount;
    if (!in.u16(&lookupCount)) {
      return nullopt;
    }
    vector<Offset16> lookupOffsets;
    lookupOffsets.reserve(lookupCount);
    for (uint16_t i = 0; i < lookupCount; i++) {
      Offset16 v;
      if (!in.o16(&v)) {
        return nullopt;
      }
      lookupOffsets.push_back(v);
    }
    for (uint16_t lookupOffset : lookupOffsets) {
      Lookup l;
      if (!in.seek(lookupOffset)) {
        return nullopt;
      }
      if (!in.u16(&l.lookupType)) {
        return nullopt;
      }
      if (!in.u16(&l.lookupFlag)) {
        return nullopt;
      }
      uint16_t subTableCount;
      if (!in.u16(&subTableCount)) {
        return nullopt;
      }
      l.subtableOffsets.reserve(subTableCount);
      for (uint16_t i = 0; i < subTableCount; i++) {
        Offset16 v;
        if (!in.o16(&v)) {
          return nullopt;
        }
        l.subtableOffsets.push_back(v);
      }
      if (!in.u16(&l.markFilteringSet)) {
        return nullopt;
      }
      ret.lookupTable.push_back(l);
    }
    return ret;
  }

public:
  std::vector<Lookup> lookupTable;
};

} // namespace eglyf
