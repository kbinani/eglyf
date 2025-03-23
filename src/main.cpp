#include "eglyf.hpp"

#include <juce_core/juce_core.h>

static void Run(juce::ArgumentList const &args) {
  using namespace std;
  using namespace eglyf;

  juce::File input = args.getExistingFileForOption("--input");
  juce::File output = args.getFileForOption("--output");

  FileInputStream fis(input);
  shared_ptr<FontFile> ff;
  if (auto st = FontFile::Read(fis, ff); !st.ok()) {
    stringstream out;
    st.print(out);
    juce::ConsoleApplication::fail(juce::String(out.str()));
    return;
  }
  auto gid0 = ff->addEmptyGlyph("foo", 0, 0);
  if (!gid0) {
    juce::ConsoleApplication::fail(juce::String("addEmptyGlyph failed"));
    return;
  }
  auto gid1 = ff->addCompositeGlyph("baz", GlyphDataTable::CompositeGlyph::GlyphRecord::New(1, 0, 0), 0, 0);
  if (!gid1) {
    juce::ConsoleApplication::fail(juce::String("addCompositeGlyph failed"));
    return;
  }
  FileOutputStream fos(output);
  if (auto st = ff->write(fos); !st.ok()) {
    stringstream out;
    st.print(out);
    juce::ConsoleApplication::fail(juce::String(out.str()));
  }
}

int main(int argc, char *argv[]) {
  juce::ConsoleApplication app;
  app.addDefaultCommand({juce::String("--run"),
                         juce::String("--run filename"),
                         juce::String(""),
                         juce::String(""),
                         [](auto const &args) { Run(args); }});
  return app.findAndRunCommand(argc, argv);
}
