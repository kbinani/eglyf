#pragma once

namespace eglyf::cmap {

// format 4
class SegmentMappingToDeltaValues : public CmapSubtable {
public:
  struct Segment {
    uint16_t endCode;
    uint16_t startCode;
    int16_t idDelta;
    std::vector<uint16_t> glyphIdArray;
  };

public:
  static Status Read(InputStream &stream, std::shared_ptr<SegmentMappingToDeltaValues> &out) {
    using namespace std;
    uint16_t length;
    if (!stream.u16(&length)) {
      return EGLYF_ERROR;
    }
    if (length < 4) {
      return EGLYF_ERROR;
    }
    string buffer;
    buffer.resize(length - 4);
    if (!stream.read(buffer.data(), buffer.size())) {
      return EGLYF_ERROR;
    }
    ByteInputStream in(buffer);
    auto ret = make_unique<SegmentMappingToDeltaValues>();
    if (!in.u16(&ret->language)) {
      return EGLYF_ERROR;
    }
    uint16_t segCountX2;
    if (!in.u16(&segCountX2)) {
      return EGLYF_ERROR;
    }
    if (segCountX2 % 2 == 1) {
      return EGLYF_ERROR;
    }
    uint16_t const segCount = segCountX2 / 2;
    uint16_t searchRange;
    if (!in.u16(&searchRange)) {
      return EGLYF_ERROR;
    }
    uint16_t const expectedSearchRange = (uint16_t)2 << (int)(floor(log2(segCount)) + 0.01f);
    if (searchRange != expectedSearchRange) {
      return EGLYF_ERROR;
    }
    uint16_t entrySelector;
    if (!in.u16(&entrySelector)) {
      return EGLYF_ERROR;
    }
    uint16_t const expectedEntrySelector = (uint16_t)(floor(log2(segCount)) + 0.01f);
    if (entrySelector != expectedEntrySelector) {
      return EGLYF_ERROR;
    }
    uint16_t rangeShift;
    if (!in.u16(&rangeShift)) {
      return EGLYF_ERROR;
    }
    uint16_t const expectedRangeShift = (segCount * 2) - searchRange;
    if (searchRange != expectedSearchRange) {
      return EGLYF_ERROR;
    }
    vector<uint16_t> endCode;
    if (!in.u16a(endCode, segCount)) {
      return EGLYF_ERROR;
    }
    uint16_t reservedPad;
    if (!in.u16(&reservedPad)) {
      return EGLYF_ERROR;
    }
    vector<uint16_t> startCode;
    if (!in.u16a(startCode, segCount)) {
      return EGLYF_ERROR;
    }
    vector<int16_t> idDelta;
    if (!in.i16a(idDelta, segCount)) {
      return EGLYF_ERROR;
    }
    vector<uint16_t> idRangeOffset;
    if (!in.u16a(idRangeOffset, segCount)) {
      return EGLYF_ERROR;
    }
    vector<uint16_t> glyphIdArray;
    while (true) {
      uint16_t glyphId;
      if (!in.u16(&glyphId)) {
        break;
      }
      glyphIdArray.push_back(glyphId);
    }
    for (size_t i = 0; i < segCount; i++) {
      Segment s;
      s.startCode = startCode[i];
      s.endCode = endCode[i];
      s.idDelta = idDelta[i];
      uint16_t iRO = idRangeOffset[i];
      if (iRO > 0) {
        if (iRO % 2 == 1) {
          return EGLYF_ERROR;
        }
        // min: iRO / 2 + i - segCount
        // max: iRO / 2 + i - segCount + (s.endCode - s.startCode)
        if (iRO / 2 + i < segCount) {
          return EGLYF_ERROR;
        } else if (iRO / 2 + i + (s.endCode - s.startCode) - segCount >= glyphIdArray.size()) {
          return EGLYF_ERROR;
        }
        size_t offset = iRO / 2 + i - segCount;
        for (uint16_t c = s.startCode; c <= s.endCode; c++) {
          size_t index = offset + (size_t)(c - s.startCode);
          s.glyphIdArray.push_back(glyphIdArray[index]);
        }
      }
      ret->segments.push_back(s);
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    auto writer = make_shared<OffsetWriter>(out);
    if (!out.u16(4)) {
      return EGLYF_ERROR;
    }
    auto lengthPos = writer->o16();
    if (!lengthPos) {
      return EGLYF_ERROR;
    }
    if (!out.u16(language)) {
      return EGLYF_ERROR;
    }
    if (!out.sizeU16(2 * (size_t)segments.size())) {
      return EGLYF_ERROR;
    }
    uint16_t segCount = segments.size();
    uint16_t const searchRange = (uint16_t)2 << (int)(floor(log2(segCount)) + 0.01f);
    if (!out.u16(searchRange)) {
      return EGLYF_ERROR;
    }
    uint16_t const entrySelector = (uint16_t)(floor(log2(segCount)) + 0.01f);
    if (!out.u16(entrySelector)) {
      return EGLYF_ERROR;
    }
    uint16_t const rangeShift = (segCount * 2) - searchRange;
    if (!out.u16(rangeShift)) {
      return EGLYF_ERROR;
    }
    for (auto const &s : segments) {
      if (!out.u16(s.endCode)) {
        return EGLYF_ERROR;
      }
    }
    // reservedPad
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    for (auto const &s : segments) {
      if (!out.u16(s.startCode)) {
        return EGLYF_ERROR;
      }
    }
    for (auto const &s : segments) {
      if (!out.i16(s.idDelta)) {
        return EGLYF_ERROR;
      }
    }
    size_t offset = 0;
    for (size_t i = 0; i < segments.size(); i++) {
      auto const &s = segments[i];
      if (s.glyphIdArray.empty()) {
        if (!out.u16(0)) {
          return EGLYF_ERROR;
        }
      } else {
        size_t idRangeOffset = (segCount - i + offset) * 2;
        if (idRangeOffset > (size_t)numeric_limits<uint16_t>::max()) {
          return EGLYF_ERROR;
        }
        uint16_t iRO = idRangeOffset;
        if (!out.u16(iRO)) {
          return EGLYF_ERROR;
        }
        offset += s.glyphIdArray.size();
      }
    }
    for (auto const &s : segments) {
      if (!out.u16a(s.glyphIdArray)) {
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
    if (codepoint > 0xffff) {
      return 0;
    }
    auto found = ranges::find_if(segments, [=](Segment const &s) { return codepoint <= s.endCode; });
    if (found == segments.end()) {
      return 0;
    }
    size_t const i = distance(segments.begin(), found);
    Segment const &s = segments[i];
    if (codepoint < s.startCode) {
      return 0;
    }
    if (s.glyphIdArray.empty()) {
      return (codepoint + (int32_t)s.idDelta) & 0xffff;
    }
    size_t index = codepoint - s.startCode;
    if (index < s.glyphIdArray.size()) {
      return s.glyphIdArray[index];
    } else {
      return EGLYF_NULLOPT;
    }
  }

  Status map(uint32_t codepoint, uint16_t glyphId) {
    using namespace std;
    if (codepoint > 0xffff) {
      return EGLYF_ERROR;
    }
    uint16_t const cp = static_cast<uint16_t>(codepoint);
    int64_t const delta = (int64_t)glyphId - (int64_t)codepoint;

    Segment single;
    single.startCode = cp;
    single.endCode = cp;
    if (delta == 0 || delta < (int64_t)numeric_limits<int16_t>::lowest() || (int64_t)numeric_limits<int16_t>::max() < delta) {
      single.idDelta = 0;
      single.glyphIdArray.push_back(glyphId);
    } else {
      single.idDelta = (int16_t)delta;
    }

    if (segments.empty()) {
      segments.push_back(single);
      return Status::Ok();
    }

    auto &back = segments.back();
    auto found = ranges::find_if(segments, [=](Segment const &s) { return cp <= s.endCode; });

    if (found == segments.end()) {
      if (back.endCode + 1 == cp) {
        if (!back.glyphIdArray.empty()) {
          back.endCode = cp;
          back.glyphIdArray.push_back(glyphId);
          return Status::Ok();
        } else if (back.idDelta == delta) {
          assert(back.idDelta != 0);
          back.endCode = cp;
          return Status::Ok();
        }
      }
      segments.push_back(single);
      return Status::Ok();
    }

    if (!found->glyphIdArray.empty()) {
      assert(found->glyphIdArray.size() == found->endCode - found->startCode + 1);
      size_t index = codepoint - found->startCode;
      found->glyphIdArray[index] = glyphId;
      return Status::Ok();
    }
    if (delta != 0 && found->idDelta == delta) {
      return Status::Ok();
    }

    size_t const index = distance(segments.begin(), found);
    if (index > 0) {
      Segment &left = segments[index - 1];
      if (left.endCode + 1 == cp) {
        if (!left.glyphIdArray.empty()) {
          left.endCode = cp;
          left.glyphIdArray.push_back(glyphId);
          return Status::Ok();
        } else if (left.idDelta == delta) {
          assert(left.idDelta != 0);
          left.endCode = cp;
          return Status::Ok();
        }
      }
    }
    if (index + 1 < segments.size()) {
      Segment &right = segments[index + 1];
      if (cp + 1 == right.startCode) {
        if (!right.glyphIdArray.empty()) {
          right.startCode = cp;
          right.glyphIdArray.insert(right.glyphIdArray.begin(), glyphId);
          return Status::Ok();
        } else if (right.idDelta == delta) {
          assert(right.idDelta != 0);
          right.startCode = cp;
          return Status::Ok();
        }
      }
    }

    Segment &center = segments[index];
    if (codepoint < center.startCode) {
      segments.insert(segments.begin() + index, single);
      return Status::Ok();
    }

    if (center.startCode == codepoint) {
      center.startCode += 1;
      assert(center.startCode <= center.endCode);
      if (!center.glyphIdArray.empty()) {
        center.glyphIdArray.erase(center.glyphIdArray.begin());
      }
      segments.insert(segments.begin() + index, single);
      return Status::Ok();
    } else if (center.endCode == codepoint) {
      center.endCode -= 1;
      assert(center.startCode <= center.endCode);
      if (!center.glyphIdArray.empty()) {
        center.glyphIdArray.pop_back();
      }
      segments.insert(segments.begin() + index + 1, single);
      return Status::Ok();
    } else {
      Segment copy = center;

      center.endCode = codepoint - 1;
      assert(center.startCode <= center.endCode);
      if (!center.glyphIdArray.empty()) {
        center.glyphIdArray.resize(center.endCode - center.startCode + 1);
      }
      Segment right;
      right.startCode = codepoint + 1;
      right.endCode = copy.endCode;
      assert(right.startCode <= right.endCode);
      right.idDelta = copy.idDelta;
      if (!copy.glyphIdArray.empty()) {
        for (uint32_t code = codepoint + 1; code <= copy.endCode; code++) {
          right.glyphIdArray.push_back(copy.glyphIdArray[code - copy.startCode]);
        }
      }
      segments.insert(segments.begin() + index + 1, single);
      segments.insert(segments.begin() + index + 2, right);
      return Status::Ok();
    }
  }

public:
  uint16_t language;
  std::vector<Segment> segments;
};

} // namespace eglyf::cmap
