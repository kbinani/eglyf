#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define HB_NO_PRAGMA_GCC_DIAGNOSTIC_WARNING
#define HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR
#include "hb.hh"

#include "eglyf.hpp"

template <auto *Dtor>
struct Destructor {
  template <typename Ptr>
  void operator()(Ptr *ptr) const {
    if (ptr != nullptr) {
      Dtor(ptr);
    }
  }
};

using HbBlobUniquePtr = std::unique_ptr<hb_blob_t, Destructor<hb_blob_destroy>>;
using HbFaceUniquePtr = std::unique_ptr<hb_face_t, Destructor<hb_face_destroy>>;
using HbFontUniquePtr = std::unique_ptr<hb_font_t, Destructor<hb_font_destroy>>;
using HbBufferUniquePtr = std::unique_ptr<hb_buffer_t, Destructor<hb_buffer_destroy>>;

static hb_buffer_t *CreateBuffer(std::u32string const &str, hb_font_t *font, std::vector<std::string> const &features) {
  HbBufferUniquePtr buffer(hb_buffer_create());
  hb_buffer_add_utf32(buffer.get(), (uint32_t const *)str.c_str(), -1, 0, -1);
  hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
  hb_buffer_set_script(buffer.get(), HB_SCRIPT_EGYPTIAN_HIEROGLYPHS);
  hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
  std::vector<hb_feature_t> fts;
  for (auto const &n : features) {
    hb_feature_t f;
    REQUIRE(hb_feature_from_string(n.c_str(), n.size(), &f));
    fts.push_back(f);
  }
  hb_shape(font, buffer.get(), fts.data(), fts.size());
  return buffer.release();
}

static std::shared_ptr<eglyf::Font> MakeFont(std::string const &file) {
  using namespace std;
  using namespace eglyf;
  auto fis = make_unique<FileInputStream>(file);
  shared_ptr<Font> font;
  REQUIRE(Font::Read(*fis, font).ok());
  fis.reset();
  Config cfg;
  REQUIRE(Transformer::Transform(font, cfg).ok());
  return font;
}

static std::deque<std::string> GetGlyphNames(hb_buffer_t *buffer, eglyf::Font const &font) {
  using namespace std;
  deque<string> ret;
  auto numGlyphs = hb_buffer_get_length(buffer);
  hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buffer, nullptr);
  for (unsigned int i = 0; i < numGlyphs; i++) {
    hb_glyph_info_t info = glyphInfo[i];
    auto gid = info.codepoint;
    auto name = font.postGetName(gid);
    REQUIRE(name);
    ret.push_back(*name);
  }
  return ret;
}

struct Fixture {
  Fixture() {
    using namespace std;
    using namespace eglyf;
    font = MakeFont("test/asset/NotoSansEgyptianHieroglyphs-Regular.ttf");
    REQUIRE(font);
    auto out = make_unique<ByteOutputStream>();
    REQUIRE(font->write(*out).ok());
    string data = out->data();
    out.reset();
    REQUIRE(!data.empty());
    hbBlob.reset(hb_blob_create(data.data(), data.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr));
    hbFace.reset(hb_face_create(hbBlob.get(), 0));
    hbFont.reset(hb_font_create(hbFace.get()));
    REQUIRE(hbFont);
  }

  std::shared_ptr<eglyf::Font> font;
  HbBlobUniquePtr hbBlob;
  HbFaceUniquePtr hbFace;
  HbFontUniquePtr hbFont;
};

