#pragma once

namespace eglyf {

// 'OS/2'
class OS2AndWindowsMetricsTable : public Table {
public:
  static Status Read(InputStream &in, std::shared_ptr<OS2AndWindowsMetricsTable> &out) {
    using namespace std;
    uint16_t version;
    if (!in.u16(&version)) {
      return EGLYF_ERROR;
    }
    auto r = make_unique<OS2AndWindowsMetricsTable>();
    r->version = version;
    if (!in.i16(&r->xAvgCharWidth)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usWeightClass)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usWidthClass)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->fsType)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySubscriptXSize)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySubscriptYSize)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySubscriptXOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySubscriptYOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySuperscriptXSize)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySuperscriptYSize)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySuperscriptXOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->ySuperscriptYOffset)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->yStrikeoutSize)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->yStrikeoutPosition)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->sFamilyClass)) {
      return EGLYF_ERROR;
    }
    if (in.read(r->panose.data(), r->panose.size()) != r->panose.size()) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->ulUnicodeRange1)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->ulUnicodeRange2)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->ulUnicodeRange3)) {
      return EGLYF_ERROR;
    }
    if (!in.u32(&r->ulUnicodeRange4)) {
      return EGLYF_ERROR;
    }
    if (auto id = ReadTag(in); id) {
      r->achVendID = *id;
    } else {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->fsSelection)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usFirstCharIndex)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usLastCharIndex)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->sTypoAscender)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->sTypoDescender)) {
      return EGLYF_ERROR;
    }
    if (!in.i16(&r->sTypoLineGap)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usWinAscent)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->usWinDescent)) {
      return EGLYF_ERROR;
    }
    if (version > 0x000) {
      uint32_t v;
      if (!in.u32(&v)) {
        return EGLYF_ERROR;
      }
      r->ulCodePageRange1 = v;
      if (!in.u32(&v)) {
        return EGLYF_ERROR;
      }
      r->ulCodePageRange2 = v;
    }
    if (version > 0x0001) {
      FWORD fw;
      if (!in.i16(&fw)) {
        return EGLYF_ERROR;
      }
      r->sxHeight = fw;
      if (!in.i16(&fw)) {
        return EGLYF_ERROR;
      }
      r->sCapHeight = fw;
      uint16_t u16;
      if (!in.u16(&u16)) {
        return EGLYF_ERROR;
      }
      r->usDefaultChar = u16;
      if (!in.u16(&u16)) {
        return EGLYF_ERROR;
      }
      r->usBreakChar = u16;
      if (!in.u16(&u16)) {
        return EGLYF_ERROR;
      }
      r->usMaxContext = u16;
    }
    if (version > 0x0004) {
      uint16_t v;
      if (!in.u16(&v)) {
        return EGLYF_ERROR;
      }
      r->usLowerOpticalPointSize = v;
      if (!in.u16(&v)) {
        return EGLYF_ERROR;
      }
      r->usUpperOpticalPointSize = v;
    }
    if (version > 0x0005) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(xAvgCharWidth)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usWeightClass)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usWidthClass)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(fsType)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySubscriptXSize)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySubscriptYSize)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySubscriptXOffset)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySubscriptYOffset)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySuperscriptXSize)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySuperscriptYSize)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySuperscriptXOffset)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(ySuperscriptYOffset)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(yStrikeoutSize)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(yStrikeoutPosition)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(sFamilyClass)) {
      return EGLYF_NULLOPT;
    }
    if (!out.write(panose.data(), panose.size())) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(ulUnicodeRange1)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(ulUnicodeRange2)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(ulUnicodeRange3)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u32(ulUnicodeRange4)) {
      return EGLYF_NULLOPT;
    }
    if (!out.write(achVendID.data(), achVendID.size())) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(fsSelection)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usFirstCharIndex)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usLastCharIndex)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(sTypoAscender)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(sTypoDescender)) {
      return EGLYF_NULLOPT;
    }
    if (!out.i16(sTypoLineGap)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usWinAscent)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(usWinDescent)) {
      return EGLYF_NULLOPT;
    }
    if (version > 0x0000) {
      if (!ulCodePageRange1 || !ulCodePageRange2) {
        return EGLYF_NULLOPT;
      }
      if (!out.u32(*ulCodePageRange1)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u32(*ulCodePageRange2)) {
        return EGLYF_NULLOPT;
      }
    }
    if (version > 0x0001) {
      if (!sxHeight || !sCapHeight || !usDefaultChar || !usBreakChar || !usMaxContext) {
        return EGLYF_NULLOPT;
      }
      if (!out.i16(*sxHeight)) {
        return EGLYF_NULLOPT;
      }
      if (!out.i16(*sCapHeight)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(*usDefaultChar)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(*usBreakChar)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(*usMaxContext)) {
        return EGLYF_NULLOPT;
      }
    }
    if (version > 0x0004) {
      if (!usLowerOpticalPointSize || !usUpperOpticalPointSize) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(*usLowerOpticalPointSize)) {
        return EGLYF_NULLOPT;
      }
      if (!out.u16(*usUpperOpticalPointSize)) {
        return EGLYF_NULLOPT;
      }
    }
    if (version > 0x0005) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

public:
  // v0
  uint16_t version;
  FWORD xAvgCharWidth;
  uint16_t usWeightClass;
  uint16_t usWidthClass;
  uint16_t fsType;
  FWORD ySubscriptXSize;
  FWORD ySubscriptYSize;
  FWORD ySubscriptXOffset;
  FWORD ySubscriptYOffset;
  FWORD ySuperscriptXSize;
  FWORD ySuperscriptYSize;
  FWORD ySuperscriptXOffset;
  FWORD ySuperscriptYOffset;
  FWORD yStrikeoutSize;
  FWORD yStrikeoutPosition;
  int16_t sFamilyClass;
  std::array<uint8_t, 10> panose;
  uint32_t ulUnicodeRange1;
  uint32_t ulUnicodeRange2;
  uint32_t ulUnicodeRange3;
  uint32_t ulUnicodeRange4;
  Tag achVendID;
  uint16_t fsSelection;
  uint16_t usFirstCharIndex;
  uint16_t usLastCharIndex;
  FWORD sTypoAscender;
  FWORD sTypoDescender;
  FWORD sTypoLineGap;
  UFWORD usWinAscent;
  UFWORD usWinDescent;

  // v1
  std::optional<uint32_t> ulCodePageRange1;
  std::optional<uint32_t> ulCodePageRange2;

  // v2
  std::optional<FWORD> sxHeight;
  std::optional<FWORD> sCapHeight;
  std::optional<uint16_t> usDefaultChar;
  std::optional<uint16_t> usBreakChar;
  std::optional<uint16_t> usMaxContext;

  // v5
  std::optional<uint16_t> usLowerOpticalPointSize;
  std::optional<uint16_t> usUpperOpticalPointSize;
};

} // namespace eglyf
