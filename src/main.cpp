#define EGLYF_ENABLE_TESTS 0

#include "eglyf.hpp"

#if EGLYF_ENABLE_TESTS
#include "editor/EditorTests.hpp"
#endif

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
  string onlyLookupWithNameValue = args.getValueForOption("--only-lookup-with-name").toStdString();
  optional<string> onlyLookupWithName;
  if (!onlyLookupWithNameValue.empty()) {
    onlyLookupWithName = onlyLookupWithNameValue;
  }

#if EGLYF_ENABLE_TESTS
  {
    using namespace eglyf::tests;
    juce::File reference = args.getExistingFileForOption("--reference");
    EditorTests editorTest(input, reference);

    juce::UnitTestRunner runner;
    runner.runAllTests();
  }
#endif

  FileInputStream fis(input);
  shared_ptr<FontFile> ff;
  if (auto st = FontFile::Read(fis, ff); !st.ok()) {
    Fail(st);
    return;
  }
  auto editor = make_shared<Editor>(ff);
  VtpParser parser(editor);
  string_view vtp(BinaryData::EgyptianText_200_vtp, BinaryData::EgyptianText_200_vtpSize);
  if (auto st = parser.parseVtp(vtp); !st.ok()) {
    Fail(st);
    return;
  }
  if (auto st = editor->compile(onlyLookupWithName); !st.ok()) {
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
