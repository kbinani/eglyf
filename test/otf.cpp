// clang-format off
#include "doctest.h"
#include "eglyf.hpp"
// clang-format on

using namespace std;
using namespace eglyf;

TEST_CASE("otf") {
  auto fis = make_unique<FileInputStream>("test/asset/NotoSansEgyptianHieroglyphs-Regular.otf");
  shared_ptr<Font> font;
  auto st = Font::Read(*fis, font);
  CHECK(st.ok());
  st.print(cout);
}
