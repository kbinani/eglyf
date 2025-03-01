// clang-format off
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include <deque>
#include <optional>
#include <variant>

#include "defer.hpp"

#include "Type.hpp"
#include "io/InputStream.hpp"
#include "io/OutputStream.hpp"
#include "io/OffsetInputStream.hpp"
#include "io/FileInputStream.hpp"
#include "io/FileOutputStream.hpp"
#include "io/ByteInputStream.hpp"
#include "io/ByteOutputStream.hpp"
#include "Tag.hpp"
#include "TableRecord.hpp"
#include "TableDirectory.hpp"
#include "table/Table.hpp"
#include "table/ReadonlyTable.hpp"
#include "table/FontHeaderTable.hpp"
#include "table/MaximumProfileTable.hpp"
#include "table/IndexToLocationTable.hpp"
#include "table/GlyphDataTable.hpp"
#include "table/PostScriptTable.hpp"
#include "table/OS2AndWindowsMetricsTable.hpp"
#include "table/HorizontalHeaderTable.hpp"
#include "table/HorizontalMetricsTable.hpp"
#include "table/ScriptList.hpp"
#include "table/FeatureList.hpp"
#include "table/LookupList.hpp"
#include "table/Coverage.hpp"
#include "table/Coverage1.hpp"
#include "table/Coverage2.hpp"
#include "table/CoverageReader.hpp"
#include "table/gsub/Subtable.hpp"
#include "table/gsub/Single.hpp"
#include "table/gsub/SequenceLookup.hpp"
#include "table/gsub/ChainedContextsSubstitution.hpp"
#include "table/gsub/SubstitutionExtension.hpp"
#include "table/GlyphSubstitutionTable.hpp"
#include "FontFile.hpp"
// clang-format on

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
