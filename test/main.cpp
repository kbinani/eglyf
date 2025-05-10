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

static hb_buffer_t *CreateBuffer(std::u32string const &str, std::shared_ptr<hb_font_t> const &font) {
  HbBufferUniquePtr buffer(hb_buffer_create());
  hb_buffer_add_utf32(buffer.get(), (uint32_t const *)str.c_str(), -1, 0, -1);
  hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
  hb_buffer_set_script(buffer.get(), HB_SCRIPT_EGYPTIAN_HIEROGLYPHS);
  hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
  hb_shape(font.get(), buffer.get(), nullptr, 0);
  return buffer.release();
}

TEST_CASE("main") {
  using namespace std;
  using namespace eglyf;
  FileInputStream fis("source.ttf");
  string data = fis.readUntilEos();
  HbBlobUniquePtr blob(hb_blob_create(data.data(), data.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr));
  HbFaceUniquePtr face(hb_face_create(blob.get(), 0));
  shared_ptr<hb_font_t> font(hb_font_create(face.get()), hb_font_destroy);
}