TEST_CASE_FIXTURE(Fixture, "main") {
  using namespace std;
  using namespace eglyf;
  namespace fs = std::filesystem;

  SUBCASE("tmp") {
    HbBufferUniquePtr buffer(CreateBuffer(U"𓇳𓮆𓁦"s, hbFont.get(), {}));
    auto names = GetGlyphNames(buffer.get(), *font);
    for (auto const &n : names) {
      cout << n << endl;
    }
  }

  SUBCASE("otf") {
    auto fis = make_unique<FileInputStream>("test/asset/NotoSansEgyptianHieroglyphs-Regular.otf");
    shared_ptr<Font> font;
    auto st = Font::Read(*fis, font);
    CHECK(st.ok());
    st.print(cout);
  }

  SUBCASE("vertical-rl") {
    HbBufferUniquePtr buffer(CreateBuffer(U"𓀀𓑀𓀀"s, hbFont.get(), {"rtlm", "vrt2"}));
    auto names = GetGlyphNames(buffer.get(), *font);
    REQUIRE(names.size() == 16);
    CHECK(names[5] == "A1");
    CHECK(names[13] == "A1R");
  }

  SUBCASE("vertical-lr") {
    HbBufferUniquePtr buffer(CreateBuffer(U"𓀀𓑀𓀀"s, hbFont.get(), {"vrt2"}));
    auto names = GetGlyphNames(buffer.get(), *font);
    REQUIRE(names.size() == 16);
    CHECK(names[5] == "A1R");
    CHECK(names[13] == "A1");
  }

  SUBCASE("cartouche-glyph") {
    vector<string> glyphs = {
        // clang-format off
        "cbL", "creR", "cbR", "creL", "cbT", "creB", "cdreB", "crbL", "ceR", "crbR",
        "ceL", "crbT", "ceB", "cdeB", "cdrbT", "corbT", "coeB", "cobL", "coreR", "cobR",
        "coreL", "cobT", "coreB", "corbL", "coeR", "corbR", "coeL", "hwtbL", "hwteR", "hwtbR",
        "hwteL", "hwtobL", "hwtoeR", "hwtobR", "hwtoeL", "hwttbL", "hwttbR", "hwtteL", "hwtteR", "hwtbbL",
        "hwtbeR", "hwtbbR", "hwtbeL", "hwtdbL", "hwtdeR", "hwtdbR", "hwtdeL", "hwtotbL", "hwtoteR", "hwtotbR",
        "hwtoteL", "hwtobbL", "hwtobeR", "hwtobbR", "hwtobeL", "hwtdtbL", "hwtdbbL", "hwtdteR", "hwtdbeR", "hwtdteL",
        "hwtdtbR", "hwtdbbR", "hwtdbeL", "hwtbT", "hwteB", "hwtdbT", "hwtdeB", "hwttbT", "hwtbbT", "hwtbeB",
        "hwtteB", "hwtdtbT", "hwtdbbT", "hwtdbeB", "hwtdteB", "hwtobT", "hwtoeB", "hwtotbT", "hwtobbT", "hwtoteB",
        "hwtobeB", "O33aeL", "O33aeR", "O33aoeL", "O33aoeR", "O33adeL", "O33adeR", "O33aeB", "O33adeB", "O33aoeB",
        "cdbL", "cdbR", "cdreL", "cdreR", "cdbT", "cdrbL", "cdrbR", "cdeL", "cdeR", "cwbL",
        "cweR", "cwbR", "cweL", "cfbL", "cfeR", "cfbR", "cfeL", "cwbT", "cweB", "cfbT",
        "cfeB", "hwbL", "hweR", "hwbR", "hweL", "hfbL", "hfeR", "hfbR", "hfeL", "hwbT",
        "hweB", "hfbT", "hfeB",
        // clang-format on
    };
    for (int ih = 1; ih <= 8; ih++) {
      glyphs.push_back(format("QC{}", ih));
      glyphs.push_back(format("QC{}V", ih));
      glyphs.push_back(format("QO{}", ih));
      glyphs.push_back(format("QC{}V", ih));
      glyphs.push_back(format("QD{}", ih));
      glyphs.push_back(format("QD{}V", ih));
      glyphs.push_back(format("QW{}", ih));
      glyphs.push_back(format("QW{}V", ih));
      glyphs.push_back(format("QF{}", ih));
      glyphs.push_back(format("QF{}V", ih));
    }
    for (string const &pre : {"1", "2", "3", "4", "12", "13", "14", "23", "24", "34", "123", "124", "134", "234", "1234"}) {
      for (int ih = 1; ih <= 8; ih++) {
        for (int iv = 1; iv <= 6; iv++) {
          glyphs.push_back(format("dq{}_{}{}", pre, ih, iv));
        }
      }
    }
    REQUIRE(holds_alternative<Font::TrueTypeOutlines>(font->outlines));
    auto const &glyf = get<Font::TrueTypeOutlines>(font->outlines).glyf;
    for (auto const &name : glyphs) {
      auto gid = font->postGetGlyphID(name);
      REQUIRE(gid);
      Shape shape;
      REQUIRE(glyf->toShape(*gid, shape).ok());
      ostringstream stream;
      shape.toSvg(stream);
      string actual = stream.str();

      auto eFile = format("test/asset/CartoucheGlyph/{}.svg", name);
      FileInputStream fis(eFile);
      string expected = fis.readUntilEos();
      REQUIRE(!expected.empty());

      CHECK(actual == expected);
    }
  }
}
