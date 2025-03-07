#include "eglyf.hpp"

int main() {
  using namespace eglyf;
  FileInputStream fis(juce::File::getCurrentWorkingDirectory().getChildFile("egyptiantext-COLR.ttf"));
  auto ff = FontFile::Read(fis);
  if (!ff) {
    return 1;
  }
  auto gid0 = ff->addEmptyGlyph("foo", 0, 0);
  if (!gid0) {
    return 1;
  }
  auto gid1 = ff->addCompositeGlyph("baz", GlyphDataTable::CompositeGlyph::GlyphRecord::New(1, 0, 0), 0, 0);
  if (!gid1) {
    return 1;
  }
  FileOutputStream fos(juce::File::getCurrentWorkingDirectory().getChildFile("egyptiantext-COLR-out.ttf"));
  if (ff->write(fos)) {
    return 0;
  } else {
    return 1;
  }
}
