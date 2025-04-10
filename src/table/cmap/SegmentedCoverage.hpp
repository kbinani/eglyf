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
      return group.startCharCode <= codepoint && codepoint <= group.endCharCode;
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

  Status map(uint32_t codepoint, uint16_t glyphId) {
    using namespace std;
    auto found = ranges::find_if(groups, [=](auto const &g) {
      return codepoint <= g.endCharCode;
    });
    SequentialMapGroup single;
    single.startCharCode = codepoint;
    single.endCharCode = codepoint;
    single.startGlyphID = glyphId;
    if (found == groups.end()) {
      groups.push_back(single);
      return Status::Ok();
    }
    if (found->startGlyphID + (codepoint - found->startCharCode) == glyphId) {
      if (found->startCharCode <= codepoint) {
        return Status::Ok();
      }
    }
    size_t const index = distance(groups.begin(), found);
    auto &center = groups[index];
    if (center.startCharCode == codepoint && codepoint == center.endCharCode) {
      groups[index] = single;
      return Status::Ok();
    } else if (center.startCharCode <= codepoint) {
      if (center.startCharCode == codepoint) {
        center.startCharCode += 1;
        center.startGlyphID += 1;
        groups.insert(groups.begin() + index, single);
        return Status::Ok();
      } else if (center.endCharCode == codepoint) {
        center.endCharCode = codepoint - 1;
        groups.insert(groups.begin() + index + 1, single);
        return Status::Ok();
      } else {
        SequentialMapGroup left;
        left.startCharCode = center.startCharCode;
        left.endCharCode = codepoint - 1;
        left.startGlyphID = center.startGlyphID;

        SequentialMapGroup right;
        right.startCharCode = codepoint + 1;
        right.endCharCode = center.endCharCode;
        right.startGlyphID = center.startGlyphID + (codepoint + 1) - center.startCharCode;

        groups[index] = left;
        groups.insert(groups.begin() + index + 1, single);
        groups.insert(groups.begin() + index + 2, right);
        return Status::Ok();
      }
    } else if (index == 0) {
      groups.insert(groups.begin(), single);
      return Status::Ok();
    } else {
      auto &left = groups[index - 1];
      if (left.endCharCode + 1 == codepoint && left.startGlyphID + (codepoint - left.startCharCode) == glyphId) {
        left.endCharCode = codepoint;
        return Status::Ok();
      } else {
        groups.insert(groups.begin() + index - 1, single);
        return Status::Ok();
      }
    }
  }

public:
  uint32_t language;
  std::vector<SequentialMapGroup> groups;
};

} // namespace eglyf::cmap
