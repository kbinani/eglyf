#pragma once

namespace ksesh {

using Offset32 = uint32_t;

struct Fixed {
  uint32_t value;
};

using LONGDATETIME = int64_t;

struct Version16Dot16 {
  uint16_t major;
  uint16_t minor;
};

static_assert(sizeof(Version16Dot16) == sizeof(uint32_t));

} // namespace ksesh
