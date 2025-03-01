#pragma once

namespace ksesh {

// 'maxp'
class MaximumProfileTable : public Table {
public:
  static std::shared_ptr<MaximumProfileTable> Read(InputStream &in) {
    using namespace std;
    auto r = make_shared<MaximumProfileTable>();
    if (!in.u16(&r->version.major)) {
      return nullptr;
    }
    if (!in.u16(&r->version.minor)) {
      return nullptr;
    }
    if (!in.u16(&r->numGlyphs)) {
      return nullptr;
    }
    if (r->version.major == 0 && r->version.minor == 0x5000) {
      return r;
    }
    if (r->version.major != 0x0001 || r->version.minor != 0) {
      return nullptr;
    }
    if (!in.u16(&r->maxPoints)) {
      return nullptr;
    }
    if (!in.u16(&r->maxContours)) {
      return nullptr;
    }
    if (!in.u16(&r->maxCompositePoints)) {
      return nullptr;
    }
    if (!in.u16(&r->maxCompositeContours)) {
      return nullptr;
    }
    if (!in.u16(&r->maxZones)) {
      return nullptr;
    }
    if (!in.u16(&r->maxTwilightPoints)) {
      return nullptr;
    }
    if (!in.u16(&r->maxStorage)) {
      return nullptr;
    }
    if (!in.u16(&r->maxFunctionDefs)) {
      return nullptr;
    }
    if (!in.u16(&r->maxInstructionDefs)) {
      return nullptr;
    }
    if (!in.u16(&r->maxStackElements)) {
      return nullptr;
    }
    if (!in.u16(&r->maxSizeOfInstructions)) {
      return nullptr;
    }
    if (!in.u16(&r->maxComponentElements)) {
      return nullptr;
    }
    if (!in.u16(&r->maxComponentDepth)) {
      return nullptr;
    }
    return r;
  }

  std::optional<EncodeResult> encode() override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version.major)) {
      return nullopt;
    }
    if (!out.u16(version.minor)) {
      return nullopt;
    }
    if (!out.u16(numGlyphs)) {
      return nullopt;
    }
    if (version.major == 0 && version.minor == 0x5000) {
      return EncodeResult(out.data());
    } else if (version.major != 0x0001 || version.minor != 0) {
      return nullopt;
    }
    if (!out.u16(maxPoints)) {
      return nullopt;
    }
    if (!out.u16(maxContours)) {
      return nullopt;
    }
    if (!out.u16(maxCompositePoints)) {
      return nullopt;
    }
    if (!out.u16(maxCompositeContours)) {
      return nullopt;
    }
    if (!out.u16(maxZones)) {
      return nullopt;
    }
    if (!out.u16(maxTwilightPoints)) {
      return nullopt;
    }
    if (!out.u16(maxStorage)) {
      return nullopt;
    }
    if (!out.u16(maxFunctionDefs)) {
      return nullopt;
    }
    if (!out.u16(maxInstructionDefs)) {
      return nullopt;
    }
    if (!out.u16(maxStackElements)) {
      return nullopt;
    }
    if (!out.u16(maxSizeOfInstructions)) {
      return nullopt;
    }
    if (!out.u16(maxComponentElements)) {
      return nullopt;
    }
    if (!out.u16(maxComponentDepth)) {
      return nullopt;
    }
    return EncodeResult(out.data());
  }

public:
  Version16Dot16 version;
  uint16_t numGlyphs = 0;
  uint16_t maxPoints = 0;
  uint16_t maxContours = 0;
  uint16_t maxCompositePoints = 0;
  uint16_t maxCompositeContours = 0;
  uint16_t maxZones = 0;
  uint16_t maxTwilightPoints = 0;
  uint16_t maxStorage = 0;
  uint16_t maxFunctionDefs = 0;
  uint16_t maxInstructionDefs = 0;
  uint16_t maxStackElements = 0;
  uint16_t maxSizeOfInstructions = 0;
  uint16_t maxComponentElements = 0;
  uint16_t maxComponentDepth = 0;
};

} // namespace ksesh
