#pragma once

namespace ksesh {

struct TableDirectory {
  uint32_t sfntVersion;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;
  std::vector<TableRecord> tableRecords;

  static std::optional<TableDirectory> Read(InputStream &in) {
    using namespace std;
    TableDirectory td;
    if (!in.u32(&td.sfntVersion)) {
      return nullopt;
    }
    if (!in.u16(&td.numTables)) {
      return nullopt;
    }
    if (!in.u16(&td.searchRange)) {
      return nullopt;
    }
    if (!in.u16(&td.entrySelector)) {
      return nullopt;
    }
    if (!in.u16(&td.rangeShift)) {
      return nullopt;
    }
    td.tableRecords.resize(td.numTables);
    for (uint32_t i = 0; i < td.numTables; i++) {
      if (auto record = TableRecord::Read(in); record) {
        td.tableRecords[i] = *record;
      } else {
        return nullopt;
      }
    }
    return td;
  }
};

} // namespace ksesh
