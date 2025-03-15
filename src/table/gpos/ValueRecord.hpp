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
};

} // namespace eglyf::gpos
