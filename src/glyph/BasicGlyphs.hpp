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
  static std::map<uint32_t, glyf::GlyphDataTable::SimpleGlyph> *Load() {
    using namespace std;
    auto t = make_unique<map<uint32_t, glyf::GlyphDataTable::SimpleGlyph>>();
    auto const read = [&t](uint32_t codepoint, string_view data) {
      ByteInputStream in(data);
      auto header = glyf::GlyphDataTable::Header::Read(in);
      if (!header) {
        return;
      }
      auto sg = glyf::GlyphDataTable::SimpleGlyph::Read(*header, in);
      if (!sg) {
        return;
      }
      (*t)[codepoint] = *sg;
    };
    read(32, res::code32);
    read(33, res::code33);
    read(34, res::code34);
    read(35, res::code35);
    read(36, res::code36);
    read(37, res::code37);
    read(38, res::code38);
    read(39, res::code39);
    read(40, res::code40);
    read(41, res::code41);
    read(42, res::code42);
    read(43, res::code43);
    read(44, res::code44);
    read(45, res::code45);
    read(46, res::code46);
    read(47, res::code47);
    read(48, res::code48);
    read(49, res::code49);
    read(50, res::code50);
    read(51, res::code51);
    read(52, res::code52);
    read(53, res::code53);
    read(54, res::code54);
    read(55, res::code55);
    read(56, res::code56);
    read(57, res::code57);
    read(58, res::code58);
    read(59, res::code59);
    read(60, res::code60);
    read(61, res::code61);
    read(62, res::code62);
    read(63, res::code63);
    read(64, res::code64);
    read(65, res::code65);
    read(66, res::code66);
    read(67, res::code67);
    read(68, res::code68);
    read(69, res::code69);
    read(70, res::code70);
    read(71, res::code71);
    read(72, res::code72);
    read(73, res::code73);
    read(74, res::code74);
    read(75, res::code75);
    read(76, res::code76);
    read(77, res::code77);
    read(78, res::code78);
    read(79, res::code79);
    read(80, res::code80);
    read(81, res::code81);
    read(82, res::code82);
    read(83, res::code83);
    read(84, res::code84);
    read(85, res::code85);
    read(86, res::code86);
    read(87, res::code87);
    read(88, res::code88);
    read(89, res::code89);
    read(90, res::code90);
    read(91, res::code91);
    read(92, res::code92);
    read(93, res::code93);
    read(94, res::code94);
    read(95, res::code95);
    read(96, res::code96);
    read(97, res::code97);
    read(98, res::code98);
    read(99, res::code99);
    read(100, res::code100);
    read(101, res::code101);
    read(102, res::code102);
    read(103, res::code103);
    read(104, res::code104);
    read(105, res::code105);
    read(106, res::code106);
    read(107, res::code107);
    read(108, res::code108);
    read(109, res::code109);
    read(110, res::code110);
    read(111, res::code111);
    read(112, res::code112);
    read(113, res::code113);
    read(114, res::code114);
    read(115, res::code115);
    read(116, res::code116);
    read(117, res::code117);
    read(118, res::code118);
    read(119, res::code119);
    read(120, res::code120);
    read(121, res::code121);
    read(122, res::code122);
    read(123, res::code123);
    read(124, res::code124);
    read(125, res::code125);
    read(126, res::code126);
    return t.release();
  }

  static std::map<uint32_t, glyf::GlyphDataTable::SimpleGlyph> const &Table() {
    using namespace std;
    static unique_ptr<map<uint32_t, glyf::GlyphDataTable::SimpleGlyph> const> const sTable(Load());
    return *sTable;
  }

public:
  static std::optional<glyf::GlyphDataTable::SimpleGlyph> Get(uint32_t codepoint) {
    using namespace std;
    auto const &table = Table();
    auto found = table.find(codepoint);
    if (found == table.end()) {
      return nullopt;
    } else {
      return found->second;
    }
  }
};

} // namespace eglyf
