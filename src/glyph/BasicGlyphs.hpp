#pragma once
#include "glyph/100.hpp"
#include "glyph/101.hpp"
#include "glyph/102.hpp"
#include "glyph/103.hpp"
#include "glyph/104.hpp"
#include "glyph/105.hpp"
#include "glyph/106.hpp"
#include "glyph/107.hpp"
#include "glyph/108.hpp"
#include "glyph/109.hpp"
#include "glyph/110.hpp"
#include "glyph/111.hpp"
#include "glyph/112.hpp"
#include "glyph/113.hpp"
#include "glyph/114.hpp"
#include "glyph/115.hpp"
#include "glyph/116.hpp"
#include "glyph/117.hpp"
#include "glyph/118.hpp"
#include "glyph/119.hpp"
#include "glyph/120.hpp"
#include "glyph/121.hpp"
#include "glyph/122.hpp"
#include "glyph/123.hpp"
#include "glyph/124.hpp"
#include "glyph/125.hpp"
#include "glyph/126.hpp"
#include "glyph/32.hpp"
#include "glyph/33.hpp"
#include "glyph/34.hpp"
#include "glyph/35.hpp"
#include "glyph/36.hpp"
#include "glyph/37.hpp"
#include "glyph/38.hpp"
#include "glyph/39.hpp"
#include "glyph/40.hpp"
#include "glyph/41.hpp"
#include "glyph/42.hpp"
#include "glyph/43.hpp"
#include "glyph/44.hpp"
#include "glyph/45.hpp"
#include "glyph/46.hpp"
#include "glyph/47.hpp"
#include "glyph/48.hpp"
#include "glyph/49.hpp"
#include "glyph/50.hpp"
#include "glyph/51.hpp"
#include "glyph/52.hpp"
#include "glyph/53.hpp"
#include "glyph/54.hpp"
#include "glyph/55.hpp"
#include "glyph/56.hpp"
#include "glyph/57.hpp"
#include "glyph/58.hpp"
#include "glyph/59.hpp"
#include "glyph/60.hpp"
#include "glyph/61.hpp"
#include "glyph/62.hpp"
#include "glyph/63.hpp"
#include "glyph/64.hpp"
#include "glyph/65.hpp"
#include "glyph/66.hpp"
#include "glyph/67.hpp"
#include "glyph/68.hpp"
#include "glyph/69.hpp"
#include "glyph/70.hpp"
#include "glyph/71.hpp"
#include "glyph/72.hpp"
#include "glyph/73.hpp"
#include "glyph/74.hpp"
#include "glyph/75.hpp"
#include "glyph/76.hpp"
#include "glyph/77.hpp"
#include "glyph/78.hpp"
#include "glyph/79.hpp"
#include "glyph/80.hpp"
#include "glyph/81.hpp"
#include "glyph/82.hpp"
#include "glyph/83.hpp"
#include "glyph/84.hpp"
#include "glyph/85.hpp"
#include "glyph/86.hpp"
#include "glyph/87.hpp"
#include "glyph/88.hpp"
#include "glyph/89.hpp"
#include "glyph/90.hpp"
#include "glyph/91.hpp"
#include "glyph/92.hpp"
#include "glyph/93.hpp"
#include "glyph/94.hpp"
#include "glyph/95.hpp"
#include "glyph/96.hpp"
#include "glyph/97.hpp"
#include "glyph/98.hpp"
#include "glyph/99.hpp"

namespace eglyf {

class BasicGlyphs {
public:
  static uint32_t constexpr kMinCodepoint = 32;
  static uint32_t constexpr kMaxCodepoint = 126;

private:
  struct Element {
    std::optional<glyf::GlyphDataTable::SimpleGlyph> glyph;
    uint16_t advanceWidth;
  };

