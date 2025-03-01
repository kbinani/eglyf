#pragma once

namespace eglyf {

// 'OS/2'
class OS2AndWindowsMetricsTable : public Table {
public:
  static std::shared_ptr<OS2AndWindowsMetricsTable> Read(InputStream &in) {
    using namespace std;
    uint16_t version;
    if (!in.u16(&version)) {
      return nullptr;
    }
    auto r = make_shared<OS2AndWindowsMetricsTable>();
    r->version = version;
    if (!in.i16(&r->xAvgCharWidth)) {
      return nullptr;
    }
    if (!in.u16(&r->usWeightClass)) {
      return nullptr;
    }
    if (!in.u16(&r->usWidthClass)) {
      return nullptr;
    }
    if (!in.u16(&r->fsType)) {
      return nullptr;
    }
    if (!in.i16(&r->ySubscriptXSize)) {
      return nullptr;
    }
    if (!in.i16(&r->ySubscriptYSize)) {
      return nullptr;
    }
    if (!in.i16(&r->ySubscriptXOffset)) {
      return nullptr;
    }
    if (!in.i16(&r->ySubscriptYOffset)) {
      return nullptr;
    }
    if (!in.i16(&r->ySuperscriptXSize)) {
      return nullptr;
    }
    if (!in.i16(&r->ySuperscriptYSize)) {
      return nullptr;
    }
    if (!in.i16(&r->ySuperscriptXOffset)) {
      return nullptr;
    }
    if (!in.i16(&r->ySuperscriptYOffset)) {
      return nullptr;
    }
    if (!in.i16(&r->yStrikeoutSize)) {
      return nullptr;
    }
    if (!in.i16(&r->yStrikeoutPosition)) {
      return nullptr;
    }
    if (!in.i16(&r->sFamilyClass)) {
      return nullptr;
    }
    if (in.read(r->panose.data(), r->panose.size()) != r->panose.size()) {
      return nullptr;
    }
    if (!in.u32(&r->ulUnicodeRange1)) {
      return nullptr;
    }
    if (!in.u32(&r->ulUnicodeRange2)) {
      return nullptr;
    }
    if (!in.u32(&r->ulUnicodeRange3)) {
      return nullptr;
    }
    if (!in.u32(&r->ulUnicodeRange4)) {
      return nullptr;
    }
    if (auto id = ReadTag(in); id) {
      r->achVendID = *id;
    } else {
      return nullptr;
    }
    if (!in.u16(&r->fsSelection)) {
      return nullptr;
    }
    if (!in.u16(&r->usFirstCharIndex)) {
      return nullptr;
    }
    if (!in.u16(&r->usLastCharIndex)) {
      return nullptr;
    }
    if (!in.i16(&r->sTypoAscender)) {
      return nullptr;
    }
    if (!in.i16(&r->sTypoDescender)) {
      return nullptr;
    }
    if (!in.i16(&r->sTypoLineGap)) {
      return nullptr;
    }
    if (!in.u16(&r->usWinAscent)) {
      return nullptr;
    }
    if (!in.u16(&r->usWinDescent)) {
      return nullptr;
    }
    if (version > 0x000) {
      uint32_t v;
      if (!in.u32(&v)) {
        return nullptr;
      }
      r->ulCodePageRange1 = v;
      if (!in.u32(&v)) {
        return nullptr;
      }
      r->ulCodePageRange2 = v;
    }
    if (version > 0x0001) {
      FWORD fw;
      if (!in.i16(&fw)) {
        return nullptr;
      }
      r->sxHeight = fw;
      if (!in.i16(&fw)) {
        return nullptr;
      }
      r->sCapHeight = fw;
      uint16_t u16;
      if (!in.u16(&u16)) {
        return nullptr;
      }
      r->usDefaultChar = u16;
      if (!in.u16(&u16)) {
        return nullptr;
      }
      r->usBreakChar = u16;
      if (!in.u16(&u16)) {
        return nullptr;
      }
      r->usMaxContext = u16;
    }
    if (version > 0x0004) {
      uint16_t v;
      if (!in.u16(&v)) {
        return nullptr;
      }
      r->usLowerOpticalPointSize = v;
      if (!in.u16(&v)) {
        return nullptr;
      }
      r->usUpperOpticalPointSize = v;
    }
    if (version > 0x0005) {
      return nullptr;
    }
    return r;
  }

