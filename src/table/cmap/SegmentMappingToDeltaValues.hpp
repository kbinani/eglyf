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

public:
  uint16_t language;
  std::vector<Segment> segments;
};

} // namespace eglyf::cmap
