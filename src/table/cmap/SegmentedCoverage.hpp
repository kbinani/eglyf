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

  static Status FromSegmentMappingToDeltaValues(SegmentMappingToDeltaValues const &in, std::shared_ptr<SegmentedCoverage> &out) {
    using namespace std;
    if (in.segCount != in.startCode.size()) {
      return EGLYF_ERROR;
    }
    if (in.segCount != in.endCode.size()) {
      return EGLYF_ERROR;
    }
    if (in.segCount != in.idDelta.size()) {
      return EGLYF_ERROR;
    }
    if (in.segCount != in.idRangeOffset.size()) {
      return EGLYF_ERROR;
    }

    auto ret = make_unique<SegmentedCoverage>();
    ret->language = in.language;

    optional<SequentialMapGroup> last;
    for (uint16_t i = 0; i < in.segCount; i++) {
      uint16_t startCode = in.startCode[i];
      uint16_t endCode = in.endCode[i];
      int16_t idDelta = in.idDelta[i];
      uint16_t idRangeOffset = in.idRangeOffset[i];
      if (endCode < startCode) {
        return EGLYF_ERROR;
      }
      for (uint32_t code = startCode; code <= endCode; code++) {
        uint16_t gid;
        if (idRangeOffset == 0) {
          gid = ((int32_t)code + (int32_t)idDelta) & 0xffff;
        } else {
          size_t offset = idRangeOffset / 2;
          if (offset * 2 != idRangeOffset) {
            return EGLYF_ERROR;
          }
          size_t index = offset + (code - startCode) - i;
          if (index < in.glyphIdArray.size()) {
            gid = in.glyphIdArray[index];
          } else {
            return EGLYF_ERROR;
          }
        }
        if (last) {
          if (gid == last->startGlyphID + code - last->startCharCode) {
            last->endCharCode = code;
            continue;
          } else {
            ret->groups.push_back(*last);
          }
        }
        SequentialMapGroup group;
        group.startCharCode = code;
        group.endCharCode = code;
        group.startGlyphID = gid;
        last = group;
      }
    }
    if (last) {
      ret->groups.push_back(*last);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(12)) {
      return EGLYF_ERROR;
    }
    // reserved
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    auto const lengthPos = writer->o32();
    if (!lengthPos) {
      return EGLYF_ERROR;
    }
    if (!out.u32(language)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU32(groups.size())) {
      return EGLYF_ERROR;
    }
    for (auto const &group : groups) {
      if (!out.u32(group.startCharCode)) {
        return EGLYF_ERROR;
      }
      if (!out.u32(group.endCharCode)) {
        return EGLYF_ERROR;
      }
      if (!out.u32(group.startGlyphID)) {
        return EGLYF_ERROR;
      }
    }
    if (auto st = lengthPos->mark(); !st.ok()) {
      return EGLYF_STATUS_PUSH(st);
    }
    return EGLYF_STATUS_PUSH(writer->commit());
  }

  Optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    auto found = ranges::find_if(groups, [=](auto const &group) {
      return group.startCharCode <= codepoint && group.endCharCode <= codepoint;
    });
    if (found == groups.end()) {
      return 0;
    }
    uint32_t r = found->startGlyphID + codepoint - found->startCharCode;
    if (r > (uint32_t)numeric_limits<uint16_t>::max()) {
      return EGLYF_NULLOPT;
    } else {
      return static_cast<uint16_t>(r);
    }
  }

public:
  uint32_t language;
  std::vector<SequentialMapGroup> groups;
};

} // namespace eglyf::cmap