  std::optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version)) {
      return nullopt;
    }
    if (!out.i16(xAvgCharWidth)) {
      return nullopt;
    }
    if (!out.u16(usWeightClass)) {
      return nullopt;
    }
    if (!out.u16(usWidthClass)) {
      return nullopt;
    }
    if (!out.u16(fsType)) {
      return nullopt;
    }
    if (!out.i16(ySubscriptXSize)) {
      return nullopt;
    }
    if (!out.i16(ySubscriptYSize)) {
      return nullopt;
    }
    if (!out.i16(ySubscriptXOffset)) {
      return nullopt;
    }
    if (!out.i16(ySubscriptYOffset)) {
      return nullopt;
    }
    if (!out.i16(ySuperscriptXSize)) {
      return nullopt;
    }
    if (!out.i16(ySuperscriptYSize)) {
      return nullopt;
    }
    if (!out.i16(ySuperscriptXOffset)) {
      return nullopt;
    }
    if (!out.i16(ySuperscriptYOffset)) {
      return nullopt;
    }
    if (!out.i16(yStrikeoutSize)) {
      return nullopt;
    }
    if (!out.i16(yStrikeoutPosition)) {
      return nullopt;
    }
    if (!out.i16(sFamilyClass)) {
      return nullopt;
    }
    if (!out.write(panose.data(), panose.size())) {
      return nullopt;
    }
    if (!out.u32(ulUnicodeRange1)) {
      return nullopt;
    }
    if (!out.u32(ulUnicodeRange2)) {
      return nullopt;
    }
    if (!out.u32(ulUnicodeRange3)) {
      return nullopt;
    }
    if (!out.u32(ulUnicodeRange4)) {
      return nullopt;
    }
    if (!out.write(achVendID.data(), achVendID.size())) {
      return nullopt;
    }
    if (!out.u16(fsSelection)) {
      return nullopt;
    }
    if (!out.u16(usFirstCharIndex)) {
      return nullopt;
    }
    if (!out.u16(usLastCharIndex)) {
      return nullopt;
    }
    if (!out.i16(sTypoAscender)) {
      return nullopt;
    }
    if (!out.i16(sTypoDescender)) {
      return nullopt;
    }
    if (!out.i16(sTypoLineGap)) {
      return nullopt;
    }
    if (!out.u16(usWinAscent)) {
      return nullopt;
    }
    if (!out.u16(usWinDescent)) {
      return nullopt;
    }
    if (version > 0x0000) {
      if (!ulCodePageRange1 || !ulCodePageRange2) {
        return nullopt;
      }
      if (!out.u32(*ulCodePageRange1)) {
        return nullopt;
      }
      if (!out.u32(*ulCodePageRange2)) {
        return nullopt;
      }
    }
    if (version > 0x0001) {
      if (!sxHeight || !sCapHeight || !usDefaultChar || !usBreakChar || !usMaxContext) {
        return nullopt;
      }
      if (!out.i16(*sxHeight)) {
        return nullopt;
      }
      if (!out.i16(*sCapHeight)) {
        return nullopt;
      }
      if (!out.u16(*usDefaultChar)) {
        return nullopt;
      }
      if (!out.u16(*usBreakChar)) {
        return nullopt;
      }
      if (!out.u16(*usMaxContext)) {
        return nullopt;
      }
    }
    if (version > 0x0004) {
      if (!usLowerOpticalPointSize || !usUpperOpticalPointSize) {
        return nullopt;
      }
      if (!out.u16(*usLowerOpticalPointSize)) {
        return nullopt;
      }
      if (!out.u16(*usUpperOpticalPointSize)) {
        return nullopt;
      }
    }
    if (version > 0x0005) {
      return nullopt;
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
