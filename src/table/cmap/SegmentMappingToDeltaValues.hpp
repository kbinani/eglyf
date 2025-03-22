#pragma once

namespace eglyf::cmap {

// format 4
class SegmentMappingToDeltaValues : public CmapSubtable {
public:
  static Status Read(InputStream &stream, std::shared_ptr<CmapSubtable> &out) {
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
    uint16_t entrySector;
    if (!in.u16(&entrySector)) {
      return EGLYF_ERROR;
    }
    uint16_t expectedEntrySector = (uint16_t)2 << (int)(floor(log2(segCount)) + 0.01f);
    if (entrySector != expectedEntrySector) {
      return EGLYF_ERROR;
    }
    uint16_t rangeShift;
    if (!in.u16(&rangeShift)) {
      return EGLYF_ERROR;
    }
    uint16_t expectedRangeShift = (uint16_t)(floor(log2(segCount)) + 0.01f);
    if (rangeShift != expectedRangeShift) {
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
