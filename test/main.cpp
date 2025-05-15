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

static hb_buffer_t *CreateBuffer(std::u32string const &str, std::shared_ptr<hb_font_t> const &font, std::vector<hb_feature_t> const &features) {
  HbBufferUniquePtr buffer(hb_buffer_create());
  hb_buffer_add_utf32(buffer.get(), (uint32_t const *)str.c_str(), -1, 0, -1);
  hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
  hb_buffer_set_script(buffer.get(), HB_SCRIPT_EGYPTIAN_HIEROGLYPHS);
  hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
  hb_shape(font.get(), buffer.get(), features.data(), features.size());
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

TEST_CASE("main") {
  using namespace std;
  using namespace eglyf;
  namespace fs = std::filesystem;
  auto f = MakeFont("test/asset/NotoSansEgyptianHieroglyphs-Regular.ttf");
  REQUIRE(f);

  SUBCASE("dump_glyph_names") {
    auto out = make_unique<ByteOutputStream>();
    REQUIRE(f->write(*out).ok());
    string data = out->data();
    out.reset();
    REQUIRE(!data.empty());
    HbBlobUniquePtr blob(hb_blob_create(data.data(), data.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr));
    HbFaceUniquePtr face(hb_face_create(blob.get(), 0));
    shared_ptr<hb_font_t> font(hb_font_create(face.get()), hb_font_destroy);
    REQUIRE(font);
    hb_feature_t vrt2;
    REQUIRE(hb_feature_from_string("vrt2", 4, &vrt2));
    vector<hb_feature_t> features;
    features.push_back(vrt2);
    HbBufferUniquePtr buf(CreateBuffer(U"𓉚𓐼𓀀𓐽𓉛"s, font, features));
    REQUIRE(buf);
    auto numGlyphs = hb_buffer_get_length(buf.get());
    hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buf.get(), nullptr);
    for (unsigned int i = 0; i < numGlyphs; i++) {
      hb_glyph_info_t info = glyphInfo[i];
      auto gid = info.codepoint;
      auto name = f->postGetName(gid);
      REQUIRE(name);
      cout << *name << endl;
    }
  }

  SUBCASE("CartoucheGlyph") {
    vector<string> glyphs = {
        "cbL",
        "creR",
        "cbR",
        "creL",
        "cbT",
        "creB",
        "cdreB",
        "crbL",
        "ceR",
        "crbR",
        "ceL",
        "crbT",
        "ceB",
        "cdeB",
        "cdrbT",
        "corbT",
        "coeB",
        "cobL",
        "coreR",
        "cobR",
        "coreL",
        "cobT",
        "coreB",
        "corbL",
        "coeR",
        "corbR",
        "coeL",
        "hwtbL",
        "hwteR",
        "hwtbR",
        "hwteL",
        "hwtobL",
        "hwtoeR",
        "hwtobR",
        "hwtoeL",
        "hwttbL",
        "hwttbR",
        "hwtteL",
        "hwtteR",
        "hwtbbL",
        "hwtbbL",
        "hwtbbR",
        "hwtbeL",
        "hwtdbL",
        "hwtdeR",
        "hwtdbR",
        "hwtdeL",
        "hwtotbL",
        "hwtoteR",
        "hwtotbR",
        "hwtoteL",
        "hwtobbL",
        "hwtobeR",
        "hwtobbR",
        "hwtobeL",
        "hwtdtbL",
        "hwtdbbL",
        "hwtdteR",
        "hwtdbeR",
        "hwtdteL",
        "hwtdtbR",
        "hwtdbbR",
        "hwtdbeL",
        "hwtbT",
        "O33aeL",
        "O33aeR",
        "O33aoeL",
        "O33aoeR",
        "O33adeL",
        "O33adeR",
        "cdbL",
        "cdbR",
        "cdreL",
        "cdreR",
        "cdbT",
        "cdrbL",
        "cdrbR",
        "cdeL",
        "cdeR",
        "cwbL",
        "cweR",
        "cwbR",
        "cweL",
        "cfbL",
        "cfeR",
        "cfbR",
        "cfeL",
        "hwbL",
        "hweR",
        "hwbR",
        "hweL",
        "hfbL",
        "hfeR",
        "hfbR",
        "hfeL",
    };
    for (int ih = 1; ih <= 8; ih++) {
      glyphs.push_back(format("QC{}", ih));
      glyphs.push_back(format("QC{}V", ih));
      glyphs.push_back(format("QO{}", ih));
      glyphs.push_back(format("QC{}V", ih));
      glyphs.push_back(format("QD{}", ih));
      glyphs.push_back(format("QD{}V", ih));
      glyphs.push_back(format("QW{}", ih));
      glyphs.push_back(format("QF{}", ih));
    }
    for (string const &pre : {"1", "2", "3", "4", "12", "13", "14", "23", "24", "34", "123", "124", "134", "234", "1234"}) {
      for (int ih = 1; ih <= 8; ih++) {
        for (int iv = 1; iv <= 6; iv++) {
          glyphs.push_back(format("dq{}_{}{}", pre, ih, iv));
        }
      }
    }
    REQUIRE(holds_alternative<Font::TrueTypeOutlines>(f->outlines));
    fs::remove_all("test/asset/CartoucheGlyph");
    fs::create_directories(fs::path("test/asset/CartoucheGlyph"));
    auto const &glyf = get<Font::TrueTypeOutlines>(f->outlines).glyf;
    for (auto const &name : glyphs) {
      auto gid = f->postGetGlyphID(name);
      REQUIRE(gid);
      Shape shape;
      REQUIRE(glyf->toShape(*gid, shape).ok());
      ofstream stream(format("test/asset/CartoucheGlyph/{}.svg", name));
      shape.toSvg(stream);
    }
  }
}
