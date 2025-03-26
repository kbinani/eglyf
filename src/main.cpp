#include "eglyf.hpp"

#include <juce_core/juce_core.h>

static void Fail(eglyf::Status st) {
  std::stringstream out;
  st.print(out);
  juce::ConsoleApplication::fail(juce::String(out.str()));
}

static void Run(juce::ArgumentList const &args) {
  using namespace std;
  using namespace eglyf;

  juce::File input = args.getExistingFileForOption("--input");
  juce::File output = args.getFileForOption("--output");

  FileInputStream fis(input);
  shared_ptr<FontFile> ff;
  if (auto st = FontFile::Read(fis, ff); !st.ok()) {
    Fail(st);
    return;
  }
  auto editor = make_shared<Editor>(ff);
  if (auto st = editor->run(); !st.ok()) {
    Fail(st);
    return;
  }
  FileOutputStream fos(output);
  if (auto st = ff->write(fos); !st.ok()) {
    Fail(st);
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
