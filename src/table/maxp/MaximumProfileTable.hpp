#pragma once

namespace eglyf::maxp {

// 'maxp'
class MaximumProfileTable : public Table {
public:
  static Status Read(InputStream &in, std::shared_ptr<MaximumProfileTable> &out) {
    using namespace std;
    auto r = make_unique<MaximumProfileTable>();
    if (!in.u16(&r->version.major)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->version.minor)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->numGlyphs)) {
      return EGLYF_ERROR;
    }
    if (r->version.major == 0 && r->version.minor == 0x5000) {
      out.reset(r.release());
      return Status::Ok();
    }
    if (r->version.major != 0x0001 || r->version.minor != 0) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxPoints)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxContours)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxCompositePoints)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxCompositeContours)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxZones)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxTwilightPoints)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxStorage)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxFunctionDefs)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxInstructionDefs)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxStackElements)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxSizeOfInstructions)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxComponentElements)) {
      return EGLYF_ERROR;
    }
    if (!in.u16(&r->maxComponentDepth)) {
      return EGLYF_ERROR;
    }
    out.reset(r.release());
    return Status::Ok();
  }

  Optional<EncodeResult> encode() const override {
    using namespace std;
    ByteOutputStream out;
    if (!out.u16(version.major)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(version.minor)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(numGlyphs)) {
      return EGLYF_NULLOPT;
    }
    if (version.major == 0 && version.minor == 0x5000) {
      return EncodeResult(out.data());
    } else if (version.major != 0x0001 || version.minor != 0) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxPoints)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxContours)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxCompositePoints)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxCompositeContours)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxZones)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxTwilightPoints)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxStorage)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxFunctionDefs)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxInstructionDefs)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxStackElements)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxSizeOfInstructions)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxComponentElements)) {
      return EGLYF_NULLOPT;
    }
    if (!out.u16(maxComponentDepth)) {
      return EGLYF_NULLOPT;
    }
    return EncodeResult(out.data());
  }

  Status clone(std::shared_ptr<MaximumProfileTable> &out) const {
    return EGLYF_STATUS_PUSH(defaultClone<MaximumProfileTable>(out));
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

} // namespace eglyf::maxp
