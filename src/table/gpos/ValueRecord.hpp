#pragma once

namespace eglyf::gpos {

class ValueRecord {
public:
  enum ValueFormat {
    X_PLACEMENT = 0x1,
    Y_PLACEMENT = 0x2,
    X_ADVANCE = 0x4,
    Y_ADVANCE = 0x8,
    X_PLACEMENT_DEVICE = 0x10,
    Y_PLACEMENT_DEVICE = 0x20,
    X_ADVANCE_DEVICE = 0x40,
    Y_ADVANCE_DEVICE = 0x80,
  };

public:
  std::optional<int16_t> xPlacement;
  std::optional<int16_t> yPlacement;
  std::optional<int16_t> xAdvance;
  std::optional<int16_t> yAdvance;
  std::optional<Offset16> xPlaDeviceOffset;
  std::optional<Offset16> yPlaDeviceOffset;
  std::optional<Offset16> xAdvDeviceOffset;
  std::optional<Offset16> yAdvDeviceOffset;

  static Optional<ValueRecord> Read(InputStream &in, uint16_t format) {
    ValueRecord record;
    if (format & X_PLACEMENT) {
      int16_t xPlacement;
      if (!in.i16(&xPlacement)) {
        return EGLYF_NULLOPT;
      }
      record.xPlacement = xPlacement;
    }
    if (format & Y_PLACEMENT) {
      int16_t yPlacement;
      if (!in.i16(&yPlacement)) {
        return EGLYF_NULLOPT;
      }
      record.yPlacement = yPlacement;
    }
    if (format & X_ADVANCE) {
      int16_t xAdvance;
      if (!in.i16(&xAdvance)) {
        return EGLYF_NULLOPT;
      }
      record.xAdvance = xAdvance;
    }
    if (format & Y_ADVANCE) {
      int16_t yAdvance;
      if (!in.i16(&yAdvance)) {
        return EGLYF_NULLOPT;
      }
      record.yAdvance = yAdvance;
    }
    if (format & X_PLACEMENT_DEVICE) {
      return EGLYF_NULLOPT_WHAT("Unsupported property xPlaDeviceOffset");
    }
    if (format & Y_PLACEMENT_DEVICE) {
      return EGLYF_NULLOPT_WHAT("Unsupported property yPlaDeviceOffset");
    }
    if (format & X_ADVANCE_DEVICE) {
      return EGLYF_NULLOPT_WHAT("Unsupported property xAdvDeviceOffset");
    }
    if (format & Y_ADVANCE_DEVICE) {
      return EGLYF_NULLOPT_WHAT("Unsupported property yAdvDeviceOffset");
    }
    return record;
    return EGLYF_NULLOPT;
  }

  Status write(OutputStream &out) const {
    if (xPlacement) {
      if (!out.i16(*xPlacement)) {
        return EGLYF_ERROR;
      }
    }
    if (yPlacement) {
      if (!out.i16(*yPlacement)) {
        return EGLYF_ERROR;
      }
    }
    if (xAdvance) {
      if (!out.i16(*xAdvance)) {
        return EGLYF_ERROR;
      }
    }
    if (yAdvance) {
      if (!out.i16(*yAdvance)) {
        return EGLYF_ERROR;
      }
    }
    if (xPlaDeviceOffset) {
      return EGLYF_ERROR_WHAT("Unsupported property xPlaDeviceOffset");
    }
    if (yPlaDeviceOffset) {
      return EGLYF_ERROR_WHAT("Unsupported property yPlaDeviceOffset");
    }
    if (xAdvDeviceOffset) {
      return EGLYF_ERROR_WHAT("Unsupported property xAdvDeviceOffset");
    }
    if (yAdvDeviceOffset) {
      return EGLYF_ERROR_WHAT("Unsupported property yAdvDeviceOffset");
    }
    return Status::Ok();
  }

  Optional<uint16_t> format() const {
    uint16_t ret = 0;
    if (xPlacement) {
      ret |= X_PLACEMENT;
    }
    if (yPlacement) {
      ret |= Y_PLACEMENT;
    }
    if (xAdvance) {
      ret |= X_ADVANCE;
    }
    if (yAdvance) {
      ret |= Y_ADVANCE;
    }
    if (xPlaDeviceOffset) {
      return EGLYF_NULLOPT_WHAT("Unsupported property xPlaDeviceOffset");
    }
    if (yPlaDeviceOffset) {
      return EGLYF_NULLOPT_WHAT("Unsupported property yPlaDeviceOffset");
    }
    if (xAdvDeviceOffset) {
      return EGLYF_NULLOPT_WHAT("Unsupported property xAdvDeviceOffset");
    }
    if (yAdvDeviceOffset) {
      return EGLYF_NULLOPT_WHAT("Unsupported property yAdvDeviceOffset");
    }
    return ret;
  }

  size_t size() const {
    size_t ret = 0;
    if (xPlacement) {
      ret += sizeof(int16_t);
    }
    if (yPlacement) {
      ret += sizeof(int16_t);
    }
    if (xAdvance) {
      ret += sizeof(int16_t);
    }
    if (yAdvance) {
      ret += sizeof(int16_t);
    }
    if (xPlaDeviceOffset) {
      ret += sizeof(Offset16);
    }
    if (yPlaDeviceOffset) {
      ret += sizeof(Offset16);
    }
    if (xAdvDeviceOffset) {
      ret += sizeof(Offset16);
    }
    if (yAdvDeviceOffset) {
      ret += sizeof(Offset16);
    }
    return ret;
  }

  static size_t Size(uint16_t format) {
    size_t ret = 0;
    if (format & X_PLACEMENT) {
      ret += sizeof(int16_t);
    }
    if (format & Y_PLACEMENT) {
      ret += sizeof(int16_t);
    }
    if (format & X_ADVANCE) {
      ret += sizeof(int16_t);
    }
    if (format & Y_ADVANCE) {
      ret += sizeof(int16_t);
    }
    if (format & X_PLACEMENT_DEVICE) {
      ret += sizeof(Offset16);
    }
    if (format & Y_PLACEMENT_DEVICE) {
      ret += sizeof(Offset16);
    }
    if (format & X_ADVANCE_DEVICE) {
      ret += sizeof(Offset16);
    }
    if (format & Y_ADVANCE_DEVICE) {
      ret += sizeof(Offset16);
    }
    return ret;
  }
};

} // namespace eglyf::gpos
