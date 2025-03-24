#include "eglyf.hpp"

#include "editor/Editor.hpp"

namespace eglyf {

Status Editor::run() {
  using namespace std;
#include "editor/DEF_GLYPH.hpp"
  return Status::Ok();
}

} // namespace eglyf