  static std::map<uint32_t, Element> *Load() {
    using namespace std;
    auto t = make_unique<map<uint32_t, Element>>();
    auto const read = [&t](uint32_t codepoint, string_view data, uint16_t advanceWidth) {
      Element e;
      e.advanceWidth = advanceWidth;
      if (data.empty()) {
        (*t)[codepoint] = e;
        return;
      }
      ByteInputStream in(data);
      auto header = glyf::GlyphDataTable::Header::Read(in);
      if (!header) {
        return;
      }
      auto sg = glyf::GlyphDataTable::SimpleGlyph::Read(*header, in);
      if (!sg) {
        return;
      }
      e.glyph = *sg;
      (*t)[codepoint] = e;
    };
    read(32, res::code32, res::code32_advanceWidth);
    read(33, res::code33, res::code33_advanceWidth);
    read(34, res::code34, res::code34_advanceWidth);
    read(35, res::code35, res::code35_advanceWidth);
    read(36, res::code36, res::code36_advanceWidth);
    read(37, res::code37, res::code37_advanceWidth);
    read(38, res::code38, res::code38_advanceWidth);
    read(39, res::code39, res::code39_advanceWidth);
    read(40, res::code40, res::code40_advanceWidth);
    read(41, res::code41, res::code41_advanceWidth);
    read(42, res::code42, res::code42_advanceWidth);
    read(43, res::code43, res::code43_advanceWidth);
    read(44, res::code44, res::code44_advanceWidth);
    read(45, res::code45, res::code45_advanceWidth);
    read(46, res::code46, res::code46_advanceWidth);
    read(47, res::code47, res::code47_advanceWidth);
    read(48, res::code48, res::code48_advanceWidth);
    read(49, res::code49, res::code49_advanceWidth);
    read(50, res::code50, res::code50_advanceWidth);
    read(51, res::code51, res::code51_advanceWidth);
    read(52, res::code52, res::code52_advanceWidth);
    read(53, res::code53, res::code53_advanceWidth);
    read(54, res::code54, res::code54_advanceWidth);
    read(55, res::code55, res::code55_advanceWidth);
    read(56, res::code56, res::code56_advanceWidth);
    read(57, res::code57, res::code57_advanceWidth);
    read(58, res::code58, res::code58_advanceWidth);
    read(59, res::code59, res::code59_advanceWidth);
    read(60, res::code60, res::code60_advanceWidth);
    read(61, res::code61, res::code61_advanceWidth);
    read(62, res::code62, res::code62_advanceWidth);
    read(63, res::code63, res::code63_advanceWidth);
    read(64, res::code64, res::code64_advanceWidth);
    read(65, res::code65, res::code65_advanceWidth);
    read(66, res::code66, res::code66_advanceWidth);
    read(67, res::code67, res::code67_advanceWidth);
    read(68, res::code68, res::code68_advanceWidth);
    read(69, res::code69, res::code69_advanceWidth);
    read(70, res::code70, res::code70_advanceWidth);
    read(71, res::code71, res::code71_advanceWidth);
    read(72, res::code72, res::code72_advanceWidth);
    read(73, res::code73, res::code73_advanceWidth);
    read(74, res::code74, res::code74_advanceWidth);
    read(75, res::code75, res::code75_advanceWidth);
    read(76, res::code76, res::code76_advanceWidth);
    read(77, res::code77, res::code77_advanceWidth);
    read(78, res::code78, res::code78_advanceWidth);
    read(79, res::code79, res::code79_advanceWidth);
    read(80, res::code80, res::code80_advanceWidth);
    read(81, res::code81, res::code81_advanceWidth);
    read(82, res::code82, res::code82_advanceWidth);
    read(83, res::code83, res::code83_advanceWidth);
    read(84, res::code84, res::code84_advanceWidth);
    read(85, res::code85, res::code85_advanceWidth);
    read(86, res::code86, res::code86_advanceWidth);
    read(87, res::code87, res::code87_advanceWidth);
    read(88, res::code88, res::code88_advanceWidth);
    read(89, res::code89, res::code89_advanceWidth);
    read(90, res::code90, res::code90_advanceWidth);
    read(91, res::code91, res::code91_advanceWidth);
    read(92, res::code92, res::code92_advanceWidth);
    read(93, res::code93, res::code93_advanceWidth);
    read(94, res::code94, res::code94_advanceWidth);
    read(95, res::code95, res::code95_advanceWidth);
    read(96, res::code96, res::code96_advanceWidth);
    read(97, res::code97, res::code97_advanceWidth);
    read(98, res::code98, res::code98_advanceWidth);
    read(99, res::code99, res::code99_advanceWidth);
    read(100, res::code100, res::code100_advanceWidth);
    read(101, res::code101, res::code101_advanceWidth);
    read(102, res::code102, res::code102_advanceWidth);
    read(103, res::code103, res::code103_advanceWidth);
    read(104, res::code104, res::code104_advanceWidth);
    read(105, res::code105, res::code105_advanceWidth);
    read(106, res::code106, res::code106_advanceWidth);
    read(107, res::code107, res::code107_advanceWidth);
    read(108, res::code108, res::code108_advanceWidth);
    read(109, res::code109, res::code109_advanceWidth);
    read(110, res::code110, res::code110_advanceWidth);
    read(111, res::code111, res::code111_advanceWidth);
    read(112, res::code112, res::code112_advanceWidth);
    read(113, res::code113, res::code113_advanceWidth);
    read(114, res::code114, res::code114_advanceWidth);
    read(115, res::code115, res::code115_advanceWidth);
    read(116, res::code116, res::code116_advanceWidth);
    read(117, res::code117, res::code117_advanceWidth);
    read(118, res::code118, res::code118_advanceWidth);
    read(119, res::code119, res::code119_advanceWidth);
    read(120, res::code120, res::code120_advanceWidth);
    read(121, res::code121, res::code121_advanceWidth);
    read(122, res::code122, res::code122_advanceWidth);
    read(123, res::code123, res::code123_advanceWidth);
    read(124, res::code124, res::code124_advanceWidth);
    read(125, res::code125, res::code125_advanceWidth);
    read(126, res::code126, res::code126_advanceWidth);
    return t.release();
  }

  static std::map<uint32_t, Element> const &Table() {
    using namespace std;
    static unique_ptr<map<uint32_t, Element> const> const sTable(Load());
    return *sTable;
  }

public:
  static std::optional<glyf::GlyphDataTable::SimpleGlyph> GetGlyph(uint32_t codepoint) {
    using namespace std;
    auto const &table = Table();
    auto found = table.find(codepoint);
    if (found == table.end()) {
      return nullopt;
    } else {
      return found->second.glyph;
    }
  }

  static std::optional<uint16_t> GetAdvanceWidth(uint32_t codepoint) {
    using namespace std;
    auto const &table = Table();
    auto found = table.find(codepoint);
    if (found == table.end()) {
      return nullopt;
    } else {
      return found->second.advanceWidth;
    }
  }
};

} // namespace eglyf
