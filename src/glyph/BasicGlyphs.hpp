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
#include "./hhea.hpp"

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
    read(32, res::tuffy::code32, res::tuffy::code32_advanceWidth);
    read(33, res::tuffy::code33, res::tuffy::code33_advanceWidth);
    read(34, res::tuffy::code34, res::tuffy::code34_advanceWidth);
    read(35, res::tuffy::code35, res::tuffy::code35_advanceWidth);
    read(36, res::tuffy::code36, res::tuffy::code36_advanceWidth);
    read(37, res::tuffy::code37, res::tuffy::code37_advanceWidth);
    read(38, res::tuffy::code38, res::tuffy::code38_advanceWidth);
    read(39, res::tuffy::code39, res::tuffy::code39_advanceWidth);
    read(40, res::tuffy::code40, res::tuffy::code40_advanceWidth);
    read(41, res::tuffy::code41, res::tuffy::code41_advanceWidth);
    read(42, res::tuffy::code42, res::tuffy::code42_advanceWidth);
    read(43, res::tuffy::code43, res::tuffy::code43_advanceWidth);
    read(44, res::tuffy::code44, res::tuffy::code44_advanceWidth);
    read(45, res::tuffy::code45, res::tuffy::code45_advanceWidth);
    read(46, res::tuffy::code46, res::tuffy::code46_advanceWidth);
    read(47, res::tuffy::code47, res::tuffy::code47_advanceWidth);
    read(48, res::tuffy::code48, res::tuffy::code48_advanceWidth);
    read(49, res::tuffy::code49, res::tuffy::code49_advanceWidth);
    read(50, res::tuffy::code50, res::tuffy::code50_advanceWidth);
    read(51, res::tuffy::code51, res::tuffy::code51_advanceWidth);
    read(52, res::tuffy::code52, res::tuffy::code52_advanceWidth);
    read(53, res::tuffy::code53, res::tuffy::code53_advanceWidth);
    read(54, res::tuffy::code54, res::tuffy::code54_advanceWidth);
    read(55, res::tuffy::code55, res::tuffy::code55_advanceWidth);
    read(56, res::tuffy::code56, res::tuffy::code56_advanceWidth);
    read(57, res::tuffy::code57, res::tuffy::code57_advanceWidth);
    read(58, res::tuffy::code58, res::tuffy::code58_advanceWidth);
    read(59, res::tuffy::code59, res::tuffy::code59_advanceWidth);
    read(60, res::tuffy::code60, res::tuffy::code60_advanceWidth);
    read(61, res::tuffy::code61, res::tuffy::code61_advanceWidth);
    read(62, res::tuffy::code62, res::tuffy::code62_advanceWidth);
    read(63, res::tuffy::code63, res::tuffy::code63_advanceWidth);
    read(64, res::tuffy::code64, res::tuffy::code64_advanceWidth);
    read(65, res::tuffy::code65, res::tuffy::code65_advanceWidth);
    read(66, res::tuffy::code66, res::tuffy::code66_advanceWidth);
    read(67, res::tuffy::code67, res::tuffy::code67_advanceWidth);
    read(68, res::tuffy::code68, res::tuffy::code68_advanceWidth);
    read(69, res::tuffy::code69, res::tuffy::code69_advanceWidth);
    read(70, res::tuffy::code70, res::tuffy::code70_advanceWidth);
    read(71, res::tuffy::code71, res::tuffy::code71_advanceWidth);
    read(72, res::tuffy::code72, res::tuffy::code72_advanceWidth);
    read(73, res::tuffy::code73, res::tuffy::code73_advanceWidth);
    read(74, res::tuffy::code74, res::tuffy::code74_advanceWidth);
    read(75, res::tuffy::code75, res::tuffy::code75_advanceWidth);
    read(76, res::tuffy::code76, res::tuffy::code76_advanceWidth);
    read(77, res::tuffy::code77, res::tuffy::code77_advanceWidth);
    read(78, res::tuffy::code78, res::tuffy::code78_advanceWidth);
    read(79, res::tuffy::code79, res::tuffy::code79_advanceWidth);
    read(80, res::tuffy::code80, res::tuffy::code80_advanceWidth);
    read(81, res::tuffy::code81, res::tuffy::code81_advanceWidth);
    read(82, res::tuffy::code82, res::tuffy::code82_advanceWidth);
    read(83, res::tuffy::code83, res::tuffy::code83_advanceWidth);
    read(84, res::tuffy::code84, res::tuffy::code84_advanceWidth);
    read(85, res::tuffy::code85, res::tuffy::code85_advanceWidth);
    read(86, res::tuffy::code86, res::tuffy::code86_advanceWidth);
    read(87, res::tuffy::code87, res::tuffy::code87_advanceWidth);
    read(88, res::tuffy::code88, res::tuffy::code88_advanceWidth);
    read(89, res::tuffy::code89, res::tuffy::code89_advanceWidth);
    read(90, res::tuffy::code90, res::tuffy::code90_advanceWidth);
    read(91, res::tuffy::code91, res::tuffy::code91_advanceWidth);
    read(92, res::tuffy::code92, res::tuffy::code92_advanceWidth);
    read(93, res::tuffy::code93, res::tuffy::code93_advanceWidth);
    read(94, res::tuffy::code94, res::tuffy::code94_advanceWidth);
    read(95, res::tuffy::code95, res::tuffy::code95_advanceWidth);
    read(96, res::tuffy::code96, res::tuffy::code96_advanceWidth);
    read(97, res::tuffy::code97, res::tuffy::code97_advanceWidth);
    read(98, res::tuffy::code98, res::tuffy::code98_advanceWidth);
    read(99, res::tuffy::code99, res::tuffy::code99_advanceWidth);
    read(100, res::tuffy::code100, res::tuffy::code100_advanceWidth);
    read(101, res::tuffy::code101, res::tuffy::code101_advanceWidth);
    read(102, res::tuffy::code102, res::tuffy::code102_advanceWidth);
    read(103, res::tuffy::code103, res::tuffy::code103_advanceWidth);
    read(104, res::tuffy::code104, res::tuffy::code104_advanceWidth);
    read(105, res::tuffy::code105, res::tuffy::code105_advanceWidth);
    read(106, res::tuffy::code106, res::tuffy::code106_advanceWidth);
    read(107, res::tuffy::code107, res::tuffy::code107_advanceWidth);
    read(108, res::tuffy::code108, res::tuffy::code108_advanceWidth);
    read(109, res::tuffy::code109, res::tuffy::code109_advanceWidth);
    read(110, res::tuffy::code110, res::tuffy::code110_advanceWidth);
    read(111, res::tuffy::code111, res::tuffy::code111_advanceWidth);
    read(112, res::tuffy::code112, res::tuffy::code112_advanceWidth);
    read(113, res::tuffy::code113, res::tuffy::code113_advanceWidth);
    read(114, res::tuffy::code114, res::tuffy::code114_advanceWidth);
    read(115, res::tuffy::code115, res::tuffy::code115_advanceWidth);
    read(116, res::tuffy::code116, res::tuffy::code116_advanceWidth);
    read(117, res::tuffy::code117, res::tuffy::code117_advanceWidth);
    read(118, res::tuffy::code118, res::tuffy::code118_advanceWidth);
    read(119, res::tuffy::code119, res::tuffy::code119_advanceWidth);
    read(120, res::tuffy::code120, res::tuffy::code120_advanceWidth);
    read(121, res::tuffy::code121, res::tuffy::code121_advanceWidth);
    read(122, res::tuffy::code122, res::tuffy::code122_advanceWidth);
    read(123, res::tuffy::code123, res::tuffy::code123_advanceWidth);
    read(124, res::tuffy::code124, res::tuffy::code124_advanceWidth);
    read(125, res::tuffy::code125, res::tuffy::code125_advanceWidth);
    read(126, res::tuffy::code126, res::tuffy::code126_advanceWidth);
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
