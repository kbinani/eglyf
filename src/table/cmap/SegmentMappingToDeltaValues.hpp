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
    vector<uint16_t> glyphIDArray;
    while (true) {
      uint16_t glyphID;
      if (!in.u16(&glyphID)) {
        break;
      }
      glyphIDArray.push_back(glyphID);
    }
    for (size_t i = 0; i < segCount; i++) {
      uint16_t _startCode = startCode[i];
      uint16_t _endCode = endCode[i];
      int16_t _idDelta = idDelta[i];
      uint16_t iRO = idRangeOffset[i];
      if (iRO > 0) {
        if (iRO % 2 == 1) {
          return EGLYF_ERROR;
        }
        // min: iRO / 2 + i - segCount
        // max: iRO / 2 + i - segCount + (s.endCode - s.startCode)
        if (iRO / 2 + i < segCount) {
          return EGLYF_ERROR;
        } else if (iRO / 2 + i + (_endCode - _startCode) - segCount >= glyphIDArray.size()) {
          return EGLYF_ERROR;
        }
        size_t offset = iRO / 2 + i - segCount;
        for (int c = _startCode; c <= _endCode; c++) {
          size_t index = offset + (size_t)(c - _startCode);
          auto gid = glyphIDArray[index];
          if (gid != 0) {
            ret->mapping[c] = glyphIDArray[index];
          }
        }
      } else {
        for (int c = _startCode; c <= _endCode; c++) {
          auto gid = (uint16_t)((c + (int32_t)_idDelta) & 0xffff);
          if (gid != 0) {
            ret->mapping[c] = gid;
          }
        }
      }
    }
    out.reset(ret.release());
    return Status::Ok();
  }

  Status write(OutputStream &out) const override {
    using namespace std;
    auto startPos = out.position();
    if (!out.u16(4)) {
      return EGLYF_ERROR;
    }
    if (!out.o16(0)) {
      return EGLYF_ERROR;
    }
    if (!out.u16(language)) {
      return EGLYF_ERROR;
    }

    struct Seg {
      uint16_t endCode;
      uint16_t startCode;
      int16_t idDelta;
      vector<uint16_t> glyphIDArray;
    };
    vector<Seg> segs;

    optional<Seg> current;
    for (auto const &it : mapping) {
      uint32_t codepoint = it.first;
      uint16_t gid = it.second;
      if (gid == 0) {
        continue;
      }

      int16_t const delta = 0xffff & ((0x10000 | (int32_t)gid) - (int32_t)codepoint);

      if (current) {
        if (current->endCode + 1 == codepoint) {
          if (current->idDelta == 0) {
            current->endCode = codepoint;
            current->glyphIDArray.push_back(gid);
          } else if (current->idDelta == delta) {
            current->endCode = codepoint;
            assert(current->glyphIDArray.empty());
          } else {
            segs.push_back(*current);
            Seg s;
            s.startCode = codepoint;
            s.endCode = codepoint;
            if (delta == 0) {
              s.idDelta = 0;
              s.glyphIDArray = {gid};
            } else {
              s.idDelta = delta;
            }
            current = s;
          }
        } else {
          segs.push_back(*current);
          Seg s;
          s.startCode = codepoint;
          s.endCode = codepoint;
          if (delta == 0) {
            s.idDelta = 0;
            s.glyphIDArray = {gid};
          } else {
            s.idDelta = delta;
          }
          current = s;
        }
      } else {
        Seg s;
        s.startCode = codepoint;
        s.endCode = codepoint;
        if (delta == 0) {
          s.idDelta = 0;
          s.glyphIDArray = {gid};
        } else {
          s.idDelta = delta;
        }
        current = s;
      }
    }
    if (current) {
      segs.push_back(*current);
      if (current->endCode != 0xffff) {
        Seg s;
        s.startCode = 0xffff;
        s.endCode = 0xffff;
        s.idDelta = 0;
        s.glyphIDArray.push_back(0);
        segs.push_back(s);
      }
    } else {
      Seg s;
      s.startCode = 0xffff;
      s.endCode = 0xffff;
      s.idDelta = 0;
      s.glyphIDArray.push_back(0);
      segs.push_back(s);
    }

    if (!out.sizeU16(2 * segs.size())) {
      return EGLYF_ERROR;
    }
    uint16_t segCount = segs.size();
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
    for (auto const &s : segs) {
      if (!out.u16(s.endCode)) {
        return EGLYF_ERROR;
      }
    }
    // reservedPad
    if (!out.u16(0)) {
      return EGLYF_ERROR;
    }
    for (auto const &s : segs) {
      if (!out.u16(s.startCode)) {
        return EGLYF_ERROR;
      }
    }
    for (auto const &s : segs) {
      if (!out.i16(s.idDelta)) {
        return EGLYF_ERROR;
      }
    }
    size_t offset = 0;
    for (size_t i = 0; i < segs.size(); i++) {
      auto const &s = segs[i];
      if (s.glyphIDArray.empty()) {
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
        offset += s.glyphIDArray.size();
      }
    }
    for (auto const &s : segs) {
      if (!out.u16a(s.glyphIDArray)) {
        return EGLYF_ERROR;
      }
    }
    auto last = out.position();
    auto size = last - startPos;
    if (size > numeric_limits<uint16_t>::max()) {
      return EGLYF_ERROR;
    }
    if (!out.seek(startPos + 2)) {
      return EGLYF_ERROR;
    }
    if (!out.o16((uint16_t)size)) {
      return EGLYF_ERROR;
    }
    if (!out.seek(last)) {
      return EGLYF_ERROR;
    }
    return Status::Ok();
  }

  Optional<uint16_t> getGlyphID(uint32_t codepoint) const {
    using namespace std;
    if (codepoint > 0xffff) {
      return 0;
    }
    if (auto found = mapping.find(codepoint); found != mapping.end()) {
      return found->second;
    } else {
      return 0;
    }
  }

  Status map(uint32_t codepoint, uint16_t glyphID) {
    if (glyphID == 0) {
      mapping.erase(codepoint);
    } else {
      mapping[codepoint] = glyphID;
    }
    return Status::Ok();
  }

public:
  uint16_t language;
  std::map<uint32_t, uint16_t> mapping;
};

} // namespace eglyf::cmap
