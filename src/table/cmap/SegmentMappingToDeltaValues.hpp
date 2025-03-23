#pragma once

namespace eglyf::cmap {

// format 4
class SegmentMappingToDeltaValues : public CmapSubtable {
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
    uint16_t segCount = segCountX2 / 2;
    ret->segCount = segCount;
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
    if (!in.u16a(ret->endCode, segCount)) {
      return EGLYF_ERROR;
    }
    uint16_t reservedPad;
    if (!in.u16(&reservedPad)) {
      return EGLYF_ERROR;
    }
    if (!in.u16a(ret->startCode, segCount)) {
      return EGLYF_ERROR;
    }
    if (!in.i16a(ret->idDelta, segCount)) {
      return EGLYF_ERROR;
    }
    if (!in.u16a(ret->idRangeOffset, segCount)) {
      return EGLYF_ERROR;
    }
    while (true) {
      uint16_t glyphId;
      if (!in.u16(&glyphId)) {
        break;
      }
      ret->glyphIdArray.push_back(glyphId);
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
    if (!out.sizeU16(2 * (size_t)segCount)) {
      return EGLYF_ERROR;
    }
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
    if (endCode.size() != segCount) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(endCode)) {
      return EGLYF_ERROR;
    }
    // reservedPad
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    if (startCode.size() != segCount) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(startCode)) {
      return EGLYF_ERROR;
    }
    if (idDelta.size() != segCount) {
      return EGLYF_ERROR;
    }
    if (!out.i16a(idDelta)) {
      return EGLYF_ERROR;
    }
    if (idRangeOffset.size() != segCount) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(idRangeOffset)) {
      return EGLYF_ERROR;
    }
    if (!out.u16a(glyphIdArray)) {
      return EGLYF_ERROR;
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
    auto found = ranges::find_if(endCode, [=](uint16_t ec) { return codepoint <= ec; });
    if (found == endCode.end()) {
      return 0;
    }
    size_t const i = distance(endCode.begin(), found);
    if (endCode.size() != startCode.size()) {
      return EGLYF_NULLOPT;
    }
    uint16_t const sc = startCode[i];
    if (codepoint < sc) {
      return 0;
    }
    if (idDelta.size() != startCode.size()) {
      return EGLYF_NULLOPT;
    }
    int16_t const iD = idDelta[i];
    if (idRangeOffset.size() != startCode.size()) {
      return EGLYF_NULLOPT;
    }
    uint16_t const iRO = idRangeOffset[i];
    if (iRO == 0) {
      return (codepoint + (int32_t)iD) & 0xffff;
    }
    size_t offset = iRO / 2;
    if (iRO != 2 * offset) {
      return EGLYF_NULLOPT;
    }
    size_t index = offset + (codepoint - sc) - i;
    if (index < glyphIdArray.size()) {
      return glyphIdArray[index];
    } else {
      return EGLYF_NULLOPT;
    }
  }

  Status enumerate(std::function<Status(uint32_t codepoint, uint16_t glyphId)> callback) const {
    for (uint16_t i = 0; i < segCount; i++) {
      uint16_t startCode = this->startCode[i];
      uint16_t endCode = this->endCode[i];
      int16_t idDelta = this->idDelta[i];
      uint16_t idRangeOffset = this->idRangeOffset[i];
      if (endCode < startCode) {
        return EGLYF_ERROR;
      }
      for (uint32_t code = startCode; code <= endCode; code++) {
        if (idRangeOffset == 0) {
          uint16_t gid = ((int32_t)code + (int32_t)idDelta) & 0xffff;
          if (auto st = callback(code, gid); !st.ok()) {
            return EGLYF_STATUS_PUSH(st);
          }
        } else {
          size_t offset = idRangeOffset / 2;
          if (offset * 2 != idRangeOffset) {
            return EGLYF_ERROR;
          }
          size_t index = offset + (code - startCode) - i;
          if (index < this->glyphIdArray.size()) {
            uint16_t gid = this->glyphIdArray[index];
            if (auto st = callback(code, gid); !st.ok()) {
              return EGLYF_STATUS_PUSH(st);
            }
          } else {
            return EGLYF_ERROR;
          }
        }
      }
    }
    return Status::Ok();
  }

public:
  uint16_t language;
  uint16_t segCount;
  std::vector<uint16_t> endCode;
  std::vector<uint16_t> startCode;
  std::vector<int16_t> idDelta;
  std::vector<uint16_t> idRangeOffset;
  std::vector<uint16_t> glyphIdArray;
};

} // namespace eglyf::cmap
