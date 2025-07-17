#pragma once
#include "./100.hpp"
#include "./101.hpp"
#include "./102.hpp"
#include "./103.hpp"
#include "./104.hpp"
#include "./105.hpp"
#include "./106.hpp"
#include "./107.hpp"
#include "./108.hpp"
#include "./109.hpp"
#include "./110.hpp"
#include "./111.hpp"
#include "./112.hpp"
#include "./113.hpp"
#include "./114.hpp"
#include "./115.hpp"
#include "./116.hpp"
#include "./117.hpp"
#include "./118.hpp"
#include "./119.hpp"
#include "./120.hpp"
#include "./121.hpp"
#include "./122.hpp"
#include "./123.hpp"
#include "./124.hpp"
#include "./125.hpp"
#include "./126.hpp"
#include "./32.hpp"
#include "./33.hpp"
#include "./34.hpp"
#include "./35.hpp"
#include "./36.hpp"
#include "./37.hpp"
#include "./38.hpp"
#include "./39.hpp"
#include "./40.hpp"
#include "./41.hpp"
#include "./42.hpp"
#include "./43.hpp"
#include "./44.hpp"
#include "./45.hpp"
#include "./46.hpp"
#include "./47.hpp"
#include "./48.hpp"
#include "./49.hpp"
#include "./50.hpp"
#include "./51.hpp"
#include "./52.hpp"
#include "./53.hpp"
#include "./54.hpp"
#include "./55.hpp"
#include "./56.hpp"
#include "./57.hpp"
#include "./58.hpp"
#include "./59.hpp"
#include "./60.hpp"
#include "./61.hpp"
#include "./62.hpp"
#include "./63.hpp"
#include "./64.hpp"
#include "./65.hpp"
#include "./66.hpp"
#include "./67.hpp"
#include "./68.hpp"
#include "./69.hpp"
#include "./70.hpp"
#include "./71.hpp"
#include "./72.hpp"
#include "./73.hpp"
#include "./74.hpp"
#include "./75.hpp"
#include "./76.hpp"
#include "./77.hpp"
#include "./78.hpp"
#include "./79.hpp"
#include "./80.hpp"
#include "./81.hpp"
#include "./82.hpp"
#include "./83.hpp"
#include "./84.hpp"
#include "./85.hpp"
#include "./86.hpp"
#include "./87.hpp"
#include "./88.hpp"
#include "./89.hpp"
#include "./90.hpp"
#include "./91.hpp"
#include "./92.hpp"
#include "./93.hpp"
#include "./94.hpp"
#include "./95.hpp"
#include "./96.hpp"
#include "./97.hpp"
#include "./98.hpp"
#include "./99.hpp"

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
