#pragma once

namespace eglyf::cmap {

// format 12
class SegmentedCoverage : public CmapSubtable {
public:
  struct SequentialMapGroup {
    uint32_t startCharCode;
    uint32_t endCharCode;
    uint32_t startGlyphID;
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
    using namespace std;
    uint16_t reserved;
    if (!stream.u16(&reserved)) {
      return EGLYF_ERROR;
    }
    uint32_t length;
    if (!stream.u32(&length)) {
      return EGLYF_ERROR;
    }
    if (length < 8) {
      return EGLYF_ERROR;
    }
    string data;
    data.resize(length - 8);
    if (!stream.read(data.data(), data.size())) {
      return EGLYF_ERROR;
    }
    auto ret = make_unique<SegmentedCoverage>();
    ByteInputStream in(data);
    if (!in.u32(&ret->language)) {
      return EGLYF_ERROR;
    }
    uint32_t numGroups;
    if (!in.u32(&numGroups)) {
      return EGLYF_ERROR;
    }
    for (uint32_t i = 0; i < numGroups; i++) {
      SequentialMapGroup g;
      if (!in.u32(&g.startCharCode)) {
        return EGLYF_ERROR;
      }
      if (!in.u32(&g.endCharCode)) {
        return EGLYF_ERROR;
      }
      if (!in.u32(&g.startGlyphID)) {
        return EGLYF_ERROR;
      }
      ret->groups.push_back(g);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

public:
  uint32_t language;
  std::vector<SequentialMapGroup> groups;
};

} // namespace eglyf::cmap
