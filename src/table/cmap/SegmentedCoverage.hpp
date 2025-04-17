#pragma once

namespace eglyf::cmap {

// format 12
class SegmentedCoverage : public CmapSubtable {
public:
  static Status Read(InputStream &stream, std::shared_ptr<SegmentedCoverage> &out) {
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
      uint32_t startCharCode;
      if (!in.u32(&startCharCode)) {
        return EGLYF_ERROR;
      }
      uint32_t endCharCode;
      if (!in.u32(&endCharCode)) {
        return EGLYF_ERROR;
      }
      uint32_t startGlyphID;
      if (!in.u32(&startGlyphID)) {
        return EGLYF_ERROR;
      }
      for (uint32_t charCode = startCharCode; charCode <= endCharCode; charCode++) {
        ret->mapping[charCode] = startGlyphID + (charCode - startCharCode);
      }
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    auto start = out.position();
    if (!out.u16(12)) {
      return EGLYF_ERROR;
    }
    // reserved
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    auto const lengthPos = out.position();
    if (!out.u32(0)) {
      return EGLYF_ERROR;
    }
    if (!out.u32(language)) {
      return EGLYF_ERROR;
    }
    auto numGroupsPos = out.position();
    if (!out.u32(0)) {
      return EGLYF_ERROR;
    }
    struct Group {
      uint32_t startCharCode;
      uint32_t startGlyphID;
      uint32_t endCharCode;

      Group(uint32_t startCharCode, uint32_t startGlyphID) : startCharCode(startCharCode), startGlyphID(startGlyphID), endCharCode(startCharCode) {}
    };
    optional<Group> current;
    uint32_t numGroups = 0;
    for (auto it : mapping) {
      auto codepoint = it.first;
      auto gid = it.second;
      if (current) {
        if (current->endCharCode + 1 == codepoint && current->startGlyphID + (codepoint - current->startCharCode) == gid) {
          current->endCharCode = codepoint;
        } else {
          if (!out.u32(current->startCharCode)) {
            return EGLYF_ERROR;
          }
          if (!out.u32(current->endCharCode)) {
            return EGLYF_ERROR;
          }
          if (!out.u32(current->startGlyphID)) {
            return EGLYF_ERROR;
          }
          numGroups++;
          current = Group(codepoint, gid);
        }
      } else {
        current = Group(codepoint, gid);
      }
    }
    if (current) {
      if (!out.u32(current->startCharCode)) {
        return EGLYF_ERROR;
      }
      if (!out.u32(current->endCharCode)) {
        return EGLYF_ERROR;
      }
      if (!out.u32(current->startGlyphID)) {
        return EGLYF_ERROR;
      }
      numGroups++;
    }
    int64_t last = out.position();
    int64_t length = last - start;
    if (length > numeric_limits<uint32_t>::max()) {
      return EGLYF_ERROR;
    }
    if (!out.seek(lengthPos)) {
      return EGLYF_ERROR;
    }
    if (!out.u32((uint32_t)length)) {
      return EGLYF_ERROR;
    }
    if (numGroups > numeric_limits<uint32_t>::max()) {
      return EGLYF_ERROR;
    }
    if (!out.seek(numGroupsPos)) {
      return EGLYF_ERROR;
    }
    if (!out.u32((uint32_t)numGroups)) {
      return EGLYF_ERROR;
    }
    if (!out.seek(last)) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

  Optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    if (auto found = mapping.find(codepoint); found != mapping.end()) {
      if (found->second > numeric_limits<uint16_t>::max()) {
        return EGLYF_NULLOPT;
      } else {
        return (uint16_t)found->second;
      }
    }
    return EGLYF_NULLOPT;
  }

  Status map(uint32_t codepoint, uint16_t glyphID) {
    mapping[codepoint] = glyphID;
    return Status::Ok();
  }

public:
  uint32_t language;
  std::map<uint32_t, uint32_t> mapping;
};

} // namespace eglyf::cmap
