#pragma once

namespace eglyf {

class CoverageBuilder {
  CoverageBuilder() = delete;

public:
  static std::shared_ptr<Coverage> Build(std::set<uint16_t> const &gids) {
    using namespace std;
    vector<Coverage2::RangeRecord> records;
    Coverage2::RangeRecord *last = nullptr;
    uint16_t currentCoverageIndex = 0;

    size_t const maxRecordCount = gids.size() / 3;

    for (auto gid : gids) {
      if (last) {
        if (gid == last->endGlyphID + 1) {
          last->endGlyphID = gid;
        } else {
          currentCoverageIndex += (last->endGlyphID - last->startGlyphID + 1);
          records.push_back({gid, gid, currentCoverageIndex});
          last = &records.back();
        }
      } else {
        records.push_back({gid, gid, 0});
        last = &records.back();
      }
      if (records.size() > maxRecordCount) {
        auto c = make_shared<Coverage1>();
        c->glyphArray = gids;
        return c;
      }
    }

    auto coverage = make_shared<Coverage2>();
    coverage->rangeRecords.swap(records);
    return coverage;
  }
};

} // namespace eglyf
