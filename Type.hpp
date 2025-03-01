#pragma once

namespace ksesh::otf {

using Offset16 = uint16_t;
using Offset32 = uint32_t;

struct Fixed {
  uint32_t value;
};

using LONGDATETIME = int64_t;
using FWORD = int16_t;

struct Version16Dot16 {
  uint16_t major;
  uint16_t minor;
};

static_assert(sizeof(Version16Dot16) == sizeof(uint32_t));

} // namespace ksesh::otf
